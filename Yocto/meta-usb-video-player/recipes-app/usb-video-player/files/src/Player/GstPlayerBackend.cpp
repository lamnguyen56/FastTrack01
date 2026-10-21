#include "GstPlayerBackend.h"
#include "PlayerController.h"
#include "Logger.h"
#include <QMetaObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRunnable>
#include <functional>
#include <QMutex>
#include <QMutexLocker>
/** @brief Prevent a cancelled render job from reviving a released sink. */
struct RenderSessionGate {
    QMutex mutex; ///< Serializes sink initialization and pipeline teardown.
    bool cancelled = false; ///< Set before releasing the session.
};

namespace {
/** @brief Run sink initialization with Qt's current scene graph GL context. */
class SinkReadyJob final : public QRunnable {
public:
    SinkReadyJob(GstElement* sink, std::shared_ptr<RenderSessionGate> gate, std::function<void(bool)> completed)
        : gate_(std::move(gate)), sink_(GST_ELEMENT(gst_object_ref(sink))), completed_(std::move(completed)) {}

    ~SinkReadyJob() override { gst_object_unref(sink_); }

    void run() override {
        QMutexLocker lock(&gate_->mutex);
        if (gate_->cancelled == true) return;
        completed_(gst_element_set_state(sink_, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE);
    }

private:
    std::shared_ptr<RenderSessionGate> gate_; ///< Shared lifetime with cancellation owner.
    GstElement* sink_; ///< Reference retained until the render job is destroyed.
    std::function<void(bool)> completed_; ///< Queues completion back to the Qt thread.
};
}

GstPlayerBackend::GstPlayerBackend(PlayerController& controller, QObject* parent)
    : QObject(parent), controller_(controller) {
    timer_.setInterval(100);
    connect(&timer_, &QTimer::timeout, this, &GstPlayerBackend::poll);
    connect(&controller_, &PlayerController::sourceChanged, this, &GstPlayerBackend::open);
    connect(&controller_, &PlayerController::stopRequested, this, [this]() { resetStream(); });
    connect(&controller_, &PlayerController::playRequested, this, [this]() {
        if (pipeline_ == nullptr || loaded_ == false) {
            open();
        }
        desired_ = GST_STATE_PLAYING;
        ended_ = false;
        applyState();
    });
    connect(&controller_, &PlayerController::pauseRequested, this, [this]() {
        desired_ = GST_STATE_PAUSED;
        applyState();
    });
    connect(&controller_, &PlayerController::seekRequested, this, [this](qint64 milliseconds) {
        // Do not flush-seek a drained V4L2 decoder and immediately start it again.
        // Reopen the same URI through READY; playRequested follows this signal.
        if (ended_ == true && milliseconds == 0) {
            LOG_I("Reopening media after EOS");
            open();
            return;
        }
        if (pipeline_ != nullptr && ready_ == true && loaded_ == true) {
            const gboolean accepted = gst_element_seek_simple(pipeline_, GST_FORMAT_TIME,
                GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), milliseconds * GST_MSECOND);
            if (accepted != FALSE) {
                ended_ = false;
            }
        }
    });
}

GstPlayerBackend::~GstPlayerBackend() {
    close();
    if (sink_ != nullptr) {
        gst_object_unref(sink_);
    }
}

bool GstPlayerBackend::initialize() {
    sink_ = gst_element_factory_make("qmlglsink", nullptr);
    if (sink_ == nullptr) {
        LOG_E("Missing qmlglsink; install gstreamer1.0-plugins-good-qmlgl");
        return false;
    }
    gst_object_ref_sink(sink_);
    // 5 ms is tighter than one 60 Hz refresh. Keep finite lateness, sync and QoS.
    g_object_set(sink_, "max-lateness", gint64(50 * GST_MSECOND),
                 "sync", TRUE, "qos", TRUE, "show-preroll-frame", TRUE, nullptr);
    videoMonitor_.attach(sink_);
    LOG_I("Video sink timing: sync=true; QoS=true; max-lateness=50ms");
    LOG_I("Video backend: direct GStreamer playbin + qmlglsink");
    return true;
}

void GstPlayerBackend::attach(QQuickItem* item) {
    if (item == nullptr || item->window() == nullptr || sink_ == nullptr) {
        fail(QStringLiteral("GStreamer video item has no Quick window"));
        return;
    }
    if (item_ == item) {
        return;
    }
    detach();
    item_ = item;
    open();
}

void GstPlayerBackend::detach() {
    close();
    if (sink_ != nullptr) {
        g_object_set(sink_, "widget", nullptr, nullptr);
    }
    item_ = nullptr;
}

void GstPlayerBackend::open() {
    if (resetStream() == false) {
        return;
    }
    if (item_ == nullptr || controller_.source().isEmpty() == true) {
        return;
    }

    // Reuse playbin and its sink bin. READY releases the old file/decoder while
    // avoiding repeated GL sink reparenting and context construction.
    if (pipeline_ != nullptr) {
        const QByteArray uri = controller_.source().toEncoded();
        g_object_set(pipeline_, "uri", uri.constData(), nullptr);
        loaded_ = true;
        loading_ = true;
        LOG_I("Opening media: ", uri.constData());
        timer_.start();
        emit stateChanged();
        prepareSink();
        return;
    }

    // Keep Qt's sink first: GL upload must inherit its display/context.
    g_object_set(sink_, "widget", item_, nullptr);
    GstElement* video = gst_bin_new(nullptr);
    GstElement* upload = gst_element_factory_make("glupload", nullptr);
    GstElement* convert = gst_element_factory_make("glcolorconvert", nullptr);
    pipeline_ = gst_element_factory_make("playbin", nullptr);
    if (video == nullptr || upload == nullptr || convert == nullptr || pipeline_ == nullptr) {
        if (video != nullptr) gst_object_unref(video);
        if (upload != nullptr) gst_object_unref(upload);
        if (convert != nullptr) gst_object_unref(convert);
        fail(QStringLiteral("Missing playbin/glupload/glcolorconvert plugin"));
        return;
    }
    gst_object_ref_sink(pipeline_);
    gst_bin_add_many(GST_BIN(video), upload, convert,
                     sink_, nullptr);
    if (gst_element_link_many(upload, convert, sink_, nullptr) == FALSE) {
        gst_object_unref(video);
        fail(QStringLiteral("Cannot link the GL video sink"));
        return;
    }
    GstPad* pad = gst_element_get_static_pad(upload, "sink");
    gst_element_add_pad(video, gst_ghost_pad_new("sink", pad));
    gst_object_unref(pad);
    gst_object_ref_sink(video);
    const QByteArray uri = controller_.source().toEncoded();
    g_object_set(pipeline_, "uri", uri.constData(), "video-sink", video, nullptr);
    gst_object_unref(video);
    loaded_ = true;
    LOG_I("Opening media: ", uri.constData());
    bus_ = gst_element_get_bus(pipeline_);
    loading_ = true;
    emit stateChanged();
    timer_.start();

    prepareSink();
}

void GstPlayerBackend::prepareSink() {
    // playbin may take its internal sink chain to NULL even during a READY reset.
    // Re-establish Qt context on its render thread before GL upload can restart.
    QPointer<GstPlayerBackend> self(this);
    gate_ = std::make_shared<RenderSessionGate>();
    const auto gate = gate_;
    item_->window()->scheduleRenderJob(new SinkReadyJob(sink_, gate, [self, gate](bool success) {
        if (self.isNull() == true) return;
        QMetaObject::invokeMethod(self.data(), [self, gate, success]() {
            if (self.isNull() == true || self->gate_ != gate) return;
            if (success == false) {
                self->fail(QStringLiteral("Cannot initialize qmlglsink with the Qt GL context"));
                return;
            }
            self->ready_ = true;
            self->applyState();
        }, Qt::QueuedConnection);
    }), QQuickWindow::BeforeSynchronizingStage);
    item_->window()->update();
}

bool GstPlayerBackend::resetStream() {
    ++generation_;
    const auto gate = gate_;
    std::unique_ptr<QMutexLocker> lock;
    if (gate != nullptr) {
        lock = std::make_unique<QMutexLocker>(&gate->mutex);
        gate->cancelled = true;
    }
    gate_.reset();
    timer_.stop();
    loaded_ = false;
    desired_ = GST_STATE_PAUSED;
    buffering_ = false;
    loading_ = false;
    started_ = false;
    prerolled_ = false;
    videoMonitor_.reset();
    ended_ = false;
    seekable_ = false;

    if (pipeline_ != nullptr && ready_ == true) {
        // Flush old errors/EOS before accepting messages from the next URI.
        gst_bus_set_flushing(bus_, TRUE);
        const GstStateChangeReturn result = gst_element_set_state(pipeline_, GST_STATE_READY);
        gst_bus_set_flushing(bus_, FALSE);
        if (result == GST_STATE_CHANGE_FAILURE) {
            fail(QStringLiteral("Cannot reset media pipeline to READY"));
            return false;
        }
    }
    ready_ = false;
    controller_.reportPlayback(false, false);
    controller_.reportPosition(0);
    controller_.reportDuration(0);
    emit stateChanged();
    return true;
}

void GstPlayerBackend::close() {
    ++generation_;
    const auto gate = gate_;
    std::unique_ptr<QMutexLocker> lock;
    if (gate != nullptr) {
        lock = std::make_unique<QMutexLocker>(&gate->mutex);
        gate->cancelled = true;
    }
    timer_.stop();
    if (pipeline_ != nullptr) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    // A cancelled render job may have put the sink in READY while playbin stayed NULL.
    if (sink_ != nullptr) {
        gst_element_set_state(sink_, GST_STATE_NULL);
    }
    if (bus_ != nullptr) {
        gst_object_unref(bus_);
        bus_ = nullptr;
    }
    loaded_ = false;
    gate_.reset();
    ready_ = false;
    buffering_ = false;
    loading_ = false;
    started_ = false;
    prerolled_ = false;
    videoMonitor_.reset();
    ended_ = false;
    seekable_ = false;
    desired_ = GST_STATE_PAUSED;
    controller_.reportPlayback(false, false);
    emit stateChanged();
}

void GstPlayerBackend::applyState() {
    if (pipeline_ != nullptr && ready_ == true && loaded_ == true) {
        // An explicit PAUSED phase prevents audio from starting before video preroll.
        const GstState target = prerolled_ == false || buffering_ == true
                                ? GST_STATE_PAUSED : desired_;
        if (gst_element_set_state(pipeline_, target) == GST_STATE_CHANGE_FAILURE) {
            fail(QStringLiteral("GStreamer could not change playback state"));
        }
    }
}

void GstPlayerBackend::fail(const QString& message) {
    close();
    controller_.reportError(message);
}

void GstPlayerBackend::poll() {
    const quint64 ticket = generation_;
    GstMessage* message = nullptr;
    while (bus_ != nullptr && (message = gst_bus_pop(bus_)) != nullptr) {
        const GstMessageType type = GST_MESSAGE_TYPE(message);
        if (type == GST_MESSAGE_ERROR) {
            GError* error = nullptr;
            gchar* detail = nullptr;
            gst_message_parse_error(message, &error, &detail);
            const QString text = QString::fromUtf8(error != nullptr ? error->message : "Unknown GStreamer error");
            LOG_E("GStreamer element=", GST_OBJECT_NAME(GST_MESSAGE_SRC(message)),
                  "; error=", text.toStdString(), "; detail=", detail != nullptr ? detail : "");
            if (error != nullptr) g_error_free(error);
            g_free(detail);
            gst_message_unref(message);
            fail(text);
            return;
        }
        if (type == GST_MESSAGE_ELEMENT) {
            // playbin may otherwise continue with audio when no video decoder exists.
            const GstStructure* info = gst_message_get_structure(message);
            if (info != nullptr && gst_structure_has_name(info, "missing-plugin") != FALSE) {
                const gchar* missingType = gst_structure_get_string(info, "type");
                const GValue* detail = gst_structure_get_value(info, "detail");
                if (g_strcmp0(missingType, "decoder") == 0 && detail != nullptr
                    && GST_VALUE_HOLDS_CAPS(detail) != FALSE) {
                    const GstCaps* caps = gst_value_get_caps(detail);
                    if (caps != nullptr && gst_caps_is_empty(caps) == FALSE
                        && gst_caps_is_any(caps) == FALSE) {
                        const gchar* name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
                        if (g_str_has_prefix(name, "video/") != FALSE
                            || g_str_has_prefix(name, "image/") != FALSE) {
                            gst_message_unref(message);
                            fail(QStringLiteral("Unsupported video format or resolution. Hardware H.264 decoding is required."));
                            return;
                        }
                    }
                }
            }
        }
        if (type == GST_MESSAGE_EOS) {
            gst_message_unref(message);
            // A short clip can finish between polls; account for its final frame
            // before holding EOS, otherwise the loading overlay would hide it.
            if (prerolled_ == true && videoMonitor_.poll(false) == true) {
                started_ = true;
            }
            loading_ = false;
            emit stateChanged();
            if (ended_ == false) {
                ended_ = true;
                desired_ = GST_STATE_PAUSED;
                controller_.reportPlayback(false, seekable_);
                // Controller may replace and destroy the pipeline synchronously.
                controller_.handleEndOfMedia();
            }
            return;
        }
        if (type == GST_MESSAGE_BUFFERING) {
            gint percent = 100;
            gst_message_parse_buffering(message, &percent);
            buffering_ = percent < 100;
            loading_ = buffering_;
            applyState();
            emit stateChanged();
        }
        gst_message_unref(message);
        if (generation_ != ticket) return;
    }
    if (pipeline_ == nullptr || ready_ == false || loaded_ == false) return;

    gint64 value = 0;
    if (gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &value) != FALSE) {
        controller_.reportDuration(value / GST_MSECOND);
    }
    if (gst_element_query_position(pipeline_, GST_FORMAT_TIME, &value) != FALSE) {
        controller_.reportPosition(value / GST_MSECOND);
    }
    GstQuery* query = gst_query_new_seeking(GST_FORMAT_TIME);
    gboolean canSeek = FALSE;
    if (gst_element_query(pipeline_, query) != FALSE) {
        gst_query_parse_seeking(query, nullptr, &canSeek, nullptr, nullptr);
    }
    gst_query_unref(query);
    seekable_ = canSeek != FALSE;
    GstState state = GST_STATE_NULL;
    GstState pending = GST_STATE_VOID_PENDING;
    gst_element_get_state(pipeline_, &state, &pending, 0);
    if (prerolled_ == false && state == GST_STATE_PAUSED && pending == GST_STATE_VOID_PENDING) {
        gint videoStreams = 0;
        g_object_get(pipeline_, "n-video", &videoStreams, nullptr);
        if (videoStreams <= 0) {
            fail(QStringLiteral("No supported video stream. Hardware H.264 video is required."));
            return;
        }
        prerolled_ = true;
        videoMonitor_.begin();
        LOG_I("Video preroll complete; streams=", videoStreams);
        applyState();
        if (generation_ != ticket) {
            return;
        }
    }

    const bool hasRenderedFrame = prerolled_ == true
                                 && videoMonitor_.poll(state == GST_STATE_PLAYING && ended_ == false);
    if (hasRenderedFrame == true && started_ == false) {
        started_ = true;
        LOG_I("Video frame delivery started: ", controller_.source().toString().toStdString());
        emit stateChanged();
    }
    const bool waiting = prerolled_ == false || buffering_ == true
                         || (desired_ == GST_STATE_PLAYING && started_ == false);
    if (loading_ != waiting) {
        loading_ = waiting;
        emit stateChanged();
    }
    controller_.reportPlayback(state == GST_STATE_PLAYING && ended_ == false, seekable_);
}
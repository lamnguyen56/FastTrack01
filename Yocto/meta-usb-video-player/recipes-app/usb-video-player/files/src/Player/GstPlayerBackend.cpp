#include "GstPlayerBackend.h"
#include "PlayerController.h"
#include "Logger.h"
#include <QMetaObject>
#include <QQuickWindow>
#include <QRunnable>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <mutex>

/** @brief Coordinate only final destruction; GUI transitions never wait on this mutex. */
struct RenderSessionGate {
    std::mutex mutex; ///< Protect render-job lifetime during final shutdown.
    std::condition_variable finished; ///< Wake shutdown after a running render job exits.
    bool cancelled = false; ///< A queued job must not initialize a detached video item.
    bool running = false; ///< Render thread is currently using the sink.
    bool done = false; ///< Callback was queued and the render job no longer uses the item.
};

namespace {
/** @brief The only state operation requiring Qt's current render-thread GL context. */
class SinkReadyJob final : public QRunnable {
public:
    /** @brief Retain the sink until its render job finishes or is cancelled. */
    SinkReadyJob(GstElement* sink, std::shared_ptr<RenderSessionGate> gate,
                 std::function<void(bool)> completed)
        : sink_(GST_ELEMENT(gst_object_ref(sink))), gate_(std::move(gate)), completed_(std::move(completed)) {}

    /** @brief Release the job reference independently of the backend reference. */
    ~SinkReadyJob() override { gst_object_unref(sink_); }

    /** @brief Prepare the sink using Qt render-thread GL context, then queue completion. */
    void run() override {
        {
            std::lock_guard<std::mutex> lock(gate_->mutex);
            if (gate_->cancelled == true) return;
            gate_->running = true;
        }
        const bool success = gst_element_set_state(sink_, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE;
        completed_(success);
        {
            std::lock_guard<std::mutex> lock(gate_->mutex);
            gate_->done = true;
        }
        gate_->finished.notify_all();
    }

private:
    GstElement* sink_; ///< Reference retained even if a queued job is cancelled.
    std::shared_ptr<RenderSessionGate> gate_; ///< Shared cancellation/lifetime state.
    std::function<void(bool)> completed_; ///< Queue completion to the GUI; never mutate QML here.
};
}

GstPlayerBackend::GstPlayerBackend(PlayerController& controller, QObject* parent)
    : QObject(parent), controller_(controller), operations_(this) {
    // Controller signals carry user intent; only worker snapshots report actual
    // decoder state. Slots below must never perform blocking pipeline operations.
    timer_.setInterval(100);
    connect(&timer_, &QTimer::timeout, this, &GstPlayerBackend::poll);
    connect(&controller_, &PlayerController::sourceChanged, this, [this]() {
        requestSource(controller_.source());
    });
    connect(&controller_, &PlayerController::stopRequested, this, [this]() { requestSource({}); });
    connect(&controller_, &PlayerController::playRequested, this, [this]() {
        if ((requests_.pending() == true && requests_.source().isEmpty() == true)
            || (loaded_ == false && requests_.pending() == false)) {
            requestSource(controller_.source());
        }
        desired_ = GST_STATE_PLAYING;
        schedule();
    });
    connect(&controller_, &PlayerController::pauseRequested, this, [this]() {
        desired_ = GST_STATE_PAUSED;
        schedule();
    });
    connect(&controller_, &PlayerController::seekRequested, this, [this](qint64 milliseconds) {
        if (ended_ == true && milliseconds == 0) {
            LOG_I("Reopening media after EOS");
            requestSource(controller_.source());
        } else {
            seek_ = milliseconds;
            schedule();
        }
    });
}

GstPlayerBackend::~GstPlayerBackend() {
    detach();
    session_.reset();
    if (sink_ != nullptr) gst_object_unref(sink_);
}

bool GstPlayerBackend::initialize() {
    // Creating this plugin registers its QML type before the engine loads the UI.
    // HardwareDecodePolicy has already initialized GStreamer in PlayerApplication.
    sink_ = gst_element_factory_make("qmlglsink", nullptr);
    if (sink_ == nullptr) {
        LOG_E("Missing qmlglsink; install gstreamer1.0-plugins-good-qmlgl");
        return false;
    }
    gst_object_ref_sink(sink_);
    g_object_set(sink_, "max-lateness", gint64(50 * GST_MSECOND), "sync", TRUE,
                 "qos", TRUE, "show-preroll-frame", TRUE, nullptr);
    session_ = std::make_unique<GstPipelineSession>(sink_);
    LOG_I("Video backend: direct GStreamer playbin + qmlglsink; serialized GstControl worker");
    LOG_I("Video sink timing: sync=true; QoS=true; max-lateness=50ms");
    return true;
}

void GstPlayerBackend::attach(QQuickItem* item) {
    if (closed_ == true || item == nullptr || item->window() == nullptr || sink_ == nullptr) {
        controller_.reportError(QStringLiteral("GStreamer video item has no Quick window"));
        return;
    }
    if (item_ == item) return;
    if (item_.isNull() == false) {
        controller_.reportError(QStringLiteral("Detach the existing video view before replacing it"));
        return;
    }
    item_ = item;
    g_object_set(sink_, "widget", item, nullptr);
    requestSource(controller_.source());
    timer_.start();
}

void GstPlayerBackend::requestSource(const QUrl& source) {
    if (closed_ == true) return;
    // Invalidate in-flight callbacks immediately; only the newest selection may
    // update UI state after the previous worker operation completes.
    requests_.replace(source);
    desired_ = GST_STATE_PAUSED;
    seek_.reset();
    loaded_ = false;
    ready_ = false;
    prerolled_ = false;
    buffering_ = false;
    started_ = false;
    ended_ = false;
    loading_ = source.isEmpty() == false;
    controller_.reportPlayback(false, false);
    controller_.reportPosition(0);
    controller_.reportDuration(0);
    emit stateChanged();
    schedule();
}

void GstPlayerBackend::schedule() {
    if (closed_ == true || scheduled_ == true) return;
    scheduled_ = true;
    // Defer once so Stop -> SourceChanged -> Play from a single click is handled
    // as one final intent instead of three competing pipeline transitions.
    QTimer::singleShot(0, this, [this]() {
        scheduled_ = false;
        dispatch();
    });
}

void GstPlayerBackend::dispatch() {
    if (closed_ == true || session_ == nullptr || item_.isNull() == true
        || operations_.busy() == true || preparing_ == true) return;

    // Selection/reset has priority over seek and playback-state requests.
    // The busy guard above keeps all session access serialized on GstControl.
    if (requests_.pending() == true) {
        const auto request = requests_.take();
        auto error = std::make_shared<QString>();
        operations_.submit(QStringLiteral("flush/reset media"), [this, request, error]() {
            *error = session_->reset(request.source);
        }, [this, request, error]() {
            if (requests_.current(request.revision) == false) {
                schedule();
                return;
            }
            if (error->isEmpty() == false) {
                // Do not endlessly queue another reset when stopping itself failed.
                controller_.reportError(*error);
                loading_ = false;
                emit stateChanged();
                return;
            }
            submitted_ = GST_STATE_VOID_PENDING;
            loaded_ = request.source.isEmpty() == false;
            if (loaded_ == true) prepareSink(request.revision);
            else schedule();
        });
        return;
    }
    if (loaded_ == false || ready_ == false) return;

    if (seek_.has_value() == true && prerolled_ == true) {
        const qint64 milliseconds = seek_.value();
        seek_.reset();
        const quint64 revision = requests_.revision();
        operations_.submit(QStringLiteral("seek media"), [this, milliseconds]() {
            session_->seek(milliseconds);
        }, [this, revision]() {
            if (requests_.current(revision) == true) ended_ = false;
            schedule();
        });
        return;
    }
    // Preroll and buffering temporarily require PAUSED even when the user wants
    // PLAYING. Resume the requested state after a worker snapshot confirms readiness.
    const GstState target = prerolled_ == false || buffering_ == true ? GST_STATE_PAUSED : desired_;
    if (target == submitted_) return;
    submitted_ = target;
    const quint64 revision = requests_.revision();
    auto error = std::make_shared<QString>();
    operations_.submit(QStringLiteral("change playback state"), [this, target, error]() {
        *error = session_->setState(target);
    }, [this, revision, error]() {
        if (requests_.current(revision) == true && error->isEmpty() == false) fail(*error);
        else schedule();
    });
}

void GstPlayerBackend::prepareSink(quint64 revision) {
    preparing_ = true;
    gate_ = std::make_shared<RenderSessionGate>();
    const auto gate = gate_;
    // This one READY operation needs Qt current GL context. Normal pipeline
    // transitions remain on GstControl; their dispatch waits for preparing_ to clear.
    item_->window()->scheduleRenderJob(new SinkReadyJob(sink_, gate, [this, revision](bool success) {
        QMetaObject::invokeMethod(this, [this, revision, success]() {
            if (closed_ == true) return;
            preparing_ = false;
            if (requests_.current(revision) == false) {
                schedule();
                return;
            }
            if (success == false) {
                fail(QStringLiteral("Cannot initialize qmlglsink with the Qt GL context"));
                return;
            }
            ready_ = true;
            schedule();
        }, Qt::QueuedConnection);
    }), QQuickWindow::BeforeSynchronizingStage);
    item_->window()->update();
}

void GstPlayerBackend::poll() {
    if (closed_ == true || operations_.busy() == true || preparing_ == true) return;
    dispatch();
    if (operations_.busy() == true || loaded_ == false || ready_ == false || requests_.pending() == true) return;
    const quint64 revision = requests_.revision();
    // Worker fills a value snapshot, then its completion applies it on the GUI.
    // Revision checks reject data belonging to a track that has since been replaced.
    auto snapshot = std::make_shared<GstSessionSnapshot>();
    operations_.submit(QStringLiteral("query playback"), [this, snapshot]() {
        *snapshot = session_->poll();
    }, [this, revision, snapshot]() {
        if (requests_.current(revision) == false) {
            schedule();
            return;
        }
        if (snapshot->error.isEmpty() == false) {
            fail(snapshot->error);
            return;
        }
        prerolled_ = snapshot->prerolled;
        buffering_ = snapshot->buffering;
        if (snapshot->started == true && started_ == false) {
            started_ = true;
            LOG_I("Video frame delivery started: ", controller_.source().toString().toStdString());
        }
        controller_.reportPosition(snapshot->position);
        controller_.reportDuration(snapshot->duration);
        controller_.reportPlayback(snapshot->playing, snapshot->seekable);
        loading_ = prerolled_ == false || buffering_ == true
                   || (desired_ == GST_STATE_PLAYING && started_ == false);
        if (snapshot->ended == true && ended_ == false) {
            ended_ = true;
            desired_ = GST_STATE_PAUSED;
            loading_ = false;
            emit stateChanged();
            controller_.handleEndOfMedia();
        } else {
            emit stateChanged();
        }
        schedule();
    });
}

void GstPlayerBackend::fail(const QString& message) {
    requestSource({});
    controller_.reportError(message);
}

void GstPlayerBackend::detach() {
    if (closed_ == true) return;
    closed_ = true;
    timer_.stop();
    if (gate_ != nullptr) {
        std::unique_lock<std::mutex> lock(gate_->mutex);
        gate_->cancelled = true;
        if (gate_->running == true && gate_->finished.wait_for(lock, std::chrono::seconds(15),
            [this]() { return gate_->done; }) == false) {
            LOG_E("Render sink initialization did not finish during shutdown");
            std::_Exit(EXIT_FAILURE);
        }
    }
    // Final teardown drains the worker before detaching the QML widget. This
    // prevents a queued operation from accessing a released session or video item.
    operations_.shutdown([this]() {
        if (session_ != nullptr) session_->release();
    });
    if (sink_ != nullptr) g_object_set(sink_, "widget", nullptr, nullptr);
    item_ = nullptr;
}

#include "GstPipelineSession.h"
#include "Logger.h"

GstPipelineSession::GstPipelineSession(GstElement* sink)
    : sink_(GST_ELEMENT(gst_object_ref(sink))) {
    monitor_.attach(sink_);
}

GstPipelineSession::~GstPipelineSession() {
    // release() is executed by the worker before it is joined.
    Q_ASSERT(pipeline_ == nullptr);
    gst_object_unref(sink_);
}

QString GstPipelineSession::create() {
    if (pipeline_ != nullptr) {
        return {};
    }
    // Keep one GL sink bin for this session: playbin owns decoding and audio,
    // while this bin uploads/converts video for the existing QML video item.
    GstElement* video = gst_bin_new(nullptr);
    GstElement* upload = gst_element_factory_make("glupload", nullptr);
    GstElement* convert = gst_element_factory_make("glcolorconvert", nullptr);
    GstElement* player = gst_element_factory_make("playbin", nullptr);
    if (video == nullptr || upload == nullptr || convert == nullptr || player == nullptr) {
        if (video != nullptr) gst_object_unref(video);
        if (upload != nullptr) gst_object_unref(upload);
        if (convert != nullptr) gst_object_unref(convert);
        if (player != nullptr) gst_object_unref(player);
        return QStringLiteral("Missing playbin/glupload/glcolorconvert plugin");
    }
    gst_bin_add_many(GST_BIN(video), upload, convert, sink_, nullptr);
    if (gst_element_link_many(upload, convert, sink_, nullptr) == FALSE) {
        gst_object_unref(video);
        gst_object_unref(player);
        return QStringLiteral("Cannot link the GL video sink");
    }
    // Expose glupload input as the bin input so playbin can link video to it.
    GstPad* pad = gst_element_get_static_pad(upload, "sink");
    GstPad* ghost = pad != nullptr ? gst_ghost_pad_new("sink", pad) : nullptr;
    if (pad != nullptr) gst_object_unref(pad);
    if (ghost == nullptr || gst_element_add_pad(video, ghost) == FALSE) {
        if (ghost != nullptr) gst_object_unref(ghost);
        gst_object_unref(video);
        gst_object_unref(player);
        return QStringLiteral("Cannot create the video sink input pad");
    }
    gst_object_ref_sink(video);
    gst_object_ref_sink(player);
    g_object_set(player, "video-sink", video, nullptr);
    gst_object_unref(video);
    pipeline_ = player;
    bus_ = gst_element_get_bus(pipeline_);
    return {};
}

QString GstPipelineSession::transition(GstState state) {
    GstStateChangeReturn result = gst_element_set_state(pipeline_, state);
    if (result == GST_STATE_CHANGE_ASYNC) {
        // Returning ASYNC is not completion. Do not reset/reuse a decoder while
        // its previous READY/PAUSED transition is still acquiring hardware buffers.
        result = gst_element_get_state(pipeline_, nullptr, nullptr, 5 * GST_SECOND);
    }
    if (result == GST_STATE_CHANGE_FAILURE || result == GST_STATE_CHANGE_ASYNC) {
        // State failure is only a symptom. Decodebin posts the useful decoder/caps
        // error on the bus; consume it before the next reset flushes that bus.
        const QString error = drainBus();
        if (error.isEmpty() == false) {
            return error;
        }
    }
    if (result == GST_STATE_CHANGE_ASYNC) {
        return QStringLiteral("Timed out waiting for GStreamer %1")
            .arg(QString::fromLatin1(gst_element_state_get_name(state)));
    }
    if (result == GST_STATE_CHANGE_FAILURE) {
        return QStringLiteral("GStreamer could not enter %1")
            .arg(QString::fromLatin1(gst_element_state_get_name(state)));
    }
    return {};
}
void GstPipelineSession::flush() {
    if (pipeline_ != nullptr && used_ == true) {
        // FLUSH_START reaches decoder sink_event(), which unlocks V4L2 output
        // and capture polling before gst_pad_stop_task() joins the source task.
        // It is intentionally sent on GstControl: plugin event handlers can block too.
        gst_element_send_event(pipeline_, gst_event_new_flush_start());
    }
}

QString GstPipelineSession::reset(const QUrl& source) {
    if (pipeline_ == nullptr && source.isEmpty() == false) {
        const QString error = create();
        if (error.isEmpty() == false) return error;
    }
    if (pipeline_ != nullptr) {
        // Discard messages from the previous URI while its stream is reset.
        // Re-enable the bus before opening the next URI so its errors are visible.
        gst_bus_set_flushing(bus_, TRUE);
        if (used_ == true) {
            flush();
            const QString error = transition(GST_STATE_READY);
            if (error.isEmpty() == false) {
                gst_bus_set_flushing(bus_, FALSE);
                return error;
            }
            gst_element_send_event(pipeline_, gst_event_new_flush_stop(TRUE));
        }
        gst_bus_set_flushing(bus_, FALSE);
        if (source.isEmpty() == false) {
            const QByteArray uri = source.toEncoded();
            g_object_set(pipeline_, "uri", uri.constData(), nullptr);
            LOG_I("Opening media: ", uri.constData());
        }
    }
    // These flags describe the new URI; old preroll, EOS and frame counters
    // must not make a newly selected video appear ready before it delivers data.
    used_ = false;
    loaded_ = source.isEmpty() == false;
    prerolled_ = false;
    buffering_ = false;
    ended_ = false;
    monitor_.reset();
    return {};
}

QString GstPipelineSession::setState(GstState state) {
    if (pipeline_ == nullptr || loaded_ == false) return {};
    used_ = true;
    return transition(state);
}

QString GstPipelineSession::seek(qint64 milliseconds) {
    if (pipeline_ == nullptr || loaded_ == false) return {};
    if (gst_element_seek_simple(pipeline_, GST_FORMAT_TIME,
        GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), milliseconds * GST_MSECOND) == FALSE) {
        LOG_W("Media seek rejected at ", milliseconds, " ms");
    } else {
        ended_ = false;
    }
    return {};
}

QString GstPipelineSession::drainBus() {
    QString backendError;
    QString unsupportedVideo;
    QString resourceError;
    GstMessage* message = nullptr;

    // Drain the whole batch: a generic demux error may precede the more useful
    // missing-video-decoder message. Never hide file/device resource failures.
    while ((message = gst_bus_pop(bus_)) != nullptr) {
        const GstMessageType type = GST_MESSAGE_TYPE(message);
        if (type == GST_MESSAGE_ERROR) {
            GError* error = nullptr;
            gchar* detail = nullptr;
            gst_message_parse_error(message, &error, &detail);
            const QString text = QString::fromUtf8(error != nullptr ? error->message : "Unknown GStreamer error");
            if (backendError.isEmpty() == true) {
                backendError = text;
            }
            if (error != nullptr && error->domain == GST_RESOURCE_ERROR) {
                resourceError = text;
            }
            LOG_E("GStreamer element=", GST_OBJECT_NAME(GST_MESSAGE_SRC(message)),
                  "; error=", text.toStdString(), "; detail=", detail != nullptr ? detail : "");
            if (error != nullptr) g_error_free(error);
            g_free(detail);
        } else if (type == GST_MESSAGE_ELEMENT) {
            const GstStructure* info = gst_message_get_structure(message);
            if (info != nullptr && gst_structure_has_name(info, "missing-plugin") != FALSE) {
                const GValue* detail = gst_structure_get_value(info, "detail");
                if (g_strcmp0(gst_structure_get_string(info, "type"), "decoder") == 0
                    && detail != nullptr && GST_VALUE_HOLDS_CAPS(detail) != FALSE) {
                    const GstCaps* caps = gst_value_get_caps(detail);
                    if (caps != nullptr && gst_caps_is_empty(caps) == FALSE && gst_caps_is_any(caps) == FALSE) {
                        const GstStructure* video = gst_caps_get_structure(caps, 0);
                        const gchar* name = gst_structure_get_name(video);
                        if (g_str_has_prefix(name, "video/") != FALSE || g_str_has_prefix(name, "image/") != FALSE) {
                            gint width = 0;
                            gint height = 0;
                            QString dimensions;
                            if (gst_structure_get_int(video, "width", &width) != FALSE
                                && gst_structure_get_int(video, "height", &height) != FALSE) {
                                dimensions = QStringLiteral(" (%1 x %2)").arg(width).arg(height);
                            }
                            unsupportedVideo = QStringLiteral(
                                "Unsupported video format or resolution%1. Hardware H.264 decoding is required.")
                                .arg(dimensions);
                            gchar* description = gst_caps_to_string(caps);
                            LOG_W("No supported hardware video decoder: ", description != nullptr ? description : name);
                            g_free(description);
                        }
                    }
                }
            }
        } else if (type == GST_MESSAGE_EOS) {
            ended_ = true;
        } else if (type == GST_MESSAGE_BUFFERING) {
            gint percent = 100;
            gst_message_parse_buffering(message, &percent);
            buffering_ = percent < 100;
        }
        gst_message_unref(message);
    }

    // File/device/buffer failures take priority over missing-codec hints.
    // Only explicit missing-video-decoder caps justify an unsupported-video message.
    if (resourceError.isEmpty() == false) return resourceError;
    if (unsupportedVideo.isEmpty() == false) return unsupportedVideo;
    return backendError;
}

GstSessionSnapshot GstPipelineSession::poll() {
    GstSessionSnapshot snapshot;
    if (pipeline_ == nullptr || loaded_ == false) return snapshot;
    snapshot.error = drainBus();
    if (snapshot.error.isEmpty() == false) return snapshot;

    GstState state = GST_STATE_NULL;
    GstState pending = GST_STATE_VOID_PENDING;
    gst_element_get_state(pipeline_, &state, &pending, 0);
    // PAUSED completion establishes the stream topology. Reject audio-only
    // playback here when no supported video stream reached the pipeline.
    if (prerolled_ == false && state == GST_STATE_PAUSED && pending == GST_STATE_VOID_PENDING) {
        gint count = 0;
        g_object_get(pipeline_, "n-video", &count, nullptr);
        if (count <= 0) {
            snapshot.error = QStringLiteral("No supported video stream. Hardware H.264 video is required.");
            return snapshot;
        }
        prerolled_ = true;
        monitor_.begin();
        LOG_I("Video preroll complete; streams=", count);
    }
    gint64 value = 0;
    if (gst_element_query_position(pipeline_, GST_FORMAT_TIME, &value) != FALSE) snapshot.position = value / GST_MSECOND;
    if (gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &value) != FALSE) snapshot.duration = value / GST_MSECOND;
    // Query the active pipeline rather than assuming every local file is seekable.
    GstQuery* query = gst_query_new_seeking(GST_FORMAT_TIME);
    gboolean seekable = FALSE;
    if (gst_element_query(pipeline_, query) != FALSE) gst_query_parse_seeking(query, nullptr, &seekable, nullptr, nullptr);
    gst_query_unref(query);
    snapshot.seekable = seekable != FALSE;
    snapshot.playing = state == GST_STATE_PLAYING && ended_ == false;
    snapshot.started = prerolled_ == true && monitor_.poll(snapshot.playing);
    snapshot.prerolled = prerolled_;
    snapshot.buffering = buffering_;
    snapshot.ended = ended_;
    return snapshot;
}

void GstPipelineSession::release() {
    if (pipeline_ != nullptr) {
        gst_bus_set_flushing(bus_, TRUE);
        flush();
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        // Drop the pipeline only on the worker after streaming has been stopped.
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    gst_element_set_state(sink_, GST_STATE_NULL);
    if (bus_ != nullptr) {
        gst_object_unref(bus_);
        bus_ = nullptr;
    }
    used_ = false;
    loaded_ = false;
}

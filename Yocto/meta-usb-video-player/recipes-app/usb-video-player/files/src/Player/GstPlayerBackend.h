#pragma once

#include <QObject>
#include <QTimer>
#include "GstVideoMonitor.h"
#include <gst/gst.h>
#include <memory>
struct RenderSessionGate;

class QQuickItem;
class PlayerController;

/** @brief Qt-thread transport adapter for a direct GStreamer/Qt Quick GL pipeline. */
class GstPlayerBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool playbackStarted READ playbackStarted NOTIFY stateChanged)

public:
    /** @brief Connect controller commands; the controller must outlive this object. */
    explicit GstPlayerBackend(PlayerController& controller, QObject* parent = nullptr);
    /** @brief Release the pipeline before QML destroys its video item. */
    ~GstPlayerBackend() override;
    /** @brief Load qmlglsink early so its QML type exists before engine.load(). */
    bool initialize();
    /** @brief Attach the GL item after it belongs to a visible Quick window. */
    Q_INVOKABLE void attach(QQuickItem* item);
    /** @brief Stop streaming and release the item before unloading the QML view. */
    Q_INVOKABLE void detach();
    bool loading() const { return loading_; } ///< Waiting for preroll or buffering.
    bool playbackStarted() const { return started_; } ///< Current URI delivered a rendered buffer to qmlglsink.

signals:
    void stateChanged(); ///< Refresh loading and placeholder bindings.

private:
    /** @brief Reopen media through READY; build/create the GL pipeline only once. */
    void open();
    /** @brief Schedule sink READY on the render thread before each URI preroll. */
    void prepareSink();
    /** @brief Release all pipeline references and invalidate pending render jobs. */
    void close();
    /** @brief Stop the stream, close its file and flush messages while retaining GL objects. */
    bool resetStream();
    /** @brief Poll the bus and timeline without requiring a GLib main loop. */
    void poll();
    /** @brief Publish an error and release media handles without auto-skipping. */
    void fail(const QString& message);
    /** @brief Change state only once the Qt GL context has been propagated. */
    void applyState();

    PlayerController& controller_; ///< Transport and model owner.
    QQuickItem* item_ = nullptr; ///< QML-owned video item, valid until detach().
    GstElement* sink_ = nullptr; ///< Independently owned qmlglsink reference.
    GstElement* pipeline_ = nullptr; ///< Owned playbin; owns its video sink bin.
    GstBus* bus_ = nullptr; ///< Owned message queue reference.
    GstVideoMonitor videoMonitor_; ///< First-frame and stalled-video diagnostics.
    bool prerolled_ = false; ///< Complete PAUSED preroll before starting the playback clock.
    QTimer timer_; ///< Bus and timeline poll at 100 ms.
    std::shared_ptr<RenderSessionGate> gate_; ///< Serialize cancellation against render-thread initialization.
    quint64 generation_ = 0; ///< Reject callbacks from obsolete render jobs.
    GstState desired_ = GST_STATE_PAUSED; ///< User intent retained during preroll/buffering.
    bool loaded_ = false; ///< A URI is active; Stop must not restart a retained pipeline.
    bool ready_ = false; ///< Sink READY initialization completed on render thread.
    bool buffering_ = false; ///< Non-live file playback is waiting for data.
    bool loading_ = false; ///< UI loading state.
    bool started_ = false; ///< Session has delivered a rendered video buffer.
    bool ended_ = false; ///< Prevent repeated EOS notifications.
    bool seekable_ = false; ///< Last queried seek capability.
};

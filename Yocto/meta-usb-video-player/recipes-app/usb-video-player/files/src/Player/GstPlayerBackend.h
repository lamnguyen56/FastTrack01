#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QQuickItem>
#include <memory>
#include <optional>
#include "GstCommandWorker.h"
#include "GstPipelineSession.h"
#include "LatestMediaRequest.h"

struct RenderSessionGate;
class PlayerController;

/** @brief Nonblocking Qt Quick facade; serial media operations run on GstControl. */
class GstPlayerBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool playbackStarted READ playbackStarted NOTIFY stateChanged)

public:
    /** @brief Wire controller intent without running pipeline transitions in its slots. */
    explicit GstPlayerBackend(PlayerController& controller, QObject* parent = nullptr);
    /** @brief Join media cleanup before the QML-owned video item can disappear. */
    ~GstPlayerBackend() override;
    /** @brief Register the GL QML type before engine.load(). */
    bool initialize();
    /** @brief Attach the single QML video item before starting the worker pipeline. */
    Q_INVOKABLE void attach(QQuickItem* item);
    /** @brief Cancel render preparation and drain worker cleanup at application shutdown. */
    Q_INVOKABLE void detach();
    bool loading() const { return loading_; } ///< Pending transition/preroll/first-frame feedback.
    bool playbackStarted() const { return started_; } ///< Current URI delivered a buffer to the GL sink.

signals:
    void stateChanged(); ///< Refresh the QML loading and video placeholder bindings.

private:
    /** @brief Replace pending selection and invalidate older asynchronous results. */
    void requestSource(const QUrl& source);
    /** @brief Defer dispatch so Stop/Source/Play in the same UI event are coalesced. */
    void schedule();
    /** @brief Submit exactly one reset, seek or state operation in priority order. */
    void dispatch();
    /** @brief Initialize GL with Qt's current context after the old stream has stopped. */
    void prepareSink(quint64 revision);
    /** @brief Poll immutable worker snapshots; never query GStreamer from the GUI. */
    void poll();
    /** @brief Publish a current-session failure and asynchronously stop its stream. */
    void fail(const QString& message);

    PlayerController& controller_; ///< GUI-owned transport/model facade.
    QPointer<QQuickItem> item_; ///< Non-owning Qt Quick video item.
    GstElement* sink_ = nullptr; ///< Owned registration/render-job reference.
    GstCommandWorker operations_; ///< Owns the serialized GstControl executor and watchdog.
    std::unique_ptr<GstPipelineSession> session_; ///< Accessed only by GstControl after construction.
    LatestMediaRequest requests_; ///< One-slot latest-selection mailbox.
    std::shared_ptr<RenderSessionGate> gate_; ///< Shutdown guard for a scheduled GL render job.
    QTimer timer_; ///< GUI timer requests nonblocking worker snapshots.
    GstState desired_ = GST_STATE_PAUSED; ///< Latest Play/Pause intent, including during reset.
    GstState submitted_ = GST_STATE_VOID_PENDING; ///< Last state command for the current URI.
    std::optional<qint64> seek_; ///< Latest seek, discarded when its URI changes.
    bool scheduled_ = false; ///< Only one deferred dispatcher is queued.
    bool preparing_ = false; ///< Render job owns the sink; worker must remain idle.
    bool loaded_ = false; ///< Worker completed a reset to a nonempty URI.
    bool ready_ = false; ///< Render-thread sink preparation completed.
    bool prerolled_ = false; ///< Latest worker snapshot completed video preroll.
    bool buffering_ = false; ///< Latest worker snapshot is waiting for data.
    bool loading_ = false; ///< UI loading state.
    bool started_ = false; ///< Current URI delivered its first rendered buffer.
    bool ended_ = false; ///< EOS already delivered to the controller.
    bool closed_ = false; ///< Final shutdown rejects all pending callbacks/commands.
};

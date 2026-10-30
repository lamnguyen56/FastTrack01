#pragma once

#include "MediaSessionLifecycle.h"
#include "PlaybackCoordinator.h"
#include "PlayerController.h"
#include <QPointer>
#include <QRectF>

/** @brief Route controller signals and backend reports; QML owns only media/render objects. */
class VideoPlaybackBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(PlayerController* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(MediaSessionLifecycle* lifecycle READ lifecycle WRITE setLifecycle NOTIFY lifecycleChanged)
    Q_PROPERTY(PlaybackCoordinator* transport READ transport WRITE setTransport NOTIFY transportChanged)
    Q_PROPERTY(QObject* backend READ backend WRITE setBackend NOTIFY backendChanged)
    Q_PROPERTY(bool qt5 READ qt5 WRITE setQt5 NOTIFY qt5Changed)
    Q_PROPERTY(bool playbackStarted READ playbackStarted NOTIFY stateChanged)
    Q_PROPERTY(bool blocked READ blocked NOTIFY stateChanged)
public:
    explicit VideoPlaybackBridge(QObject* parent = nullptr) : QObject(parent) {}
    PlayerController* controller() const { return controller_; }
    void setController(PlayerController* controller);
    MediaSessionLifecycle* lifecycle() const { return lifecycle_; }
    void setLifecycle(MediaSessionLifecycle* lifecycle);
    PlaybackCoordinator* transport() const { return transport_; }
    void setTransport(PlaybackCoordinator* transport);
    QObject* backend() const { return backend_; }
    void setBackend(QObject* backend);
    bool qt5() const { return qt5_; }
    void setQt5(bool qt5);
    bool playbackStarted() const { return started_; }
    bool blocked() const { return blocked_; }

    /** @brief Accept reports only from the still-current, unblocked session. */
    Q_INVOKABLE void reportPosition(qint64 position);
    Q_INVOKABLE void reportDuration(qint64 duration);
    /** @brief Log the negotiated video dimensions reported by the view. */
    Q_INVOKABLE void reportVideoRect(const QRectF& rect);
    Q_INVOKABLE void reportPlayback(bool playing, bool seekable, qint64 duration);
    /** @brief Stop rejected media and publish a format-specific or original I/O error. */
    Q_INVOKABLE void reportError(bool formatFailure, const QString& detail);
    /** @brief Decode Qt Multimedia error enums, ignoring NoError notifications. */
    Q_INVOKABLE void reportMediaError(int error, const QString& detail);
    /** @brief Release/recreate the selected session after the QML adapter is complete. */
    Q_INVOKABLE void initialize();
    /** @brief Load Qt5 GL media only after VideoOutput publishes its render context. */
    Q_INVOKABLE void prepareMedia(QObject* media, QObject* surface, const QUrl& source);
signals:
    void controllerChanged();
    void lifecycleChanged();
    void transportChanged();
    void backendChanged();
    void qt5Changed();
    void stateChanged();
private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void tryPendingLoad();
    void cancelPendingLoad();
    bool current() const;
    void releaseBackend();
    void stopBackend();
    void seek(qint64 position);
    void checkConstructionError();
    QPointer<PlayerController> controller_;
    QPointer<MediaSessionLifecycle> lifecycle_;
    QPointer<PlaybackCoordinator> transport_;
    QPointer<QObject> backend_;
    QPointer<QObject> pendingMedia_; ///< Media awaiting the matching VideoOutput GL context.
    QPointer<QObject> pendingSurface_; ///< Dynamic GLContext property is observed on the GUI thread.
    QUrl pendingSource_; ///< Cleared before every session release to reject stale loads.
    bool qt5_ = true; ///< Qt5 requires seek(int); Qt6 uses the position property.
    bool started_ = false; ///< Remains true through Pause/EOS until a session is replaced.
    bool blocked_ = false; ///< Prevent retries on media rejected by the current session.
};

#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>

/** @brief Queue transport/EOS work outside the media engine's current callback. */
class PlaybackCoordinator : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* backend READ backend WRITE setBackend NOTIFY backendChanged)
    Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(bool ready READ ready WRITE setReady NOTIFY readyChanged)
    Q_PROPERTY(bool atEnd READ atEnd WRITE setAtEnd NOTIFY atEndChanged)
    Q_PROPERTY(bool pendingPlay READ pendingPlay NOTIFY pendingPlayChanged)
public:
    explicit PlaybackCoordinator(QObject* parent = nullptr) : QObject(parent) {}
    QObject* backend() const { return backend_; }
    void setBackend(QObject* backend);
    QObject* controller() const { return controller_; }
    void setController(QObject* controller);
    bool ready() const { return ready_; }
    void setReady(bool ready);
    bool atEnd() const { return atEnd_; }
    void setAtEnd(bool atEnd);
    bool pendingPlay() const { return pendingPlay_; }

    /** @brief Retain Play until readiness and both source URLs match. */
    Q_INVOKABLE void requestPlay();
    /** @brief Invalidate queued Play and EOS callbacks on Pause/Stop/errors. */
    Q_INVOKABLE void cancel();
    /** @brief Consume Play only after actual playback confirmation. */
    Q_INVOKABLE void reportPlaying(bool playing);
signals:
    void backendChanged();
    void controllerChanged();
    void readyChanged();
    void atEndChanged();
    void pendingPlayChanged();
private:
    void schedulePlay(); ///< Recheck generation, object lifetime and URLs when queued work runs.
    void setPendingPlay(bool pending);
    QPointer<QObject> backend_; ///< QML owns the media object; never extend its lifetime.
    QPointer<QObject> controller_;
    QUrl requestedSource_; ///< Source captured by the latest Play intent.
    quint64 generation_ = 0;
    bool ready_ = false;
    bool atEnd_ = false;
    bool pendingPlay_ = false;
};

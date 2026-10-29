#pragma once

#include <QObject>
#include <QUrl>

/** @brief Own the inactive boundary between two QML media sessions. */
class MediaSessionLifecycle : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl desiredSource READ desiredSource WRITE setDesiredSource NOTIFY desiredSourceChanged)
    Q_PROPERTY(QUrl sessionSource READ sessionSource NOTIFY sessionChanged)
    Q_PROPERTY(bool active READ active NOTIFY sessionChanged)
    Q_PROPERTY(bool resetting READ resetting NOTIFY sessionChanged)
public:
    explicit MediaSessionLifecycle(QObject* parent = nullptr) : QObject(parent) {}
    QUrl desiredSource() const { return desiredSource_; } ///< Latest controller selection.
    void setDesiredSource(const QUrl& source);
    QUrl sessionSource() const { return sessionSource_; } ///< URL owned by the current renderer.
    bool active() const { return active_; } ///< Controls the QML session Loader.
    bool resetting() const { return resetting_; } ///< Suppresses callbacks during teardown.

    /** @brief Release now, then create only the latest session on the next event turn. */
    Q_INVOKABLE void reset();
signals:
    void desiredSourceChanged();
    void sessionChanged();
    void releaseRequested(); ///< Backend must stop before its renderer is destroyed.
private:
    QUrl desiredSource_; ///< Next URL, including an empty URL after USB removal.
    QUrl sessionSource_; ///< Empty while the old session is being released.
    quint64 generation_ = 0; ///< Invalidates obsolete queued recreation callbacks.
    bool active_ = false;
    bool resetting_ = false;
};

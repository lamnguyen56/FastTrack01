#pragma once
#include <QObject>
#include <QString>
#include <QTimer>

/** @brief Brief center-screen feedback for an explicit video-area Play/Pause click. */
class PlaybackClickFeedback : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool shown READ shown NOTIFY changed)
    Q_PROPERTY(QString iconName READ iconName NOTIFY changed)
public:
    /** @brief Construct feedback that expires without any recurring UI updates. */
    explicit PlaybackClickFeedback(QObject* parent = nullptr);
    bool shown() const { return timer_.isActive(); } ///< Whether feedback is visible.
    QString iconName() const { return iconName_; } ///< Play when paused, Pause when playing.

public slots:
    /** @brief Show the control icon matching the requested playback state for 650 ms. */
    void showForPlayback(bool playing);
    /** @brief Clear stale feedback when selection, storage or error state changes. */
    void dismiss();

signals:
    void changed(); ///< Refresh icon visibility or appearance.

private:
    QTimer timer_; ///< One expiry event per click; never polls the playback position.
    QString iconName_ = "play"; ///< Font-independent MediaIcon name.
};

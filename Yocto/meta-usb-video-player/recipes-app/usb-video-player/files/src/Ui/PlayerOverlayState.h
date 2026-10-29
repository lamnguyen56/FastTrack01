#pragma once
#include <QObject>
#include <QString>

/** @brief Derive empty/loading/error presentation state from application reports. */
class PlayerOverlayState : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool mounted READ mounted WRITE setMounted NOTIFY changed)
    Q_PROPERTY(int mediaCount READ mediaCount WRITE setMediaCount NOTIFY changed)
    Q_PROPERTY(QString errorText READ errorText WRITE setErrorText NOTIFY changed)
    Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY changed)
    Q_PROPERTY(bool playbackStarted READ playbackStarted WRITE setPlaybackStarted NOTIFY changed)
    Q_PROPERTY(bool visible READ visible NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool canPlay READ canPlay NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString detail READ detail NOTIFY changed)
public:
    explicit PlayerOverlayState(QObject* parent = nullptr) : QObject(parent) {}
    bool mounted() const { return mounted_; }
    void setMounted(bool value);
    int mediaCount() const { return mediaCount_; }
    void setMediaCount(int value);
    QString errorText() const { return errorText_; }
    void setErrorText(QString value);
    bool loading() const { return loading_; }
    void setLoading(bool value);
    bool playbackStarted() const { return playbackStarted_; }
    void setPlaybackStarted(bool value);
    bool visible() const;
    bool busy() const;
    bool canPlay() const;
    QString title() const;
    QString detail() const;
signals:
    void changed(); ///< Recompute QML bindings only when an input actually changes.
private:
    bool mounted_ = false; ///< Latest presentation input.
    int mediaCount_ = 0; ///< Latest presentation input.
    QString errorText_ = {}; ///< Latest presentation input.
    bool loading_ = false; ///< Latest presentation input.
    bool playbackStarted_ = false; ///< Latest presentation input.
};

#pragma once
#include <QObject>
#include <QTimer>

/** @brief Own nonmodal USB-removal feedback and its six-second lifetime. */
class UsbNoticeState : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)
public:
    explicit UsbNoticeState(QObject* parent = nullptr);
    bool shown() const { return timer_.isActive(); }
public slots:
    void show(); ///< Show/restart the notification after a disconnect.
    void dismiss(); ///< Dismiss on user action or reconnection, without touching storage.
signals:
    void shownChanged();
private:
    QTimer timer_; ///< Single-shot timer; all operations run on the GUI event thread.
};

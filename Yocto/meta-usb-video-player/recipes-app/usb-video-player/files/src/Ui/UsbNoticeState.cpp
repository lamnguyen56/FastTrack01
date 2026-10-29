#include "UsbNoticeState.h"

UsbNoticeState::UsbNoticeState(QObject* parent) : QObject(parent) {
    timer_.setSingleShot(true);
    timer_.setInterval(6000);
    connect(&timer_, &QTimer::timeout, this, &UsbNoticeState::shownChanged);
}

void UsbNoticeState::show() {
    const bool alreadyShown = shown();
    timer_.start();
    if (alreadyShown == false) {
        emit shownChanged();
    }
}

void UsbNoticeState::dismiss() {
    if (shown() == false) {
        return;
    }
    timer_.stop();
    emit shownChanged();
}

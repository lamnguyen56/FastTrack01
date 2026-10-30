#include "PlaybackClickFeedback.h"

PlaybackClickFeedback::PlaybackClickFeedback(QObject* parent) : QObject(parent) {
    timer_.setSingleShot(true);
    timer_.setInterval(650);
    connect(&timer_, &QTimer::timeout, this, &PlaybackClickFeedback::changed);
}

void PlaybackClickFeedback::showForPlayback(bool playing) {
    iconName_ = playing == true ? "pause" : "play";
    timer_.start(); // A repeated click replaces the icon and restarts its lifetime.
    emit changed();
}

void PlaybackClickFeedback::dismiss() {
    if (shown() == false) {
        return;
    }
    timer_.stop();
    emit changed();
}

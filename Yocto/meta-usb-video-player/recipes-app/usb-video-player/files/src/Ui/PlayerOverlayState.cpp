#include "PlayerOverlayState.h"

void PlayerOverlayState::setMounted(bool value) {
    if (mounted_ == value) {
        return;
    }
    mounted_ = value;
    emit changed();
}

void PlayerOverlayState::setMediaCount(int value) {
    if (mediaCount_ == value) {
        return;
    }
    mediaCount_ = value;
    emit changed();
}

void PlayerOverlayState::setErrorText(QString value) {
    if (errorText_ == value) {
        return;
    }
    errorText_ = value;
    emit changed();
}

void PlayerOverlayState::setLoading(bool value) {
    if (loading_ == value) {
        return;
    }
    loading_ = value;
    emit changed();
}

void PlayerOverlayState::setPlaybackStarted(bool value) {
    if (playbackStarted_ == value) {
        return;
    }
    playbackStarted_ = value;
    emit changed();
}

bool PlayerOverlayState::visible() const {
    return mounted_ == false || errorText_.isEmpty() == false || mediaCount_ == 0 || playbackStarted_ == false;
}

bool PlayerOverlayState::busy() const {
    return mounted_ == true && mediaCount_ > 0 && errorText_.isEmpty() == true
        && loading_ == true && playbackStarted_ == false;
}

bool PlayerOverlayState::canPlay() const {
    return mounted_ == true && mediaCount_ > 0 && errorText_.isEmpty() == true && busy() == false;
}

QString PlayerOverlayState::title() const {
    if (mounted_ == false) {
        return tr("NO USB CONNECTED");
    }
    if (errorText_.isEmpty() == false) {
        return tr("PLAYBACK ERROR");
    }
    if (mediaCount_ == 0) {
        return tr("NO MEDIA");
    }
    return busy() == true ? tr("LOADING VIDEO") : tr("READY TO PLAY");
}

QString PlayerOverlayState::detail() const {
    if (mounted_ == false) {
        return tr("Insert a USB drive containing videos");
    }
    if (errorText_.isEmpty() == false) {
        return errorText_;
    }
    if (mediaCount_ == 0) {
        return tr("Waiting for playlist / no supported media files");
    }
    return busy() == true ? tr("Preparing playback...") : tr("Select a video from the playlist or press Play");
}

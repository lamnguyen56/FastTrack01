#include "PlayerController.h"
#include <QFileInfo>
#include <algorithm>
#include "Logger.h"

PlayerController::PlayerController(UsbPlaylistModel& model, QObject* parent)
    : QObject(parent), model_(model) {
    connect(&model_, &UsbPlaylistModel::playlistChanged, this, &PlayerController::synchronizePlaylist);
    synchronizePlaylist();
}

QUrl PlayerController::source() const { return source_; }

int PlayerController::currentIndex() const { return currentIndex_; }

QString PlayerController::currentTitle() const { return QFileInfo(source_.toLocalFile()).fileName(); }

qint64 PlayerController::position() const { return position_; }

qint64 PlayerController::duration() const { return duration_; }

bool PlayerController::isPlaying() const { return playing_; }

bool PlayerController::isSeekable() const { return seekable_; }

bool PlayerController::autoAdvance() const { return autoAdvance_; }

bool PlayerController::isFinished() const { return finished_; }

void PlayerController::setAutoAdvance(bool enabled) {
    if (autoAdvance_ == enabled) {
        return;
    }
    autoAdvance_ = enabled;
    emit autoAdvanceChanged();
    LOG_I("Auto advance: ", enabled ? "enabled" : "disabled");
}

void PlayerController::handleEndOfMedia() {
    if (source_.isEmpty() == true || finished_ == true) {
        return;
    }

    // Keep the source loaded: stopping/clearing here would discard the final image.
    finished_ = true;
    playing_ = false;
    emit playbackChanged();
    LOG_I("Track ended; auto advance=", autoAdvance_);
    if (autoAdvance_ == true) {
        next();
    }
}

QString PlayerController::errorText() const { return errorText_; }

void PlayerController::synchronizePlaylist() {
    // Preserve selection by file path when a playlist refresh changes row indices.
    const int preserved = model_.indexOf(source_.toLocalFile());
    if (preserved >= 0 && source_.isEmpty() == false) {
        currentIndex_ = preserved;
        emit selectionChanged();
        return;
    }
    setSelection(model_.count() > 0 ? 0 : -1);
}

void PlayerController::setSelection(int index) {
    const QString path = model_.pathAt(index);
    const QUrl nextSource = path.isEmpty() == true ? QUrl() : QUrl::fromLocalFile(path);
    currentIndex_ = path.isEmpty() == true ? -1 : index;
    if (nextSource != source_) {
        // Stop before clearing the URL so unplug also releases open media handles.
        emit stopRequested();
        source_ = nextSource;
        position_ = 0;
        duration_ = 0;
        finished_ = false;
        playing_ = false;
        seekable_ = false;
        errorText_.clear();
        emit sourceChanged();
        emit timelineChanged();
        emit playbackChanged();
        emit errorChanged();
        LOG_I("Player selected: ", path.toStdString());
    }
    emit selectionChanged();
}

void PlayerController::selectTrack(int index) {
    if (index < 0 || index >= model_.count()) {
        return;
    }
    // Selecting the current row still requests a stop/reopen, even though its
    // unchanged URL will not emit sourceChanged in setSelection().
    if (index == currentIndex_) {
        emit stopRequested();
    }
    setSelection(index);
    play();
}

void PlayerController::play() {
    if (source_.isEmpty() == true) {
        return;
    }
    // An explicit Play after completion replays the selected file from the start.
    if (finished_ == true) {
        finished_ = false;
        emit seekRequested(0);
        emit playbackChanged();
    }
    emit playRequested();
}

void PlayerController::pause() {
    emit pauseRequested();
}

void PlayerController::toggle() {
    if (playing_ == true) {
        pause();
    } else {
        play();
    }
}

void PlayerController::next() {
    if (model_.count() > 0) {
        selectTrack((currentIndex_ + 1) % model_.count());
    }
}

void PlayerController::previous() {
    if (model_.count() > 0) {
        selectTrack(currentIndex_ <= 0 ? model_.count() - 1 : currentIndex_ - 1);
    }
}

void PlayerController::seekRelative(int seconds) {
    seekTo(position_ + static_cast<qint64>(seconds) * 1000);
}

void PlayerController::seekTo(qint64 milliseconds) {
    if (seekable_ == false || duration_ <= 0) {
        return;
    }
    emit seekRequested(std::clamp(milliseconds, qint64(0), duration_));
}

void PlayerController::reportPosition(qint64 milliseconds) {
    position_ = std::max(qint64(0), milliseconds);
    emit timelineChanged();
}

void PlayerController::reportDuration(qint64 milliseconds) {
    duration_ = std::max(qint64(0), milliseconds);
    emit timelineChanged();
}

void PlayerController::reportPlayback(bool playing, bool seekable) {
    // Keep failures visible until the backend actually reports successful playback.
    if (playing == true && errorText_.isEmpty() == false) {
        errorText_.clear();
        emit errorChanged();
    }
    if (playing_ != playing || seekable_ != seekable) {
        playing_ = playing;
        seekable_ = seekable;
        emit playbackChanged();
    }
}

void PlayerController::reportError(const QString& message) {
    errorText_ = message;
    emit errorChanged();
    LOG_E("Media playback error: ", message.toStdString());
}

QString PlayerController::formatTime(qint64 milliseconds) const {
    const qint64 seconds = std::max(qint64(0), milliseconds) / 1000;
    const QString minutesAndSeconds = QStringLiteral("%1:%2")
        .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
    if (seconds >= 3600) {
        return QString::number(seconds / 3600) + QLatin1Char(':') + minutesAndSeconds;
    }
    return minutesAndSeconds;
}

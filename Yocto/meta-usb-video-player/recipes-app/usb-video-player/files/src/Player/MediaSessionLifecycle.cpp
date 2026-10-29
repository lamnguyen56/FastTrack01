#include "MediaSessionLifecycle.h"
#include <QTimer>
#include "Logger.h"

void MediaSessionLifecycle::setDesiredSource(const QUrl& source) {
    if (desiredSource_ == source) {
        return;
    }
    desiredSource_ = source;
    emit desiredSourceChanged();
}

void MediaSessionLifecycle::reset() {
    const quint64 ticket = ++generation_;
    resetting_ = true;
    emit sessionChanged();
    emit releaseRequested();
    active_ = false;
    sessionSource_.clear();
    emit sessionChanged();

    // A context-bound callback cannot run after this QObject is destroyed.
    QTimer::singleShot(0, this, [this, ticket]() {
        if (ticket != generation_) {
            return;
        }
        sessionSource_ = desiredSource_;
        active_ = sessionSource_.isEmpty() == false;
        // Notify while still resetting: QML attaches VideoOutput before assigning source.
        emit sessionChanged();
        resetting_ = false;
        emit sessionChanged();
        LOG_I("[MediaSession] new session: ", sessionSource_.toString().toStdString());
    });
}

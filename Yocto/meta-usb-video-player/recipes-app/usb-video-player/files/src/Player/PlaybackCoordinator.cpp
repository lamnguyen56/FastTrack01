#include "PlaybackCoordinator.h"
#include <QTimer>
#include <QVariant>

namespace {
QUrl sourceOf(const QObject* object) {
    return object != nullptr ? object->property("source").toUrl() : QUrl();
}
}

void PlaybackCoordinator::setBackend(QObject* backend) {
    if (backend_ == backend) {
        return;
    }
    backend_ = backend;
    emit backendChanged();
}

void PlaybackCoordinator::setController(QObject* controller) {
    if (controller_ == controller) {
        return;
    }
    cancel();
    controller_ = controller;
    emit controllerChanged();
}

void PlaybackCoordinator::setPendingPlay(bool pending) {
    if (pendingPlay_ == pending) {
        return;
    }
    pendingPlay_ = pending;
    emit pendingPlayChanged();
}

void PlaybackCoordinator::requestPlay() {
    ++generation_;
    requestedSource_ = sourceOf(controller_);
    setPendingPlay(requestedSource_.isEmpty() == false);
    schedulePlay();
}

void PlaybackCoordinator::schedulePlay() {
    const quint64 ticket = generation_;
    QTimer::singleShot(0, this, [this, ticket]() {
        if (ticket == generation_ && pendingPlay_ == true && ready_ == true
                && backend_.isNull() == false && controller_.isNull() == false
                && sourceOf(backend_) == requestedSource_ && sourceOf(controller_) == requestedSource_) {
            // QML-owned Qt5/Qt6 MediaPlayer exposes play through its meta-object.
            QMetaObject::invokeMethod(backend_, "play", Qt::DirectConnection);
        }
    });
}

void PlaybackCoordinator::cancel() {
    ++generation_;
    setPendingPlay(false);
}

void PlaybackCoordinator::reportPlaying(bool playing) {
    if (playing == true) {
        setPendingPlay(false);
    }
}

void PlaybackCoordinator::setReady(bool ready) {
    if (ready_ == ready) {
        return;
    }
    ready_ = ready;
    emit readyChanged();
    if (ready_ == true && pendingPlay_ == true) {
        schedulePlay();
    }
}

void PlaybackCoordinator::setAtEnd(bool atEnd) {
    if (atEnd_ == atEnd) {
        return;
    }
    atEnd_ = atEnd;
    emit atEndChanged();
    if (atEnd_ == false || backend_.isNull() == true) {
        return;
    }
    const QUrl completedSource = sourceOf(backend_);
    const quint64 ticket = generation_;
    QTimer::singleShot(0, this, [this, completedSource, ticket]() {
        if (ticket == generation_ && atEnd_ == true && controller_.isNull() == false
                && sourceOf(controller_) == completedSource) {
            QMetaObject::invokeMethod(controller_, "handleEndOfMedia", Qt::DirectConnection);
        }
    });
}

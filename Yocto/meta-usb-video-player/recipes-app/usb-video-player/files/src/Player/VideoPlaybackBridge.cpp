#include "VideoPlaybackBridge.h"
#include <QSize>
#include <QVariant>
#include "Logger.h"

void VideoPlaybackBridge::setController(PlayerController* controller) {
    if (controller_ == controller) {
        return;
    }
    if (controller_.isNull() == false) {
        disconnect(controller_, nullptr, this, nullptr);
    }
    controller_ = controller;
    if (controller_.isNull() == false) {
        connect(controller_, &PlayerController::sourceChanged, this, [this]() {
            if (lifecycle_.isNull() == false) {
                lifecycle_->reset();
            }
        });
        connect(controller_, &PlayerController::stopRequested, this, [this]() {
            if (transport_.isNull() == false) {
                transport_->cancel();
            }
            if (lifecycle_.isNull() == false) {
                lifecycle_->reset();
            }
        });
        connect(controller_, &PlayerController::playRequested, this, [this]() {
            if (blocked_ == false && transport_.isNull() == false) {
                transport_->requestPlay();
            }
        });
        connect(controller_, &PlayerController::pauseRequested, this, [this]() {
            if (transport_.isNull() == false) {
                transport_->cancel();
            }
            if (backend_.isNull() == false) {
                QMetaObject::invokeMethod(backend_, "pause");
            }
        });
        connect(controller_, &PlayerController::seekRequested, this, &VideoPlaybackBridge::seek);
    }
    emit controllerChanged();
}

void VideoPlaybackBridge::setLifecycle(MediaSessionLifecycle* lifecycle) {
    if (lifecycle_ == lifecycle) {
        return;
    }
    if (lifecycle_.isNull() == false) {
        disconnect(lifecycle_, nullptr, this, nullptr);
    }
    lifecycle_ = lifecycle;
    if (lifecycle_.isNull() == false) {
        connect(lifecycle_, &MediaSessionLifecycle::releaseRequested, this, &VideoPlaybackBridge::releaseBackend);
        connect(lifecycle_, &MediaSessionLifecycle::sessionChanged, this, &VideoPlaybackBridge::checkConstructionError);
    }
    emit lifecycleChanged();
}

void VideoPlaybackBridge::setTransport(PlaybackCoordinator* transport) {
    if (transport_ == transport) {
        return;
    }
    transport_ = transport;
    emit transportChanged();
}

void VideoPlaybackBridge::setBackend(QObject* backend) {
    if (backend_ == backend) {
        return;
    }
    backend_ = backend;
    emit backendChanged();
}

void VideoPlaybackBridge::setQt5(bool qt5) {
    if (qt5_ == qt5) {
        return;
    }
    qt5_ = qt5;
    emit qt5Changed();
}

bool VideoPlaybackBridge::current() const {
    return lifecycle_.isNull() == false && controller_.isNull() == false
        && lifecycle_->active() == true && lifecycle_->resetting() == false
        && lifecycle_->sessionSource() == controller_->source();
}

void VideoPlaybackBridge::stopBackend() {
    if (backend_.isNull() == false) {
        QMetaObject::invokeMethod(backend_, "stop");
    }
}

void VideoPlaybackBridge::releaseBackend() {
    // Lifecycle already marked itself resetting: stop callbacks must not update the controller.
    stopBackend();
    if (backend_.isNull() == false) {
        backend_->setProperty("source", QUrl());
    }
    started_ = false;
    blocked_ = false;
    emit stateChanged();
}

void VideoPlaybackBridge::initialize() {
    if (lifecycle_.isNull() == false) {
        lifecycle_->reset();
    }
}

void VideoPlaybackBridge::reportPosition(qint64 position) {
    if (current() == true && blocked_ == false) {
        controller_->reportPosition(position);
    }
}

void VideoPlaybackBridge::reportDuration(qint64 duration) {
    if (current() == true && blocked_ == false) {
        controller_->reportDuration(duration);
    }
}

void VideoPlaybackBridge::reportVideoRect(const QRectF& rect) {
    if (current() == true && rect.width() > 0 && rect.height() > 0) {
        LOG_I("[MediaSession] video output format: ", rect.width(), " x ", rect.height());
    }
}

void VideoPlaybackBridge::reportPlayback(bool playing, bool seekable, qint64 duration) {
    if (blocked_ == true && playing == true) {
        stopBackend();
        return;
    }
    if (current() == false || blocked_ == true) {
        return;
    }
    if (playing == true && started_ == false) {
        started_ = true;
        emit stateChanged();
        controller_->reportDuration(duration);
        LOG_I("[MediaSession] playing: ", controller_->source().toString().toStdString());
    }
    if (transport_.isNull() == false) {
        transport_->reportPlaying(playing);
    }
    controller_->reportPlayback(playing, seekable);
}

void VideoPlaybackBridge::seek(qint64 position) {
    if (current() == false || backend_.isNull() == true) {
        return;
    }
    if (qt5_ == true) {
        QMetaObject::invokeMethod(backend_, "seek", Q_ARG(int, static_cast<int>(position)));
    } else {
        backend_->setProperty("position", position);
    }
}

void VideoPlaybackBridge::reportError(bool formatFailure, const QString& detail) {
    if (current() == false || backend_.isNull() == true || blocked_ == true) {
        return;
    }
    QString message = detail;
    if (qt5_ == true && formatFailure == true) {
        const QObject* metadata = backend_->property("metaData").value<QObject*>();
        const QSize size = metadata != nullptr ? metadata->property("resolution").toSize() : QSize();
        const QString dimensions = size.width() > 0 && size.height() > 0
            ? QStringLiteral(" (%1 x %2)").arg(size.width()).arg(size.height()) : QString();
        message = tr("Unsupported resolution or video format%1. Hardware H.264 decoding is required. Use H.264 720p.")
            .arg(dimensions);
    }
    // Block before stopping: stop() can synchronously emit more backend callbacks.
    blocked_ = true;
    emit stateChanged();
    if (transport_.isNull() == false) {
        transport_->cancel();
    }
    stopBackend();
    controller_->reportPlayback(false, false);
    controller_->reportError(message);
}

void VideoPlaybackBridge::reportMediaError(int error, const QString& detail) {
    if (error == 0) {
        return;
    }
    // FormatError is 2 in the Qt5 and Qt6 MediaPlayer API exposed by these adapters.
    reportError(error == 2, detail);
}

void VideoPlaybackBridge::checkConstructionError() {
    if (current() == false || backend_.isNull() == true) {
        return;
    }
    if (backend_->property("error").toInt() != 0) {
        reportMediaError(backend_->property("error").toInt(),
                         backend_->property("errorString").toString());
    }
}

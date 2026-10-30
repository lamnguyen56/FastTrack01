#include "VideoPlaybackBridge.h"
#include <QSize>
#include <QDynamicPropertyChangeEvent>
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
        connect(lifecycle_, &MediaSessionLifecycle::sessionChanged, this, &VideoPlaybackBridge::tryPendingLoad);
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
    cancelPendingLoad();
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

void VideoPlaybackBridge::prepareMedia(QObject* media, QObject* surface, const QUrl& source) {
    cancelPendingLoad();
    if (media == nullptr) {
        return;
    }
    if (qEnvironmentVariableIntValue("QT_GSTREAMER_USE_OPENGL_PLUGIN") == 0) {
        media->setProperty("source", source); // Preserve the normal non-GL loading path.
        return;
    }
    if (surface == nullptr) {
        LOG_E("GL video loading requires a VideoOutput surface");
        return;
    }
    pendingMedia_ = media;
    pendingSurface_ = surface;
    pendingSource_ = source;
    surface->installEventFilter(this);
    // VideoOutput publishes GLContext after its first scene-graph update. Never
    // substitute a fixed delay: slow boots and rapid selections must remain safe.
    tryPendingLoad();
}

bool VideoPlaybackBridge::eventFilter(QObject* watched, QEvent* event) {
    if (watched == pendingSurface_ && event->type() == QEvent::DynamicPropertyChange) {
        auto* change = static_cast<QDynamicPropertyChangeEvent*>(event);
        if (change->propertyName() == "GLContext") {
            tryPendingLoad();
        }
    }
    return QObject::eventFilter(watched, event);
}

void VideoPlaybackBridge::tryPendingLoad() {
    if (pendingMedia_.isNull() == true || pendingSurface_.isNull() == true) {
        return;
    }
    if (controller_.isNull() == false && controller_->source() != pendingSource_) {
        cancelPendingLoad();
        return;
    }
    if (lifecycle_.isNull() == false
            && (lifecycle_->active() == false || lifecycle_->resetting() == true
                || lifecycle_->sessionSource() != pendingSource_)) {
        return;
    }
    if (pendingSurface_->property("GLContext").value<QObject*>() == nullptr) {
        return;
    }
    QPointer<QObject> media = pendingMedia_;
    const QUrl source = pendingSource_;
    cancelPendingLoad();
    LOG_I("VideoOutput GL context ready; loading media");
    media->setProperty("source", source);
}

void VideoPlaybackBridge::cancelPendingLoad() {
    if (pendingSurface_.isNull() == false) {
        pendingSurface_->removeEventFilter(this);
    }
    pendingSurface_.clear();
    pendingMedia_.clear();
    pendingSource_.clear();
}

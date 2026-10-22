#include "PlayerApplication.h"
#include <QGuiApplication>
#include <QQmlContext>
#include "Constants.h"
#include "Logger.h"
#include "Player/HardwareDecodePolicy.h"

PlayerApplication::PlayerApplication()
    : usb_(playlist_, QString::fromUtf8(FastTrack::DEFAULT_USB_PLAYLIST_PATH),
           QString::fromUtf8(FastTrack::DEFAULT_USB_MOUNT_POINT)), controller_(playlist_) {
    // CAN commands and UI controls share one controller and the same selection state.
    QObject::connect(&can_, &CanIpcClient::commandPlay, &controller_, &PlayerController::play);
    QObject::connect(&can_, &CanIpcClient::commandPause, &controller_, &PlayerController::pause);
    QObject::connect(&can_, &CanIpcClient::commandNext, &controller_, &PlayerController::next);
    QObject::connect(&can_, &CanIpcClient::commandPrev, &controller_, &PlayerController::previous);
    QObject::connect(&can_, &CanIpcClient::commandSeek, &controller_, &PlayerController::seekRelative);

    // Context objects are members and outlive engine_: QML keeps non-owning access.
    engine_.rootContext()->setContextProperty("usbPlaylist", &playlist_);
    engine_.rootContext()->setContextProperty("usbReader", &usb_);
    engine_.rootContext()->setContextProperty("playerController", &controller_);
    engine_.rootContext()->setContextProperty("canClient", &can_);
    engine_.rootContext()->setContextProperty("qtMajorVersion", QT_VERSION_MAJOR);
}

int PlayerApplication::run() {
    // The first snapshot is loaded before QML; selected media does not autoplay.
    HardwareDecodePolicy::apply();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (media_.initialize() == false) {
        return 1;
    }
    engine_.rootContext()->setContextProperty("gstPlayer", &media_);
#endif
    usb_.start();
    engine_.load(QUrl(QStringLiteral("qrc:/ui/main.qml")));
    if (engine_.rootObjects().isEmpty() == true) {
        LOG_E("Cannot load the player QML component");
        return 1;
    }
    LOG_I("USB video player UI started");
    const int result = QGuiApplication::exec();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    media_.detach();
#endif
    return result;
}

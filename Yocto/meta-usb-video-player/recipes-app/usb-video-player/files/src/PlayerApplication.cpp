#include "PlayerApplication.h"
#include <QGuiApplication>
#include <QQmlContext>
#include "Constants.h"
#include "Logger.h"
#include "Player/HardwareDecodePolicy.h"
#include "Ui/QmlTypes.h"

PlayerApplication::PlayerApplication()
    : usb_(playlist_, QString::fromUtf8(FastTrack::DEFAULT_USB_PLAYLIST_PATH),
           QString::fromUtf8(FastTrack::DEFAULT_USB_MOUNT_POINT)), controller_(playlist_) {
    QObject::connect(&can_, &CanIpcClient::commandPlay, &controller_, &PlayerController::play);
    QObject::connect(&can_, &CanIpcClient::commandPause, &controller_, &PlayerController::pause);
    QObject::connect(&can_, &CanIpcClient::commandNext, &controller_, &PlayerController::next);
    QObject::connect(&can_, &CanIpcClient::commandPrev, &controller_, &PlayerController::previous);
    QObject::connect(&can_, &CanIpcClient::commandSeek, &controller_, &PlayerController::seekRelative);

    registerPlayerQmlTypes();
    QObject::connect(&usb_, &UsbPlaylistReader::disconnected, &notice_, &UsbNoticeState::show);
    QObject::connect(&usb_, &UsbPlaylistReader::mountedChanged, &notice_, [this]() {
        if (usb_.isMounted() == true) { notice_.dismiss(); }
    });
    engine_.rootContext()->setContextProperty("usbNoticeState", &notice_);
    engine_.rootContext()->setContextProperty("usbPlaylist", &playlist_);
    engine_.rootContext()->setContextProperty("usbReader", &usb_);
    engine_.rootContext()->setContextProperty("playerController", &controller_);
    engine_.rootContext()->setContextProperty("canClient", &can_);
    engine_.rootContext()->setContextProperty("qtMajorVersion", QT_VERSION_MAJOR);
}

int PlayerApplication::run() {
    // The first snapshot is loaded before QML; selected media does not autoplay.
    HardwareDecodePolicy::apply();
    usb_.start();
    engine_.load(QUrl(QStringLiteral("qrc:/ui/main.qml")));
    if (engine_.rootObjects().isEmpty() == true) {
        LOG_E("Cannot load the player QML component");
        return 1;
    }
    LOG_I("USB video player UI started");
    return QGuiApplication::exec();
}

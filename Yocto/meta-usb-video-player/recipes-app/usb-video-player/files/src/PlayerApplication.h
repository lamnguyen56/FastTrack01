#pragma once

#include <QQmlApplicationEngine>
#include "CanClient/CanIpcClient.h"
#include "Player/PlayerController.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include "Player/GstPlayerBackend.h"
#endif
#include "Usb/UsbPlaylistReader.h"

/** @brief Composes USB data, transport control, CAN input and the QML view. */
class PlayerApplication {
public:
    /** @brief Construct modules in dependency lifetime order and wire commands. */
    PlayerApplication();

    /** @brief Load the QML view, then enter the Qt application event loop. */
    int run();

private:
    UsbPlaylistModel playlist_; ///< Canonical model shared by controller and UI.
    UsbPlaylistReader usb_; ///< Playlist/mount observer; never mounts devices itself.
    PlayerController controller_; ///< Single transport entry point for UI and CAN.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    GstPlayerBackend media_{controller_}; ///< Direct pipeline, destroyed after the QML engine.
#endif
    CanIpcClient can_; ///< Existing nonblocking local CAN command receiver.
    QQmlApplicationEngine engine_; ///< Destroyed before its exposed context objects.
};

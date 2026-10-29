#pragma once

#include <QQmlApplicationEngine>
#include "CanClient/CanIpcClient.h"
#include "Player/PlayerController.h"
#include "Usb/UsbPlaylistReader.h"
#include "Ui/UsbNoticeState.h"

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
    CanIpcClient can_; ///< Existing nonblocking local CAN command receiver.
    UsbNoticeState notice_; ///< C++ notification timer and dismissal state.
    QQmlApplicationEngine engine_; ///< Destroyed before its exposed context objects.
};

#include "QmlTypes.h"
#include <qqml.h>
#include "Player/MediaSessionLifecycle.h"
#include "Player/PlaybackCoordinator.h"
#include "Player/VideoPlaybackBridge.h"
#include "PlayerOverlayState.h"

void registerPlayerQmlTypes() {
    qmlRegisterUncreatableType<PlayerController>("UsbPlayer", 1, 0, "PlayerController",
                                               "PlayerApplication owns the controller");
    qmlRegisterType<MediaSessionLifecycle>("UsbPlayer", 1, 0, "MediaSessionLifecycle");
    qmlRegisterType<PlaybackCoordinator>("UsbPlayer", 1, 0, "PlaybackCoordinator");
    qmlRegisterType<VideoPlaybackBridge>("UsbPlayer", 1, 0, "VideoPlaybackBridge");
    qmlRegisterType<PlayerOverlayState>("UsbPlayer", 1, 0, "PlayerOverlayState");
}

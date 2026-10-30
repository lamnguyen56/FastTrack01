# Shared source list for the application and headless QtCore integration tests.
add_library(player-core STATIC
    ${CMAKE_CURRENT_LIST_DIR}/Player/MediaSessionLifecycle.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Player/MediaSessionLifecycle.h
    ${CMAKE_CURRENT_LIST_DIR}/Player/PlaybackCoordinator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Player/PlaybackCoordinator.h
    ${CMAKE_CURRENT_LIST_DIR}/Player/VideoPlaybackBridge.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Player/VideoPlaybackBridge.h
    ${CMAKE_CURRENT_LIST_DIR}/Ui/PlaybackClickFeedback.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Ui/PlaybackClickFeedback.h
    ${CMAKE_CURRENT_LIST_DIR}/Ui/PlayerOverlayState.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Ui/PlayerOverlayState.h
    ${CMAKE_CURRENT_LIST_DIR}/Ui/UsbNoticeState.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Ui/UsbNoticeState.h
    ${CMAKE_CURRENT_LIST_DIR}/Ui/QmlCache.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Ui/QmlCache.h
    ${CMAKE_CURRENT_LIST_DIR}/Usb/UsbPlaylistModel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Usb/UsbPlaylistModel.h
    ${CMAKE_CURRENT_LIST_DIR}/Usb/UsbPlaylistReader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Usb/UsbPlaylistReader.h
    ${CMAKE_CURRENT_LIST_DIR}/Player/PlayerController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Player/PlayerController.h
)
set_target_properties(player-core PROPERTIES AUTOMOC ON)
target_include_directories(player-core PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(player-core PUBLIC Qt${QT_VERSION_MAJOR}::Core Common::Headers)

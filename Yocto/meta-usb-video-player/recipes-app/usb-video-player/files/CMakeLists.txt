cmake_minimum_required(VERSION 3.16)
project(usb-video-player VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Standalone builds use the same canonical headers as the Yocto sysroot package.
if(NOT TARGET Common::Headers)
    set(COMMON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../../recipes-common/common/files")
    if(EXISTS "${COMMON_SOURCE}/CMakeLists.txt")
        add_subdirectory("${COMMON_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/common")
    else()
        find_package(Common CONFIG REQUIRED)
    endif()
endif()

# Qt5 is used on the Yocto target; Qt6 remains available for desktop development.
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Qml Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Qml Quick REQUIRED)

include(src/PlayerCore.cmake)

add_executable(${PROJECT_NAME}
    src/Player/HardwareDecodePolicy.cpp
    src/Player/HardwareDecodePolicy.h
    src/main.cpp
    src/PlayerApplication.cpp
    src/PlayerApplication.h
    src/CanClient/CanIpcClient.cpp
    src/CanClient/CanIpcClient.h
    qml/qml.qrc
)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE
    player-core
    Common::Headers
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
)
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS Multimedia REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Multimedia)
endif()
if(QT_VERSION_MAJOR EQUAL 5)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
    target_sources(${PROJECT_NAME} PRIVATE src/Player/GstPlayerBackend.cpp src/Player/GstPlayerBackend.h
        src/Player/GstVideoMonitor.cpp src/Player/GstVideoMonitor.h)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GSTREAMER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USB_REQUIRE_HARDWARE_H264=1)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

cmake_minimum_required(VERSION 3.16)
project(ubs-video-player VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Shared Constants.h:
# 1. Từ thư mục cục bộ của App team (common/Constants.h)
# 2. Hoặc từ Platform layer (meta-ubs-video-player/recipes-platform/common/files)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../meta-ubs-video-player/recipes-platform/common/files
)

# Hỗ trợ linh hoạt cả Qt6 (trên Windows Dev) và Qt5 (trên Yocto Linux Target)
# Giữ nguyên convention từ References/App/CMakeLists.txt
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Qml Quick Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Qml Quick Multimedia REQUIRED)

include_directories(
    src
    src/Core
    src/CanClient
    src/UsbScanner
    src/PlayerEngine
)

set(SOURCES
    src/main.cpp
    src/CanClient/CanIpcClient.cpp
    src/CanClient/CanIpcClient.h
)

set(RESOURCES
    qml/qml.qrc
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Multimedia
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

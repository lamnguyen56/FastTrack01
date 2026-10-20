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
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Qml Quick Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Qml Quick Multimedia REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/CanClient/CanIpcClient.cpp
    src/CanClient/CanIpcClient.h
    qml/qml.qrc
)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Common::Headers
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Multimedia
)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

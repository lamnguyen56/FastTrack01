cmake_minimum_required(VERSION 3.16)
project(fasttrack-video-player VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Hỗ trợ linh hoạt cả Qt6 (trên Windows Dev) và Qt5 (trên Yocto Linux Target)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Qml Quick Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Qml Quick Multimedia REQUIRED)

# SerialBus (CAN Bus) - Tùy chọn trên Windows Dev, tự động bật khi có module
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS SerialBus QUIET)

# Check for libudev on Linux (không bắt buộc trên Windows)
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED libudev)
    add_definitions(-DHAVE_LIBUDEV)
endif()

include_directories(
    src
    src/Core
    src/CanService
    src/UsbStorage
    src/PlayerEngine
    ${UDEV_INCLUDE_DIRS}
)

set(SOURCES
    src/main.cpp
    # Add other C++ implementation files as development progresses
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

if(TARGET Qt${QT_VERSION_MAJOR}::SerialBus)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::SerialBus)
    add_definitions(-DHAVE_QT_SERIALBUS)
endif()

if(UDEV_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${UDEV_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

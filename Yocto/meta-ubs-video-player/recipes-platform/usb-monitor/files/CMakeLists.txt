cmake_minimum_required(VERSION 3.16)
project(usb-monitor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
find_package(Threads REQUIRED)

add_executable(usb-monitor
    src/main.cpp
    src/UsbApplication.cpp
    src/HotplugMonitor.cpp
    src/StorageController.cpp
    src/MountManager.cpp
    src/MediaScanner.cpp
    src/PlaylistWriter.cpp
)
target_include_directories(usb-monitor PRIVATE
    src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/files
)
target_link_libraries(usb-monitor PRIVATE PkgConfig::UDEV Threads::Threads)
install(TARGETS usb-monitor DESTINATION bin)

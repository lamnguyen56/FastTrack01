cmake_minimum_required(VERSION 3.16)
project(usb-monitor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
find_package(Threads REQUIRED)

# Use the installed sysroot package, or the canonical source for host builds.
if(NOT TARGET Common::Headers)
    set(COMMON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../../recipes-common/common/files")
    if(EXISTS "${COMMON_SOURCE}/CMakeLists.txt")
        add_subdirectory("${COMMON_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/common")
    else()
        find_package(Common CONFIG REQUIRED)
    endif()
endif()

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
)
target_link_libraries(usb-monitor PRIVATE Common::Headers PkgConfig::UDEV Threads::Threads)
install(TARGETS usb-monitor DESTINATION bin)

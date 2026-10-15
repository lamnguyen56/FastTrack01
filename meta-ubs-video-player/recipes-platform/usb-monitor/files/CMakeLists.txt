cmake_minimum_required(VERSION 3.16)
project(usb-monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common headers:
# 1. ${CMAKE_CURRENT_SOURCE_DIR} (khi Yocto unpack Constants.h vào WORKDIR)
# 2. ${CMAKE_CURRENT_SOURCE_DIR}/../../common/files (khi build thủ công trên dev machine)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/files
)

# libudev là thư viện Linux chuẩn, có sẵn trong mọi bản Yocto
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED libudev)

add_executable(usb-monitor
    src/main.cpp
)

target_include_directories(usb-monitor PRIVATE ${UDEV_INCLUDE_DIRS})
target_link_libraries(usb-monitor PRIVATE ${UDEV_LIBRARIES} pthread)

install(TARGETS usb-monitor DESTINATION bin)

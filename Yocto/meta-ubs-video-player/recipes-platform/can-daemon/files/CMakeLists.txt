cmake_minimum_required(VERSION 3.16)
project(can-daemon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common headers:
# 1. ${CMAKE_CURRENT_SOURCE_DIR} (khi Yocto unpack Constants.h vào WORKDIR)
# 2. ${CMAKE_CURRENT_SOURCE_DIR}/../../common/files (khi build thủ công trên dev machine)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/files
)

add_executable(can-daemon
    src/main.cpp
)

target_link_libraries(can-daemon PRIVATE pthread)

install(TARGETS can-daemon DESTINATION bin)

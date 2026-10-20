cmake_minimum_required(VERSION 3.16)
project(can-daemon LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

add_executable(can-daemon
    src/main.cpp
    src/CanApplication.cpp
    src/SocketCan.cpp
    src/IpcChannel.cpp
    src/ProtocolCodec.cpp
)
target_include_directories(can-daemon PRIVATE
    src
)
target_link_libraries(can-daemon PRIVATE Common::Headers Threads::Threads)
install(TARGETS can-daemon DESTINATION bin)

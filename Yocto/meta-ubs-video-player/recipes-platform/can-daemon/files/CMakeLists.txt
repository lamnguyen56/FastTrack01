cmake_minimum_required(VERSION 3.16)
project(can-daemon LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)

add_executable(can-daemon
    src/main.cpp
    src/CanApplication.cpp
    src/SocketCan.cpp
    src/IpcChannel.cpp
    src/ProtocolCodec.cpp
)
target_include_directories(can-daemon PRIVATE
    src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/files
)
target_link_libraries(can-daemon PRIVATE Threads::Threads)
install(TARGETS can-daemon DESTINATION bin)

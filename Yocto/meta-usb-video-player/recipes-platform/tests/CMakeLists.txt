cmake_minimum_required(VERSION 3.16)
project(platform-host-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(../../recipes-common/common/files common)

# Build the actual daemons too, so omitted sources/link dependencies are caught.
add_subdirectory(../usb-monitor/files usb-monitor)
add_subdirectory(../can-daemon/files can-daemon)

add_executable(usb-scan-test
    ../usb-monitor/tests/scan_test.cpp
    ../usb-monitor/files/src/MediaScanner.cpp
)
target_link_libraries(usb-scan-test PRIVATE Common::Headers)
target_link_libraries(usb-scan-test PRIVATE Threads::Threads)

add_executable(can-codec-test
    ../can-daemon/tests/codec_test.cpp
    ../can-daemon/files/src/ProtocolCodec.cpp
)
target_link_libraries(can-codec-test PRIVATE Common::Headers)

foreach(target usb-monitor can-daemon usb-scan-test can-codec-test)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
endforeach()

add_test(NAME usb-scan COMMAND usb-scan-test)
add_test(NAME can-codec COMMAND can-codec-test)
add_test(NAME can-setup COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../can-daemon/tests/setup_test.py)
set_tests_properties(usb-scan can-codec can-setup PROPERTIES TIMEOUT 30)

add_executable(logger-test logger_test.cpp)
target_link_libraries(logger-test PRIVATE Common::Headers)
target_link_libraries(logger-test PRIVATE Threads::Threads)
target_compile_options(logger-test PRIVATE -Wall -Wextra -Werror)
add_test(NAME logger-output COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/logger_output_test.py $<TARGET_FILE:logger-test>)
set_tests_properties(logger-output PROPERTIES TIMEOUT 30)

add_executable(playlist-write-test playlist_write_test.cpp ../usb-monitor/files/src/PlaylistWriter.cpp)
target_link_libraries(playlist-write-test PRIVATE Common::Headers)
target_compile_options(playlist-write-test PRIVATE -Wall -Wextra -Werror)
add_test(NAME playlist-write COMMAND playlist-write-test)
set_tests_properties(playlist-write PROPERTIES TIMEOUT 30)

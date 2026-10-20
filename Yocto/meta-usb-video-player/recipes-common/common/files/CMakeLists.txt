cmake_minimum_required(VERSION 3.16)
project(Common VERSION 1.0 LANGUAGES CXX)
include(GNUInstallDirs)

# One interface target works both from the Yocto sysroot and standalone builds.
add_library(common INTERFACE)
add_library(Common::Headers ALIAS common)
set_target_properties(common PROPERTIES EXPORT_NAME Headers)
target_include_directories(common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/common>
)
target_compile_features(common INTERFACE cxx_std_17)

install(FILES Constants.h Logger.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common)
install(TARGETS common EXPORT CommonTargets)
install(EXPORT CommonTargets
    FILE CommonConfig.cmake
    NAMESPACE Common::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Common
)

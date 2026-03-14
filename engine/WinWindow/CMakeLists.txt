cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE WINWINDOW_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/WinWindow/*.cpp"
)

file(GLOB_RECURSE WINWINDOW_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/WinWindow/*.h"
)

add_library(WinWindow STATIC ${WINWINDOW_SOURCES} ${WINWINDOW_HEADERS})

target_include_directories(WinWindow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(WinWindow PUBLIC cxx_std_20)
target_compile_definitions(WinWindow PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

set_target_properties(WinWindow PROPERTIES FOLDER "engine")

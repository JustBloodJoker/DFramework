cmake_minimum_required(VERSION 3.24)

set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_dx12.cpp
    backends/imgui_impl_win32.cpp
)

set(IMGUI_HEADERS
    imgui.h
    imgui_internal.h
    imconfig.h
    backends/imgui_impl_dx12.h
    backends/imgui_impl_win32.h
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
add_library(thirdparty::imgui ALIAS imgui)

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(imgui PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(imgui PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(imgui PROPERTIES FOLDER "external")

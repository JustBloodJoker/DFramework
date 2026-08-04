cmake_minimum_required(VERSION 3.24)

project(DFramework LANGUAGES CXX)

include(CTest)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/MP>")
endif()

add_subdirectory(external/imgui)
add_subdirectory(engine/WinWindow)
add_subdirectory(engine/D3DFramework)
add_subdirectory(projects/FDW)

if(BUILD_TESTING AND WIN32)
    add_custom_target(DFrameworkTests
        DEPENDS
            WinWindowTests
            D3DFrameworkRenderQueueTests
    )
    set_target_properties(DFrameworkTests PROPERTIES FOLDER "tests")
endif()

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT FDW)

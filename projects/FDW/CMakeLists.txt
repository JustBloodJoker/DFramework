cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE FDW_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Component/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainRenderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/System/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)

file(GLOB_RECURSE FDW_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Component/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainRenderer/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/System/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h"
)

add_executable(FDW ${FDW_SOURCES} ${FDW_HEADERS})

target_include_directories(FDW PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(FDW PRIVATE cxx_std_20)
target_compile_definitions(FDW PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(FDW PRIVATE D3DFramework WinWindow thirdparty::imgui)

set(FDW_CONTENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Content")
add_custom_command(TARGET FDW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${FDW_CONTENT_DIR}"
        "$<TARGET_FILE_DIR:FDW>/Content"
    COMMENT "Copy FDW Content to output directory"
)

install(TARGETS FDW RUNTIME DESTINATION bin)
install(DIRECTORY "${FDW_CONTENT_DIR}/" DESTINATION "bin/Content")

set_target_properties(FDW PROPERTIES
    FOLDER "projects"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

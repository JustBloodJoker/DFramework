cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE D3DFRAMEWORK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/D3DFramework/*.cpp"
)

file(GLOB_RECURSE D3DFRAMEWORK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/D3DFramework/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/D3DFramework/*.cuh"
)

add_library(D3DFramework STATIC ${D3DFRAMEWORK_SOURCES} ${D3DFRAMEWORK_HEADERS})

target_include_directories(D3DFramework PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(D3DFramework PUBLIC cxx_std_20)
target_compile_definitions(D3DFramework PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(D3DFramework PUBLIC WinWindow)

if(WIN32)
    target_link_libraries(D3DFramework PUBLIC d3d12 dxgi dxguid dxcompiler)
endif()

include(CheckLanguage)
check_language(CUDA)

if(CMAKE_CUDA_COMPILER)
    set(DFW_CUDA_ARCHITECTURES "75;86;89" CACHE STRING "CUDA architectures for D3DFramework.")
    enable_language(CUDA)
    target_sources(D3DFramework PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/D3DFramework/GraphicUtilites/PostProcessingAlgorithms.cu"
    )
    set_target_properties(D3DFramework PROPERTIES
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON
        CUDA_ARCHITECTURES "${DFW_CUDA_ARCHITECTURES}"
    )
else()
    message(WARNING "CUDA compiler was not found. PostProcessingAlgorithms.cu is excluded from D3DFramework.")
endif()

find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    target_link_libraries(D3DFramework PUBLIC CUDA::cuda_driver CUDA::cudart)
endif()

set_target_properties(D3DFramework PROPERTIES FOLDER "engine")

if(BUILD_TESTING AND WIN32)
    add_executable(D3DFrameworkRenderQueueTests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/RenderQueueTests.cpp"
    )
    target_compile_features(D3DFrameworkRenderQueueTests PRIVATE cxx_std_20)
    target_link_libraries(D3DFrameworkRenderQueueTests PRIVATE D3DFramework)
    set_target_properties(D3DFrameworkRenderQueueTests PROPERTIES FOLDER "tests")

    add_test(
        NAME D3DFramework.RenderQueueDependencies
        COMMAND D3DFrameworkRenderQueueTests
    )
    set_tests_properties(D3DFramework.RenderQueueDependencies PROPERTIES TIMEOUT 30)

    add_executable(D3DFrameworkBufferReadbackTests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/BufferReadbackTests.cpp"
    )
    target_compile_features(D3DFrameworkBufferReadbackTests PRIVATE cxx_std_20)
    target_link_libraries(D3DFrameworkBufferReadbackTests PRIVATE D3DFramework)
    set_target_properties(D3DFrameworkBufferReadbackTests PROPERTIES FOLDER "tests")
    add_test(NAME D3DFramework.BufferReadback COMMAND D3DFrameworkBufferReadbackTests)
    set_tests_properties(D3DFramework.BufferReadback PROPERTIES
        TIMEOUT 30
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${CMAKE_SOURCE_DIR}/x64/$<CONFIG>"
    )

    add_executable(D3DFrameworkD3D12DiagnosticsTests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/D3D12DiagnosticsTests.cpp"
    )
    target_compile_features(D3DFrameworkD3D12DiagnosticsTests PRIVATE cxx_std_20)
    target_link_libraries(D3DFrameworkD3D12DiagnosticsTests PRIVATE D3DFramework)
    set_target_properties(D3DFrameworkD3D12DiagnosticsTests PROPERTIES FOLDER "tests")
    add_test(NAME D3DFramework.D3D12Diagnostics COMMAND D3DFrameworkD3D12DiagnosticsTests)
    set_tests_properties(D3DFramework.D3D12Diagnostics PROPERTIES TIMEOUT 30)
endif()

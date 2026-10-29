cmake_minimum_required(VERSION 3.24)

add_compile_definitions(_XM_NO_INTRINSICS_)

set(NS_OFFLINE_DATASET_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset/DatasetFormat.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset/DatasetContainer.cpp")
set(NS_OFFLINE_CAPTURE_SUPPORT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ShadowData/ShadowLightParameters.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ShadowData/NeuralShadowData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset/CaptureConfiguration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset/VisibilityData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset/GroundTruthMetadata.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/System/Shadows/RayTracing/RTGeometryCoverage.cpp")
set(NS_OFFLINE_SPLIT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset/DatasetSplit.cpp")

file(GLOB_RECURSE NS_OFFLINE_RENDERER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM NS_OFFLINE_RENDERER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/CaptureApplication.cpp"
    ${NS_OFFLINE_DATASET_SOURCES}
    ${NS_OFFLINE_CAPTURE_SUPPORT_SOURCES}
    ${NS_OFFLINE_SPLIT_SOURCES})

file(GLOB_RECURSE NS_OFFLINE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_library(NeuralShadowOfflineDataset STATIC ${NS_OFFLINE_DATASET_SOURCES})
target_precompile_headers(NeuralShadowOfflineDataset PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(NeuralShadowOfflineDataset PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(NeuralShadowOfflineDataset PUBLIC cxx_std_20)
target_link_libraries(NeuralShadowOfflineDataset PUBLIC D3DFramework WinWindow)
set_target_properties(NeuralShadowOfflineDataset PROPERTIES FOLDER "projects/NeuralShadowOffline/libraries")

add_library(NeuralShadowOfflineCaptureSupport STATIC ${NS_OFFLINE_CAPTURE_SUPPORT_SOURCES})
target_precompile_headers(NeuralShadowOfflineCaptureSupport PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(NeuralShadowOfflineCaptureSupport PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(NeuralShadowOfflineCaptureSupport PUBLIC cxx_std_20)
target_link_libraries(NeuralShadowOfflineCaptureSupport PUBLIC
    NeuralShadowOfflineDataset D3DFramework WinWindow)
set_target_properties(NeuralShadowOfflineCaptureSupport PROPERTIES FOLDER "projects/NeuralShadowOffline/libraries")

add_library(NeuralShadowOfflineSplit STATIC ${NS_OFFLINE_SPLIT_SOURCES})
target_precompile_headers(NeuralShadowOfflineSplit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(NeuralShadowOfflineSplit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(NeuralShadowOfflineSplit PUBLIC cxx_std_20)
target_link_libraries(NeuralShadowOfflineSplit PUBLIC NeuralShadowOfflineCaptureSupport)
set_target_properties(NeuralShadowOfflineSplit PROPERTIES FOLDER "projects/NeuralShadowOffline/libraries")

add_library(NeuralShadowOfflineRendererRuntime STATIC
    ${NS_OFFLINE_RENDERER_SOURCES}
    ${NS_OFFLINE_HEADERS})
target_precompile_headers(NeuralShadowOfflineRendererRuntime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(NeuralShadowOfflineRendererRuntime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(NeuralShadowOfflineRendererRuntime PUBLIC cxx_std_20)
target_compile_definitions(NeuralShadowOfflineRendererRuntime PUBLIC
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(NeuralShadowOfflineRendererRuntime PUBLIC
    D3DFramework
    WinWindow
    NeuralShadowOfflineCaptureSupport)
set_target_properties(NeuralShadowOfflineRendererRuntime PROPERTIES
    FOLDER "projects/NeuralShadowOffline/runtime")

add_executable(NeuralShadowOfflineRenderer
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/CaptureApplication.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/CaptureApplication.h")
target_precompile_headers(NeuralShadowOfflineRenderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(NeuralShadowOfflineRenderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/Application")
target_link_libraries(NeuralShadowOfflineRenderer PRIVATE NeuralShadowOfflineRendererRuntime)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES
    ${NS_OFFLINE_RENDERER_SOURCES} ${NS_OFFLINE_HEADERS}
    ${NS_OFFLINE_DATASET_SOURCES}
    ${NS_OFFLINE_CAPTURE_SUPPORT_SOURCES}
    ${NS_OFFLINE_SPLIT_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/CaptureApplication.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/CaptureApplication.h")

set_target_properties(NeuralShadowOfflineRenderer PROPERTIES
    FOLDER "projects"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS
        "--config &quot;${CMAKE_CURRENT_SOURCE_DIR}/Content/CaptureConfigs/capture.example.ini&quot; --output &quot;${CMAKE_BINARY_DIR}/offline-capture-debug&quot;")

add_custom_command(TARGET NeuralShadowOfflineRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Content"
        "$<TARGET_FILE_DIR:NeuralShadowOfflineRenderer>/Content"
    COMMENT "Copy NeuralShadowOfflineRenderer Content")

if(BUILD_TESTING AND WIN32)
    function(ns_offline_add_data_test target source library)
        add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/tests/${source}")
        target_precompile_headers(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
        target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_compile_features(${target} PRIVATE cxx_std_20)
        target_link_libraries(${target} PRIVATE ${library})
        set_target_properties(${target} PROPERTIES FOLDER "projects/NeuralShadowOffline/tests")
    endfunction()

    ns_offline_add_data_test(NeuralShadowOfflineDatasetTests
        NeuralShadowDatasetContainerTests.cpp NeuralShadowOfflineCaptureSupport)
    ns_offline_add_data_test(NeuralShadowOfflineCaptureTests
        NeuralShadowCaptureConfigurationTests.cpp NeuralShadowOfflineCaptureSupport)
    ns_offline_add_data_test(NeuralShadowOfflineRTTargetTests
        NeuralShadowGroundTruthMetadataTests.cpp NeuralShadowOfflineCaptureSupport)
    ns_offline_add_data_test(NeuralShadowOfflineSplitTests
        NeuralShadowDatasetSplitTests.cpp NeuralShadowOfflineSplit)

    add_test(NAME NeuralShadowOffline.Dataset COMMAND NeuralShadowOfflineDatasetTests)
    set_tests_properties(NeuralShadowOffline.Dataset PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${CMAKE_SOURCE_DIR}/x64/$<CONFIG>")
    add_test(NAME NeuralShadowOffline.Capture COMMAND NeuralShadowOfflineCaptureTests)
    add_test(NAME NeuralShadowOffline.RTTarget COMMAND NeuralShadowOfflineRTTargetTests)
    add_test(NAME NeuralShadowOffline.Split COMMAND NeuralShadowOfflineSplitTests)
    add_test(NAME NeuralShadow.ArchitectureBoundaries
        COMMAND ${CMAKE_COMMAND}
            "-DREPOSITORY_ROOT=${CMAKE_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateProjectBoundaries.cmake")

    set(NS_OFFLINE_DXC_EXECUTABLE "${CMAKE_SOURCE_DIR}/x64/dxc.exe")
    if(EXISTS "${NS_OFFLINE_DXC_EXECUTABLE}")
        add_test(NAME NeuralShadowOffline.AtlasRTShadowShaders
            COMMAND "${NS_OFFLINE_DXC_EXECUTABLE}"
                -T lib_6_5
                -E RayGen
                -I "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/DefaultInclude"
                -I "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/ShadowCommon"
                -Fo "${CMAKE_CURRENT_BINARY_DIR}/AtlasRTShadowRayGen.dxil"
                "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/AtlasRTShadow/RayGen.hlsl")
        set_tests_properties(NeuralShadowOffline.AtlasRTShadowShaders PROPERTIES TIMEOUT 30)

        add_test(NAME NeuralShadowOffline.RTTargetShaders
            COMMAND ${CMAKE_COMMAND}
                "-DDXC_EXECUTABLE=${NS_OFFLINE_DXC_EXECUTABLE}"
                "-DSHADER_DIR=${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/NeuralShadowRTTarget"
                "-DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/DefaultInclude"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/NeuralShadowRTTarget"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateNeuralShadowRTTargetHLSL.cmake")
    endif()
endif()

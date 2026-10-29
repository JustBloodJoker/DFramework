cmake_minimum_required(VERSION 3.24)

add_compile_definitions(_XM_NO_INTRINSICS_)

file(GLOB_RECURSE FDW_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Component/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Config/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ShadowData/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainRenderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/System/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/*.cpp")
file(GLOB_RECURSE FDW_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_library(FDWRendererRuntime STATIC ${FDW_RUNTIME_SOURCES} ${FDW_HEADERS})
target_precompile_headers(FDWRendererRuntime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(FDWRendererRuntime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(FDWRendererRuntime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/include")
target_compile_features(FDWRendererRuntime PUBLIC cxx_std_20)
target_compile_definitions(FDWRendererRuntime PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(FDWRendererRuntime PUBLIC D3DFramework WinWindow thirdparty::imgui)
target_link_libraries(FDWRendererRuntime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/lib/win-x64/onnxruntime.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/lib/win-x64/DirectML.lib"
    bcrypt delayimp)
target_link_options(FDWRendererRuntime PRIVATE "/DELAYLOAD:onnxruntime.dll" "/DELAYLOAD:DirectML.dll")
set_target_properties(FDWRendererRuntime PROPERTIES FOLDER "libraries/runtime")

add_executable(FDW "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_precompile_headers(FDW REUSE_FROM FDWRendererRuntime)
target_link_libraries(FDW PRIVATE FDWRendererRuntime)
set_target_properties(FDW PROPERTIES
    FOLDER "projects"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(FDW_CONTENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Content")
add_custom_command(TARGET FDW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${FDW_CONTENT_DIR}" "$<TARGET_FILE_DIR:FDW>/Content"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FDW_CONTENT_DIR}/ShadowRendering/Runtime/onnxruntime.dll" "$<TARGET_FILE_DIR:FDW>/onnxruntime.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FDW_CONTENT_DIR}/ShadowRendering/Runtime/DirectML.dll" "$<TARGET_FILE_DIR:FDW>/DirectML.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/LICENSE-ONNXRUNTIME.txt"
        "$<TARGET_FILE_DIR:FDW>/LICENSE-ONNXRUNTIME.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/LICENSE-DIRECTML.txt"
        "$<TARGET_FILE_DIR:FDW>/LICENSE-DIRECTML.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/x64/dxcompiler.dll" "$<TARGET_FILE_DIR:FDW>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/x64/dxil.dll" "$<TARGET_FILE_DIR:FDW>/dxil.dll"
    COMMENT "Copy FDW content and runtime dependencies")

install(TARGETS FDW RUNTIME DESTINATION bin)
install(DIRECTORY "${FDW_CONTENT_DIR}/" DESTINATION "bin/Content")
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/LICENSE-ONNXRUNTIME.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/onnxruntime-directml/LICENSE-DIRECTML.txt"
    DESTINATION bin)

if(BUILD_TESTING AND WIN32)
    add_executable(FDWNeuralShadowRoutingTests tests/NeuralShadowRoutingTests.cpp)
	target_precompile_headers(FDWNeuralShadowRoutingTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
    target_include_directories(FDWNeuralShadowRoutingTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(FDWNeuralShadowRoutingTests PRIVATE cxx_std_20)
    target_compile_definitions(FDWNeuralShadowRoutingTests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(FDWNeuralShadowRoutingTests PRIVATE FDWRendererRuntime)
    add_test(NAME FDW.NeuralShadowRouting COMMAND FDWNeuralShadowRoutingTests)

    add_executable(FDWShadowLightSelectorTests tests/ShadowLightSelectorTests.cpp)
	target_precompile_headers(FDWShadowLightSelectorTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
    target_include_directories(FDWShadowLightSelectorTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(FDWShadowLightSelectorTests PRIVATE cxx_std_20)
    target_link_libraries(FDWShadowLightSelectorTests PRIVATE FDWRendererRuntime)
    add_test(NAME FDW.ShadowLightSelector COMMAND FDWShadowLightSelectorTests)

    add_executable(FDWShadowRouterTests tests/ShadowRouterTests.cpp)
	target_precompile_headers(FDWShadowRouterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
    target_include_directories(FDWShadowRouterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(FDWShadowRouterTests PRIVATE cxx_std_20)
    target_compile_definitions(FDWShadowRouterTests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(FDWShadowRouterTests PRIVATE FDWRendererRuntime)
    add_test(NAME FDW.ShadowRouter COMMAND FDWShadowRouterTests)

    add_executable(FDWNeuralShadowModelPackageTests
        tests/NeuralShadowModelPackageTests.cpp
        src/System/Shadows/Neural/NeuralShadowModelPackage.cpp
        src/ShadowData/NeuralShadowData.cpp)
    target_precompile_headers(FDWNeuralShadowModelPackageTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
    target_include_directories(FDWNeuralShadowModelPackageTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(FDWNeuralShadowModelPackageTests PRIVATE cxx_std_20)
    target_compile_definitions(FDWNeuralShadowModelPackageTests PRIVATE
        NOMINMAX WIN32_LEAN_AND_MEAN
        "FDW_NEURAL_MODEL_TEST_GRAPH=L\"${FDW_CONTENT_DIR}/ShadowRendering/NeuralModel/neural_shadow_model.onnx\"")
    target_link_libraries(FDWNeuralShadowModelPackageTests PRIVATE D3DFramework WinWindow bcrypt)
    add_test(NAME FDW.NeuralShadowModelPackage COMMAND FDWNeuralShadowModelPackageTests)

    set(FDW_DXC_EXECUTABLE "${CMAKE_SOURCE_DIR}/x64/dxc.exe")
    if(EXISTS "${FDW_DXC_EXECUTABLE}")
        add_test(NAME FDW.NeuralRayGenerationShader
            COMMAND "${FDW_DXC_EXECUTABLE}"
                -T lib_6_5 -E RayGen
                -I "${FDW_CONTENT_DIR}/Shaders/DefaultInclude"
                -I "${FDW_CONTENT_DIR}/Shaders/NeuralShadowRayTracing"
                -Fo "${CMAKE_CURRENT_BINARY_DIR}/NeuralRayGen.dxil"
                "${FDW_CONTENT_DIR}/Shaders/NeuralShadowRayTracing/RayGen.hlsl")
        add_test(NAME FDW.NeuralTensorShader
            COMMAND "${FDW_DXC_EXECUTABLE}"
                -T cs_6_5 -E CS
                -I "${FDW_CONTENT_DIR}/Shaders/DefaultInclude"
                -I "${FDW_CONTENT_DIR}/Shaders/NeuralShadowTensorPacking"
                -Fo "${CMAKE_CURRENT_BINARY_DIR}/NeuralTensor.dxil"
                "${FDW_CONTENT_DIR}/Shaders/NeuralShadowTensorPacking/CS.hlsl")
    endif()
endif()

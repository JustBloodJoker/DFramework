if(NOT DEFINED REPOSITORY_ROOT)
    message(FATAL_ERROR "REPOSITORY_ROOT is required")
endif()

set(FDW_ROOT "${REPOSITORY_ROOT}/projects/FDW")
set(OFFLINE_ROOT "${REPOSITORY_ROOT}/projects/NeuralShadowOfflineRenderer")
set(TRAINING_ROOT "${REPOSITORY_ROOT}/projects/NeuralShadowTraining")

foreach(required_root IN ITEMS "${FDW_ROOT}" "${OFFLINE_ROOT}" "${TRAINING_ROOT}")
    if(NOT IS_DIRECTORY "${required_root}")
        message(FATAL_ERROR "Required sibling project is missing: ${required_root}")
    endif()
endforeach()

set(ROOT_SOLUTION "${REPOSITORY_ROOT}/FDW.sln")
foreach(required_file IN ITEMS "${ROOT_SOLUTION}")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Required checked-in Visual Studio integration is missing: ${required_file}")
    endif()
endforeach()

file(READ "${FDW_ROOT}/FDW.vcxproj" fdw_project_text)
file(READ "${FDW_ROOT}/FDW.vcxproj.filters" fdw_filter_text)
if(fdw_project_text MATCHES "\\.hlsl(i)?" OR fdw_filter_text MATCHES "\\.hlsl(i)?")
    message(FATAL_ERROR "Shader files must not be listed in the checked-in Visual Studio project")
endif()

file(READ "${OFFLINE_ROOT}/CMakeLists.txt" offline_cmake_text)
if(offline_cmake_text MATCHES "NS_OFFLINE_CONTENT")
    message(FATAL_ERROR "Offline shader/content files must not be added to generated Visual Studio projects")
endif()

file(READ "${ROOT_SOLUTION}" solution_text)
foreach(required_solution_project IN ITEMS
        "projects\\FDW\\FDW.vcxproj"
        "engine\\D3DFramework\\D3DFramework.vcxproj"
        "engine\\WinWindow\\WinWindow.vcxproj")
    string(FIND "${solution_text}" "${required_solution_project}" project_position)
    if(project_position EQUAL -1)
        message(FATAL_ERROR "Root solution does not expose project: ${required_solution_project}")
    endif()
endforeach()

set(forbidden_fdw_paths
    "${FDW_ROOT}/src/Capture"
    "${FDW_ROOT}/libraries"
    "${FDW_ROOT}/tools"
    "${FDW_ROOT}/training"
    "${FDW_ROOT}/Content/Shaders/NeuralShadowRTTarget")
foreach(path IN LISTS forbidden_fdw_paths)
    if(EXISTS "${path}")
        message(FATAL_ERROR "Offline-generation path leaked into FDW: ${path}")
    endif()
endforeach()

# FDW legitimately deploys the frozen runtime-only model packages plus the
# accepted ONNX Runtime/DirectML dependencies. Keep the project boundary
# strict by allowing exactly these runtime artifacts, never dataset/capture/
# training infrastructure.
set(FDW_RUNTIME_MODEL_ROOT "${FDW_ROOT}/Content/NeuralShadows")
if(IS_DIRECTORY "${FDW_RUNTIME_MODEL_ROOT}")
    file(GLOB_RECURSE fdw_runtime_model_files RELATIVE "${FDW_RUNTIME_MODEL_ROOT}"
        LIST_DIRECTORIES FALSE "${FDW_RUNTIME_MODEL_ROOT}/*")
    list(SORT fdw_runtime_model_files)
    set(expected_fdw_runtime_model_files
		"NeuralModel/neural_shadow_model.json"
		"NeuralModel/neural_shadow_model.onnx"
		"Runtime/DirectML.dll"
		"Runtime/onnxruntime.dll")
    if(NOT fdw_runtime_model_files STREQUAL expected_fdw_runtime_model_files)
        message(FATAL_ERROR
            "FDW runtime-model deployment contains non-runtime artifacts: ${fdw_runtime_model_files}")
    endif()
endif()

function(read_project_text root output)
    file(GLOB_RECURSE candidates LIST_DIRECTORIES FALSE
        "${root}/CMakeLists.txt"
        "${root}/*.cmake"
        "${root}/*.cpp"
        "${root}/*.h"
        "${root}/*.hpp"
        "${root}/*.hlsl"
        "${root}/*.hlsli"
        "${root}/*.py"
        "${root}/*.toml"
        "${root}/*.cmd"
        "${root}/*.vcxproj"
        "${root}/*.filters")
    set(combined "")
    foreach(candidate IN LISTS candidates)
        # The boundary test necessarily contains the forbidden dependency
        # spellings that it enforces. Do not audit the test implementation
        # as though it were production project source.
        if(candidate STREQUAL CMAKE_CURRENT_LIST_FILE)
            continue()
        endif()
        file(READ "${candidate}" contents)
        string(APPEND combined "\n${candidate}\n${contents}")
    endforeach()
    set(${output} "${combined}" PARENT_SCOPE)
endfunction()

read_project_text("${FDW_ROOT}" fdw_text)
foreach(pattern IN ITEMS
        "NeuralShadowCaptureController"
        "CaptureApplication"
        "CapturePlanner"
        "DatasetWriter"
        "CaptureConfig"
        "CaptureJournal"
        "RTTargetProducer"
        "FDW_ENABLE_NEURAL_SHADOW_CAPTURE"
        "OfflineCapture"
        "captureActive"
        "captureInitProgress"
        "captureGeneration"
        "\\.nssample")
    if(fdw_text MATCHES "${pattern}")
        message(FATAL_ERROR "Forbidden offline-generation dependency in FDW: ${pattern}")
    endif()
endforeach()

read_project_text("${OFFLINE_ROOT}" offline_text)
foreach(pattern IN ITEMS
        "projects[/\\\\]FDW"
        "\\.\\.[/\\\\]FDW"
        "<MainRenderer/"
        "FDWRendererRuntime"
        "FDWCaptureRendererRuntime")
    if(offline_text MATCHES "${pattern}")
        message(FATAL_ERROR "Offline renderer depends on FDW: ${pattern}")
    endif()
endforeach()

read_project_text("${TRAINING_ROOT}" training_text)
foreach(pattern IN ITEMS "projects[/\\\\]FDW" "\\.\\.[/\\\\]FDW")
    if(training_text MATCHES "${pattern}")
        message(FATAL_ERROR "Training project depends on FDW source: ${pattern}")
    endif()
endforeach()

message(STATUS "Neural-shadow sibling project boundaries are clean")

foreach(required_variable DXC_EXECUTABLE SHADER_DIR INCLUDE_DIR OUTPUT_DIR)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing ${required_variable}")
    endif()
endforeach()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
foreach(shader RayGen ClosestHit AnyHit Miss)
    execute_process(
        COMMAND "${DXC_EXECUTABLE}" -T lib_6_3 -O3 -I "${INCLUDE_DIR}" -I "${SHADER_DIR}"
            -Fo "${OUTPUT_DIR}/${shader}.dxil" -Fc "${OUTPUT_DIR}/${shader}.asm"
            "${SHADER_DIR}/${shader}.hlsl"
        RESULT_VARIABLE shader_result OUTPUT_VARIABLE shader_stdout ERROR_VARIABLE shader_stderr)
    if(NOT shader_result EQUAL 0)
        message(FATAL_ERROR "Neural RT target ${shader} compilation failed (${shader_result}):\n${shader_stdout}\n${shader_stderr}")
    endif()
endforeach()

execute_process(
    COMMAND "${DXC_EXECUTABLE}" -T rootsig_1_1 -E RS
        -Fo "${OUTPUT_DIR}/RS.dxil" "${SHADER_DIR}/RS.hlsl"
    RESULT_VARIABLE rs_result OUTPUT_VARIABLE rs_stdout ERROR_VARIABLE rs_stderr)
if(NOT rs_result EQUAL 0)
    message(FATAL_ERROR "Neural RT target root-signature compilation failed (${rs_result}):\n${rs_stdout}\n${rs_stderr}")
endif()

file(READ "${OUTPUT_DIR}/RayGen.asm" assembly)
foreach(required_text
    "InverseViewProjectionMatrix;.*Offset:    0"
    "Quality;.*Offset:   64"
    "PassIndex;.*Offset:  144"
    "LayoutVersion;.*Offset:  172"
    "Resource bind info for WorkItems"
    "Resource bind info for WorkHeader"
    "Resource bind info for OutputVisibility"
    "Resource bind info for OutputValidity"
    "Resource bind info for OutputHeader")
    string(REGEX MATCH "${required_text}" found "${assembly}")
    if(NOT found)
        message(FATAL_ERROR "Compiled direct RT target shader is missing ${required_text}")
    endif()
endforeach()

file(READ "${SHADER_DIR}/RayGen.hlsl" source)
foreach(forbidden_text "TexelToLight" "LightAtlasMeta" "PerLightVisibility" "ShadowAtlas :")
    string(FIND "${source}" "${forbidden_text}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "Direct RT target shader illegally references atlas-backed symbol ${forbidden_text}")
    endif()
endforeach()

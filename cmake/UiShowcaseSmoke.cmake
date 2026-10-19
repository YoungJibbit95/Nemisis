execute_process(COMMAND "${GAME_EXE}" --ui-showcase --smoke-test "--ui-size=${UI_SIZE}"
    WORKING_DIRECTORY "${GAME_ROOT}" RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 55)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Native Vulkan UI smoke failed (${result})\n${output}\n${error}")
endif()
if(NOT "${output}${error}" MATCHES "Native UI Vulkan draws: [1-9][0-9]*")
    message(FATAL_ERROR "No native UI draw was actually recorded on Vulkan\n${output}\n${error}")
endif()
message(STATUS "Native Vulkan UI submitted at ${UI_SIZE}")

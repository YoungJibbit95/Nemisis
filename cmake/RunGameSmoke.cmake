# Check real startup and process success together. CTest's PASS_REGULAR_EXPRESSION
# alone would ignore a nonzero exit status after a matching startup message.
set(smoke_arguments "${SMOKE_ARGUMENT}")
if(NOT REQUIRE_VULKAN)
    list(APPEND smoke_arguments --no-vulkan)
endif()

execute_process(
    COMMAND "${GAME_EXECUTABLE}" ${smoke_arguments}
    RESULT_VARIABLE smoke_result
    OUTPUT_VARIABLE smoke_output
    ERROR_VARIABLE smoke_error
    TIMEOUT 110
)
string(CONCAT smoke_output "${smoke_output}" "${smoke_error}")
message("${smoke_output}")

if(NOT smoke_result STREQUAL "0")
    message(FATAL_ERROR "Game smoke process failed: ${smoke_result}")
endif()
if(NOT smoke_output MATCHES "Asset catalog loaded: [1-9][0-9]* assets" OR
   NOT smoke_output MATCHES "Application shutdown: nemisis_game")
    message(FATAL_ERROR "Game smoke did not complete startup, asset loading and shutdown")
endif()
if(smoke_output MATCHES "Initial config load failed|Dev asset binding failed|Dev mesh resource missing|Dev mesh resource registration failed")
    message(FATAL_ERROR "Game smoke reported missing or invalid runtime data")
endif()
if(REQUIRE_VULKAN AND NOT smoke_output MATCHES "Vulkan backend active:")
    message(FATAL_ERROR "Graphics smoke requires an active Vulkan backend; headless/null fallback is not graphics validation")
endif()

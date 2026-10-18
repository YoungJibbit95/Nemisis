# Check real startup and process success together. CTest's PASS_REGULAR_EXPRESSION
# alone would ignore a nonzero exit status after a matching startup message.
set(smoke_arguments "${SMOKE_ARGUMENT}")
if(NOT REQUIRE_VULKAN)
    list(PREPEND smoke_arguments --no-vulkan)
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

if(REQUIRE_HAND_IK)
    if(NOT smoke_output MATCHES "Hand IK frames: applied=([1-9][0-9]*) samples=([1-9][0-9]*) skins=([1-9][0-9]*) vertex_updates=([1-9][0-9]*) invalid=0 unreachable=0")
        message(FATAL_ERROR "Opt-in hand IK smoke did not apply valid reachable constraints")
    endif()
    if(NOT CMAKE_MATCH_1 STREQUAL CMAKE_MATCH_2 OR NOT CMAKE_MATCH_2 STREQUAL CMAKE_MATCH_3 OR NOT CMAKE_MATCH_3 STREQUAL CMAKE_MATCH_4)
        message(FATAL_ERROR "Hand IK smoke must sample, constrain, skin and submit one accepted vertex update per pose")
    endif()
endif()

if(REQUIRE_PROJECT_FPS)
    if(NOT smoke_output MATCHES "Project FPS preview loaded: body=chr_project_male1_fp_body_preview arms=chr_project_male1_fp_arms_preview vertices=50068")
        message(FATAL_ERROR "Preview did not load the actual Project subsets")
    endif()
    if(NOT smoke_output MATCHES "Project FPS preview frames: body_samples=([1-9][0-9]*) body_skins=([1-9][0-9]*) body_updates=([1-9][0-9]*) arms_samples=([1-9][0-9]*) arms_skins=([1-9][0-9]*) arms_updates=([1-9][0-9]*) world_updates=0 submitted_body=1 submitted_arms=1")
        message(FATAL_ERROR "Preview did not animate/update/submit both Project meshes without hidden full-body updates")
    endif()
    if(NOT CMAKE_MATCH_1 STREQUAL CMAKE_MATCH_2 OR NOT CMAKE_MATCH_2 STREQUAL CMAKE_MATCH_3 OR
       NOT CMAKE_MATCH_3 STREQUAL CMAKE_MATCH_4 OR NOT CMAKE_MATCH_4 STREQUAL CMAKE_MATCH_5 OR
       NOT CMAKE_MATCH_5 STREQUAL CMAKE_MATCH_6)
        message(FATAL_ERROR "Preview must sample/skin/update each visible subset once per fixed tick")
    endif()
endif()

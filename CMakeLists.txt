cmake_minimum_required(VERSION 3.27)

project(Nemisis
    VERSION 0.1.0
    DESCRIPTION "6v6 Arena Shooter - Nemisis Game"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NEMISIS_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(NEMISIS_COPY_RUNTIME_DATA "Copy configs and asset docs beside the game executable" ON)
set(NOVACORE_ENGINE_ROOT "" CACHE PATH "Path to a NovaCore Engine checkout")

if(MSVC)
    add_compile_options(/EHsc)
endif()

function(nemisis_copy_mingw_runtime target_name)
    if(NOT MINGW)
        return()
    endif()

    get_filename_component(NEMISIS_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(runtime_dll IN ITEMS
        libatomic-1.dll
        libgcc_s_seh-1.dll
        libgomp-1.dll
        libssp-0.dll
        libstdc++-6.dll
        libwinpthread-1.dll
    )
        if(EXISTS "${NEMISIS_MINGW_BIN_DIR}/${runtime_dll}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${NEMISIS_MINGW_BIN_DIR}/${runtime_dll}"
                    "$<TARGET_FILE_DIR:${target_name}>/${runtime_dll}"
                VERBATIM
            )
        endif()
    endforeach()
endfunction()

if(NOT NOVACORE_ENGINE_ROOT)
    get_filename_component(NEMISIS_WORKSPACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    if(EXISTS "${NEMISIS_WORKSPACE_ROOT}/Novacore-Engine/CMakeLists.txt")
        set(NOVACORE_ENGINE_ROOT "${NEMISIS_WORKSPACE_ROOT}/Novacore-Engine" CACHE PATH "Path to a NovaCore Engine checkout" FORCE)
    endif()
endif()

if(NOVACORE_ENGINE_ROOT AND EXISTS "${NOVACORE_ENGINE_ROOT}/CMakeLists.txt")
    set(NOVACORE_BUILD_SERVER OFF CACHE BOOL "Build the NovaCore dedicated server executable" FORCE)
    set(NOVACORE_BUILD_TESTS OFF CACHE BOOL "Build NovaCore smoke tests" FORCE)
    add_subdirectory("${NOVACORE_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/Novacore-Engine")
else()
    find_package(Novacore CONFIG REQUIRED)
endif()

add_executable(nemisis_game
    game/src/GameApp.cpp
    game/src/assets/DevAssetBindings.cpp
    game/src/assets/GameAssetCatalog.cpp
    game/src/dev/DebugTarget.cpp
    game/src/dev/DevRangeRenderScene.cpp
    game/src/dev/DevRangeSession.cpp
    game/src/dev/DevSandbox.cpp
    game/src/dev/DevTargetRange.cpp
    game/src/dev/GreyboxCollision.cpp
    game/src/dev/GreyboxWorld.cpp
    game/src/input/InputBindings.cpp
    game/src/input/InputCommandBuilder.cpp
    game/src/input/GameplayInputBuffer.cpp
    game/src/main.cpp
    game/src/movement/MovementConfig.cpp
    game/src/movement/MovementSystem.cpp
    game/src/net/CommandMessages.cpp
    game/src/net/LoopbackCommandBridge.cpp
    game/src/net/PredictionHistory.cpp
    game/src/net/SnapshotInterpolation.cpp
    game/src/net/SnapshotMessages.cpp
    game/src/player/PlayerAnimation.cpp
    game/src/player/PlayerCommandQueue.cpp
    game/src/player/PlayerCameraRig.cpp
    game/src/player/PlayerFirstPersonRig.cpp
    game/src/player/PlayerHealth.cpp
    game/src/player/PlayerProfile.cpp
    game/src/player/PlayerSkeletalAnimator.cpp
    game/src/player/PlayerSpawn.cpp
    game/src/player/PlayerView.cpp
    game/src/render/RenderTuning.cpp
    game/src/settings/GameSettings.cpp
    game/src/settings/UserSettingsPersistence.cpp
    game/src/ui/GameMenu.cpp
    game/src/ui/UiCanvas.cpp
    game/src/weapons/WeaponAttachments.cpp
    game/src/weapons/WeaponMetrics.cpp
    game/src/weapons/WeaponShot.cpp
    game/src/weapons/WeaponSimulation.cpp
    game/src/weapons/WeaponSystem.cpp
)

target_include_directories(nemisis_game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_game PRIVATE Novacore::Engine)
set_target_properties(nemisis_game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(NEMISIS_COPY_RUNTIME_DATA)
    add_custom_command(TARGET nemisis_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/configs"
            "$<TARGET_FILE_DIR:nemisis_game>/configs"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:nemisis_game>/assets"
        VERBATIM
    )
endif()

if(MSVC)
    target_compile_options(nemisis_game PRIVATE /W4 /permissive-)
    if(NEMISIS_WARNINGS_AS_ERRORS)
        target_compile_options(nemisis_game PRIVATE /WX)
    endif()
else()
    target_compile_options(nemisis_game PRIVATE -Wall -Wextra -Wpedantic)
    if(NEMISIS_WARNINGS_AS_ERRORS)
        target_compile_options(nemisis_game PRIVATE -Werror)
    endif()
endif()

enable_testing()

option(NEMISIS_BUILD_PROJECT_FPS_CANDIDATE "Cook, test and stage the optional Project FPS runtime preview" OFF)
if(NEMISIS_BUILD_PROJECT_FPS_CANDIDATE)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    set(project_fps_candidate_dir "${CMAKE_BINARY_DIR}/project-fps-candidate")
    add_custom_command(
        OUTPUT "${project_fps_candidate_dir}/arms.glb" "${project_fps_candidate_dir}/body.glb"
               "${project_fps_candidate_dir}/arms_runtime.glb" "${project_fps_candidate_dir}/body_runtime.glb"
               "${project_fps_candidate_dir}/arms.map" "${project_fps_candidate_dir}/body.map"
               "${project_fps_candidate_dir}/report.json" "${project_fps_candidate_dir}/preview_assets.json"
               "${project_fps_candidate_dir}/arms_runtime.metadata.json" "${project_fps_candidate_dir}/body_runtime.metadata.json"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/assets/cook_project_fps_candidate.py"
                --output "${project_fps_candidate_dir}"
        DEPENDS tools/assets/cook_project_fps_candidate.py tools/assets/asset_readiness_check.py assets/metadata/project_fps_candidate_regions.json
                assets/processed/normalized/project_assets/chr_project_male1.glb assets/project_assets/character_male1.glb
        VERBATIM
    )
    add_custom_target(nemisis_project_fps_candidate
        DEPENDS "${project_fps_candidate_dir}/report.json" "${project_fps_candidate_dir}/arms.glb"
                "${project_fps_candidate_dir}/body.glb" "${project_fps_candidate_dir}/arms.map"
                "${project_fps_candidate_dir}/body.map" "${project_fps_candidate_dir}/preview_assets.json"
                "${project_fps_candidate_dir}/arms_runtime.glb" "${project_fps_candidate_dir}/body_runtime.glb"
                "${project_fps_candidate_dir}/arms_runtime.metadata.json" "${project_fps_candidate_dir}/body_runtime.metadata.json")
    add_dependencies(nemisis_game nemisis_project_fps_candidate)
    add_custom_command(
        OUTPUT "${project_fps_candidate_dir}/ready_assets.json"
               "${project_fps_candidate_dir}/arms_ready.glb" "${project_fps_candidate_dir}/body_ready.glb"
               "${project_fps_candidate_dir}/rifle_ready.glb"
               "${project_fps_candidate_dir}/arms_ready.metadata.json" "${project_fps_candidate_dir}/body_ready.metadata.json"
               "${project_fps_candidate_dir}/rifle_ready.metadata.json"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/assets/cook_project_rifle_ready.py"
                --candidate-dir "${project_fps_candidate_dir}"
        DEPENDS nemisis_project_fps_candidate "${project_fps_candidate_dir}/arms_runtime.glb"
                "${project_fps_candidate_dir}/body_runtime.glb" "${project_fps_candidate_dir}/preview_assets.json"
                assets/metadata/project_rifle_ready_pose.json tools/assets/cook_project_rifle_ready.py
                assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb
        VERBATIM)
    add_custom_target(nemisis_project_rifle_ready DEPENDS "${project_fps_candidate_dir}/ready_assets.json")
    add_dependencies(nemisis_game nemisis_project_rifle_ready)
    target_compile_definitions(nemisis_game PRIVATE NEMISIS_PROJECT_FPS_PREVIEW=1
        NEMISIS_PROJECT_FPS_BUILD_ROOT="${CMAKE_BINARY_DIR}")
    add_custom_command(TARGET nemisis_game POST_BUILD
        COMMAND "${CMAKE_COMMAND}" "-DSOURCE=${project_fps_candidate_dir}"
                "-DDEST=$<TARGET_FILE_DIR:nemisis_game>/project-fps-candidate"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageProjectFpsPreview.cmake" VERBATIM)
    add_executable(nemisis_project_fps_candidate_tests tests/project_fps_candidate_tests.cpp)
    target_link_libraries(nemisis_project_fps_candidate_tests PRIVATE Novacore::Engine)
    target_compile_definitions(nemisis_project_fps_candidate_tests PRIVATE
        NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
        NEMISIS_PROJECT_FPS_CANDIDATE_ROOT="${project_fps_candidate_dir}")
    nemisis_copy_mingw_runtime(nemisis_project_fps_candidate_tests)
    add_dependencies(nemisis_project_fps_candidate_tests nemisis_project_fps_candidate)
    add_custom_command(
        OUTPUT "${project_fps_candidate_dir}/arms_palm_proposal.glb" "${project_fps_candidate_dir}/palm_proposal_report.json"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/assets/author_project_palm_proposal.py"
                --candidate-dir "${project_fps_candidate_dir}"
        DEPENDS "${project_fps_candidate_dir}/arms.glb" "${project_fps_candidate_dir}/report.json"
                assets/metadata/project_palm_frame_proposal.json tools/assets/author_project_palm_proposal.py
                tools/assets/asset_readiness_check.py VERBATIM)
    add_custom_target(nemisis_project_palm_proposal
        DEPENDS "${project_fps_candidate_dir}/arms_palm_proposal.glb" "${project_fps_candidate_dir}/palm_proposal_report.json")
    add_dependencies(nemisis_project_fps_candidate_tests nemisis_project_palm_proposal)
    add_test(NAME nemisis_project_palm_proposal_tests COMMAND nemisis_project_fps_candidate_tests --palm-proposal)
    add_test(NAME nemisis_project_fps_candidate_tests COMMAND nemisis_project_fps_candidate_tests)
    add_test(NAME nemisis_project_fps_cook_tests
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/project_fps_cook_tests.py"
                "${project_fps_candidate_dir}")
endif()

function(nemisis_add_runtime_smoke test_name argument require_vulkan)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            "-DGAME_EXECUTABLE=$<TARGET_FILE:nemisis_game>"
            "-DSMOKE_ARGUMENT=${argument}"
            "-DREQUIRE_VULKAN=${require_vulkan}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunGameSmoke.cmake"
    )
endfunction()

nemisis_add_runtime_smoke(nemisis_game_smoke --smoke-test OFF)
nemisis_add_runtime_smoke(nemisis_game_vulkan_smoke --vulkan-smoke-test ON)
nemisis_add_runtime_smoke(nemisis_game_vulkan_dev_range_smoke --vulkan-dev-range-smoke-test ON)
nemisis_add_runtime_smoke(nemisis_game_menu_flow_smoke --menu-flow-smoke-test ON)

# These exercise real cooked assets/configs without copying the authoring tree.
set_tests_properties(
    nemisis_game_smoke
    nemisis_game_vulkan_smoke
    nemisis_game_vulkan_dev_range_smoke
    nemisis_game_menu_flow_smoke
    PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        TIMEOUT 120
)

# CPU-only CI excludes this explicit hardware label. An unavailable Vulkan
# backend must never yield a successful graphics smoke through the null renderer.
set_tests_properties(
    nemisis_game_vulkan_smoke
    nemisis_game_vulkan_dev_range_smoke
    nemisis_game_menu_flow_smoke
    PROPERTIES
        LABELS "gpu"
)

add_executable(nemisis_movement_replay_tests
    game/src/input/InputCommandBuilder.cpp
    game/src/movement/MovementConfig.cpp
    game/src/movement/MovementSystem.cpp
    game/src/settings/GameSettings.cpp
    tests/movement_replay_tests.cpp
)

target_include_directories(nemisis_movement_replay_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_movement_replay_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_movement_replay_tests COMMAND nemisis_movement_replay_tests)

add_executable(nemisis_input_command_tests
    game/src/input/InputBindings.cpp
    game/src/input/InputCommandBuilder.cpp
    game/src/input/GameplayInputBuffer.cpp
    game/src/settings/GameSettings.cpp
    tests/input_command_tests.cpp
)

target_include_directories(nemisis_input_command_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_input_command_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_input_command_tests COMMAND nemisis_input_command_tests)

add_executable(nemisis_game_menu_tests
    game/src/dev/DebugTarget.cpp
    game/src/dev/DevRangeSession.cpp
    game/src/dev/DevTargetRange.cpp
    game/src/input/InputBindings.cpp
    game/src/player/PlayerHealth.cpp
    game/src/player/PlayerProfile.cpp
    game/src/settings/GameSettings.cpp
    game/src/ui/GameMenu.cpp
    game/src/ui/UiCanvas.cpp
    game/src/weapons/WeaponAttachments.cpp
    tests/game_menu_tests.cpp
)

target_include_directories(nemisis_game_menu_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_game_menu_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_game_menu_tests COMMAND nemisis_game_menu_tests)

add_executable(nemisis_ui_canvas_tests
    game/src/ui/UiCanvas.cpp
    tests/ui_canvas_tests.cpp
)

target_include_directories(nemisis_ui_canvas_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_ui_canvas_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_ui_canvas_tests COMMAND nemisis_ui_canvas_tests)

add_executable(nemisis_dev_range_render_scene_tests
    game/src/dev/DebugTarget.cpp
    game/src/dev/DevRangeRenderScene.cpp
    game/src/dev/DevTargetRange.cpp
    game/src/dev/GreyboxWorld.cpp
    game/src/player/PlayerAnimation.cpp
    game/src/player/PlayerFirstPersonRig.cpp
    game/src/player/PlayerView.cpp
    tests/dev_range_render_scene_tests.cpp
)

target_include_directories(nemisis_dev_range_render_scene_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_dev_range_render_scene_tests PRIVATE Novacore::Engine)
target_compile_definitions(nemisis_dev_range_render_scene_tests PRIVATE NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME nemisis_dev_range_render_scene_tests COMMAND nemisis_dev_range_render_scene_tests)
if(NEMISIS_BUILD_PROJECT_FPS_CANDIDATE)
    target_sources(nemisis_dev_range_render_scene_tests PRIVATE game/src/player/PlayerSkeletalAnimator.cpp)
    target_compile_definitions(nemisis_dev_range_render_scene_tests PRIVATE
        NEMISIS_PROJECT_FPS_CANDIDATE_ROOT="${project_fps_candidate_dir}")
    add_dependencies(nemisis_dev_range_render_scene_tests nemisis_project_fps_candidate)
    add_test(NAME nemisis_project_fps_preview_smoke
        COMMAND "${CMAKE_COMMAND}" "-DGAME_EXECUTABLE=$<TARGET_FILE:nemisis_game>"
                "-DSMOKE_ARGUMENT=--smoke-test;--project-fps-preview" -DREQUIRE_PROJECT_FPS=ON
                -DREQUIRE_VULKAN=OFF -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunGameSmoke.cmake")
    set_tests_properties(nemisis_project_fps_preview_smoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 120)
endif()

add_executable(nemisis_player_animation_tests
    game/src/player/PlayerAnimation.cpp
    tests/player_animation_tests.cpp
)

target_include_directories(nemisis_player_animation_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_animation_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_animation_tests COMMAND nemisis_player_animation_tests)

add_executable(nemisis_player_skeletal_animator_tests
    tests/player_skeletal_animator_tests.cpp
    game/src/player/PlayerSkeletalAnimator.cpp
)

target_include_directories(nemisis_player_skeletal_animator_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_compile_definitions(nemisis_player_skeletal_animator_tests
    PRIVATE
        NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(nemisis_player_skeletal_animator_tests PRIVATE Novacore::Engine)
nemisis_copy_mingw_runtime(nemisis_player_skeletal_animator_tests)
add_test(NAME nemisis_player_skeletal_animator_tests COMMAND nemisis_player_skeletal_animator_tests)

add_executable(nemisis_player_first_person_rig_tests
    game/src/player/PlayerAnimation.cpp
    game/src/player/PlayerFirstPersonRig.cpp
    tests/player_first_person_rig_tests.cpp
)

target_include_directories(nemisis_player_first_person_rig_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_first_person_rig_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_first_person_rig_tests COMMAND nemisis_player_first_person_rig_tests)

add_executable(nemisis_render_tuning_tests
    game/src/render/RenderTuning.cpp
    tests/render_tuning_tests.cpp
)

target_include_directories(nemisis_render_tuning_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_render_tuning_tests PRIVATE Novacore::Engine)
target_compile_definitions(nemisis_render_tuning_tests PRIVATE NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME nemisis_render_tuning_tests COMMAND nemisis_render_tuning_tests)

add_executable(nemisis_weapon_simulation_tests
    game/src/weapons/WeaponSimulation.cpp
    tests/weapon_simulation_tests.cpp
)

target_include_directories(nemisis_weapon_simulation_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_weapon_simulation_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_weapon_simulation_tests COMMAND nemisis_weapon_simulation_tests)

add_executable(nemisis_player_command_queue_tests
    game/src/player/PlayerCommandQueue.cpp
    tests/player_command_queue_tests.cpp
)

target_include_directories(nemisis_player_command_queue_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_command_queue_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_command_queue_tests COMMAND nemisis_player_command_queue_tests)

add_executable(nemisis_player_health_tests
    game/src/player/PlayerHealth.cpp
    tests/player_health_tests.cpp
)

target_include_directories(nemisis_player_health_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_health_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_health_tests COMMAND nemisis_player_health_tests)

add_executable(nemisis_asset_catalog_tests
    game/src/assets/GameAssetCatalog.cpp
    tests/asset_catalog_tests.cpp
)

target_include_directories(nemisis_asset_catalog_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_asset_catalog_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_asset_catalog_tests COMMAND nemisis_asset_catalog_tests)

add_executable(nemisis_dev_asset_bindings_tests
    game/src/assets/DevAssetBindings.cpp
    game/src/assets/GameAssetCatalog.cpp
    tests/dev_asset_bindings_tests.cpp
)

target_include_directories(nemisis_dev_asset_bindings_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_dev_asset_bindings_tests PRIVATE Novacore::Engine)
target_compile_definitions(nemisis_dev_asset_bindings_tests PRIVATE
    NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_test(NAME nemisis_dev_asset_bindings_tests COMMAND nemisis_dev_asset_bindings_tests)

add_executable(nemisis_command_messages_tests
    game/src/net/CommandMessages.cpp
    tests/command_messages_tests.cpp
)

target_include_directories(nemisis_command_messages_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_command_messages_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_command_messages_tests COMMAND nemisis_command_messages_tests)

add_executable(nemisis_loopback_command_bridge_tests
    game/src/net/CommandMessages.cpp
    game/src/net/LoopbackCommandBridge.cpp
    game/src/player/PlayerCommandQueue.cpp
    tests/loopback_command_bridge_tests.cpp
)

target_include_directories(nemisis_loopback_command_bridge_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_loopback_command_bridge_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_loopback_command_bridge_tests COMMAND nemisis_loopback_command_bridge_tests)

add_executable(nemisis_prediction_history_tests
    game/src/net/PredictionHistory.cpp
    tests/prediction_history_tests.cpp
)

target_include_directories(nemisis_prediction_history_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_prediction_history_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_prediction_history_tests COMMAND nemisis_prediction_history_tests)

add_executable(nemisis_snapshot_messages_tests
    game/src/net/PredictionHistory.cpp
    game/src/net/SnapshotInterpolation.cpp
    game/src/net/SnapshotMessages.cpp
    tests/snapshot_messages_tests.cpp
)

target_include_directories(nemisis_snapshot_messages_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_snapshot_messages_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_snapshot_messages_tests COMMAND nemisis_snapshot_messages_tests)

add_executable(nemisis_player_spawn_tests
    game/src/player/PlayerSpawn.cpp
    tests/player_spawn_tests.cpp
)

target_include_directories(nemisis_player_spawn_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_spawn_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_spawn_tests COMMAND nemisis_player_spawn_tests)

add_executable(nemisis_dev_sandbox_tests
    game/src/dev/DebugTarget.cpp
    game/src/dev/DevRangeSession.cpp
    game/src/dev/DevSandbox.cpp
    game/src/dev/DevTargetRange.cpp
    game/src/player/PlayerHealth.cpp
    tests/dev_sandbox_tests.cpp
)

target_include_directories(nemisis_dev_sandbox_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_dev_sandbox_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_dev_sandbox_tests COMMAND nemisis_dev_sandbox_tests)

add_executable(nemisis_greybox_world_tests
    game/src/dev/GreyboxWorld.cpp
    tests/greybox_world_tests.cpp
)

target_include_directories(nemisis_greybox_world_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_greybox_world_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_greybox_world_tests COMMAND nemisis_greybox_world_tests)

add_executable(nemisis_greybox_collision_tests
    game/src/dev/GreyboxCollision.cpp
    game/src/dev/GreyboxWorld.cpp
    tests/greybox_collision_tests.cpp
)

target_include_directories(nemisis_greybox_collision_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_greybox_collision_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_greybox_collision_tests COMMAND nemisis_greybox_collision_tests)

add_executable(nemisis_weapon_shot_tests
    game/src/weapons/WeaponShot.cpp
    tests/weapon_shot_tests.cpp
)

target_include_directories(nemisis_weapon_shot_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_weapon_shot_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_weapon_shot_tests COMMAND nemisis_weapon_shot_tests)

add_executable(nemisis_weapon_metrics_tests
    game/src/weapons/WeaponMetrics.cpp
    tests/weapon_metrics_tests.cpp
)

target_include_directories(nemisis_weapon_metrics_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_weapon_metrics_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_weapon_metrics_tests COMMAND nemisis_weapon_metrics_tests)

add_executable(nemisis_weapon_attachments_tests
    game/src/weapons/WeaponAttachments.cpp
    tests/weapon_attachments_tests.cpp
)

target_include_directories(nemisis_weapon_attachments_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_weapon_attachments_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_weapon_attachments_tests COMMAND nemisis_weapon_attachments_tests)

add_executable(nemisis_game_settings_tests
    game/src/settings/GameSettings.cpp
    tests/game_settings_tests.cpp
)

target_include_directories(nemisis_game_settings_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_game_settings_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_game_settings_tests COMMAND nemisis_game_settings_tests)

add_executable(nemisis_user_settings_persistence_tests
    game/src/settings/GameSettings.cpp
    game/src/settings/UserSettingsPersistence.cpp
    game/src/weapons/WeaponAttachments.cpp
    tests/user_settings_persistence_tests.cpp
)

target_include_directories(nemisis_user_settings_persistence_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_user_settings_persistence_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_user_settings_persistence_tests COMMAND nemisis_user_settings_persistence_tests)

add_executable(nemisis_dev_range_session_tests
    game/src/dev/DevRangeSession.cpp
    game/src/player/PlayerHealth.cpp
    tests/dev_range_session_tests.cpp
)

target_include_directories(nemisis_dev_range_session_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_dev_range_session_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_dev_range_session_tests COMMAND nemisis_dev_range_session_tests)

add_executable(nemisis_dev_target_range_tests
    game/src/dev/DebugTarget.cpp
    game/src/dev/DevTargetRange.cpp
    tests/dev_target_range_tests.cpp
)

target_include_directories(nemisis_dev_target_range_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_dev_target_range_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_dev_target_range_tests COMMAND nemisis_dev_target_range_tests)

add_executable(nemisis_player_profile_tests
    game/src/player/PlayerProfile.cpp
    tests/player_profile_tests.cpp
)

target_include_directories(nemisis_player_profile_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_profile_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_profile_tests COMMAND nemisis_player_profile_tests)

add_executable(nemisis_player_view_tests
    game/src/player/PlayerView.cpp
    tests/player_view_tests.cpp
)

target_include_directories(nemisis_player_view_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_view_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_view_tests COMMAND nemisis_player_view_tests)

add_executable(nemisis_player_camera_rig_tests
    game/src/player/PlayerCameraRig.cpp
    game/src/player/PlayerView.cpp
    tests/player_camera_rig_tests.cpp
)

target_include_directories(nemisis_player_camera_rig_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_player_camera_rig_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_player_camera_rig_tests COMMAND nemisis_player_camera_rig_tests)

add_executable(nemisis_debug_target_tests
    game/src/dev/DebugTarget.cpp
    tests/debug_target_tests.cpp
)

target_include_directories(nemisis_debug_target_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_debug_target_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_debug_target_tests COMMAND nemisis_debug_target_tests)

foreach(nemisis_runtime_target IN ITEMS
    nemisis_game
    nemisis_movement_replay_tests
    nemisis_input_command_tests
    nemisis_game_menu_tests
    nemisis_ui_canvas_tests
    nemisis_dev_range_render_scene_tests
    nemisis_player_animation_tests
    nemisis_player_first_person_rig_tests
    nemisis_weapon_simulation_tests
    nemisis_player_command_queue_tests
    nemisis_player_health_tests
    nemisis_asset_catalog_tests
    nemisis_dev_asset_bindings_tests
    nemisis_command_messages_tests
    nemisis_loopback_command_bridge_tests
    nemisis_prediction_history_tests
    nemisis_snapshot_messages_tests
    nemisis_player_spawn_tests
    nemisis_dev_sandbox_tests
    nemisis_greybox_world_tests
    nemisis_greybox_collision_tests
    nemisis_weapon_shot_tests
    nemisis_weapon_metrics_tests
    nemisis_weapon_attachments_tests
    nemisis_game_settings_tests
    nemisis_user_settings_persistence_tests
    nemisis_dev_range_session_tests
    nemisis_dev_target_range_tests
    nemisis_player_profile_tests
    nemisis_player_view_tests
    nemisis_player_camera_rig_tests
    nemisis_debug_target_tests
)
    nemisis_copy_mingw_runtime(${nemisis_runtime_target})
endforeach()

option(NEMISIS_BUILD_OZZ_IK_PROBE "Build the opt-in CPU arm-IK adoption probe" OFF)
option(NEMISIS_ENABLE_HAND_IK "Build opt-in first-person hand constraints (visual acceptance pending)" OFF)
if(NEMISIS_BUILD_OZZ_IK_PROBE OR NEMISIS_ENABLE_HAND_IK)
    include(cmake/OzzAnimation.cmake)
endif()
if(NEMISIS_BUILD_OZZ_IK_PROBE)
    include(cmake/OzzIKProbe.cmake)
endif()
if(NEMISIS_ENABLE_HAND_IK)
    add_library(nemisis_hand_ik STATIC
        game/src/player/PlayerHandConstraints.cpp
        game/src/player/ProjectHandConstraints.cpp)
    target_include_directories(nemisis_hand_ik PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/game/include)
    target_compile_definitions(nemisis_hand_ik PUBLIC NEMISIS_ENABLE_HAND_IK=1)
    target_link_libraries(nemisis_hand_ik PUBLIC Novacore::Engine PRIVATE ozz_animation)
    if(MSVC)
        target_compile_options(nemisis_hand_ik PRIVATE /W4 /permissive-)
    else()
        target_compile_options(nemisis_hand_ik PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    foreach(hand_target IN ITEMS nemisis_game nemisis_player_skeletal_animator_tests)
        target_link_libraries(${hand_target} PRIVATE nemisis_hand_ik)
        nemisis_copy_ozz_notice(${hand_target})
    endforeach()
    if(NEMISIS_BUILD_PROJECT_FPS_CANDIDATE)
        target_link_libraries(nemisis_dev_range_render_scene_tests PRIVATE nemisis_hand_ik)
        nemisis_copy_ozz_notice(nemisis_dev_range_render_scene_tests)
        add_dependencies(nemisis_dev_range_render_scene_tests nemisis_project_rifle_ready)
        add_test(NAME nemisis_project_rifle_ready_tests COMMAND nemisis_dev_range_render_scene_tests --ready-only)
        add_test(NAME nemisis_project_rifle_ready_smoke
            COMMAND "${CMAKE_COMMAND}" "-DGAME_EXECUTABLE=$<TARGET_FILE:nemisis_game>"
                "-DSMOKE_ARGUMENT=--smoke-test;--project-rifle-ready" -DREQUIRE_PROJECT_FPS=ON
                -DREQUIRE_HAND_IK=ON -DREQUIRE_VULKAN=OFF -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunGameSmoke.cmake")
        set_tests_properties(nemisis_project_rifle_ready_smoke PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 120)
        add_test(NAME nemisis_project_rifle_ready_cook_tests
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/project_rifle_ready_cook_tests.py"
                "${project_fps_candidate_dir}")
    endif()

    add_executable(nemisis_player_hand_constraints_tests
        tests/player_hand_constraints_tests.cpp
        game/src/player/PlayerSkeletalAnimator.cpp
        game/src/dev/DevRangeRenderScene.cpp
        game/src/dev/DebugTarget.cpp
        game/src/dev/DevTargetRange.cpp
        game/src/dev/GreyboxWorld.cpp
        game/src/player/PlayerAnimation.cpp
        game/src/player/PlayerFirstPersonRig.cpp
        game/src/player/PlayerView.cpp)
    target_link_libraries(nemisis_player_hand_constraints_tests PRIVATE nemisis_hand_ik)
    target_compile_definitions(nemisis_player_hand_constraints_tests PRIVATE NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
    nemisis_copy_mingw_runtime(nemisis_player_hand_constraints_tests)
    nemisis_copy_ozz_notice(nemisis_player_hand_constraints_tests)
    add_test(NAME nemisis_player_hand_constraints_tests COMMAND nemisis_player_hand_constraints_tests)
    if(NEMISIS_BUILD_PROJECT_FPS_CANDIDATE)
        add_executable(nemisis_project_hand_constraints_tests tests/project_hand_constraints_tests.cpp)
        target_link_libraries(nemisis_project_hand_constraints_tests PRIVATE nemisis_hand_ik)
        target_compile_definitions(nemisis_project_hand_constraints_tests PRIVATE
            NEMISIS_PROJECT_FPS_CANDIDATE_ROOT="${project_fps_candidate_dir}")
        add_dependencies(nemisis_project_hand_constraints_tests nemisis_project_palm_proposal)
        nemisis_copy_mingw_runtime(nemisis_project_hand_constraints_tests)
        nemisis_copy_ozz_notice(nemisis_project_hand_constraints_tests)
        add_test(NAME nemisis_project_hand_constraints_tests COMMAND nemisis_project_hand_constraints_tests)
    endif()
    add_test(NAME nemisis_hand_ik_headless_smoke
        COMMAND ${CMAKE_COMMAND}
            "-DGAME_EXECUTABLE=$<TARGET_FILE:nemisis_game>"
            "-DSMOKE_ARGUMENT=--smoke-test;--first-person-hand-ik;--dev-range"
            -DREQUIRE_VULKAN=OFF -DREQUIRE_HAND_IK=ON
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunGameSmoke.cmake")
    set_tests_properties(nemisis_hand_ik_headless_smoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 120)
endif()

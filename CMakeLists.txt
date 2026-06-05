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
set(NOVACORE_ENGINE_ROOT "" CACHE PATH "Path to a NovaCore Engine checkout")

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
    game/src/dev/DebugTarget.cpp
    game/src/dev/DevSandbox.cpp
    game/src/input/InputBindings.cpp
    game/src/input/InputCommandBuilder.cpp
    game/src/main.cpp
    game/src/movement/MovementConfig.cpp
    game/src/movement/MovementSystem.cpp
    game/src/net/CommandMessages.cpp
    game/src/net/LoopbackCommandBridge.cpp
    game/src/player/PlayerCommandQueue.cpp
    game/src/player/PlayerSpawn.cpp
    game/src/player/PlayerView.cpp
    game/src/weapons/WeaponShot.cpp
    game/src/weapons/WeaponSimulation.cpp
    game/src/weapons/WeaponSystem.cpp
)

target_include_directories(nemisis_game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_game PRIVATE Novacore::Engine)

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

add_executable(nemisis_movement_replay_tests
    game/src/input/InputCommandBuilder.cpp
    game/src/movement/MovementConfig.cpp
    game/src/movement/MovementSystem.cpp
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
    tests/input_command_tests.cpp
)

target_include_directories(nemisis_input_command_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_input_command_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_input_command_tests COMMAND nemisis_input_command_tests)

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
    game/src/dev/DevSandbox.cpp
    tests/dev_sandbox_tests.cpp
)

target_include_directories(nemisis_dev_sandbox_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game/include
)

target_link_libraries(nemisis_dev_sandbox_tests PRIVATE Novacore::Engine)
add_test(NAME nemisis_dev_sandbox_tests COMMAND nemisis_dev_sandbox_tests)

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

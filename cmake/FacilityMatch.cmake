add_library(nemisis_match STATIC
    game/src/dev/OuterFacility.cpp
    game/src/dev/DevTargetRange.cpp
    game/src/dev/DebugTarget.cpp
    game/src/dev/GreyboxCollision.cpp
    game/src/dev/GreyboxWorld.cpp
    game/src/movement/FacilityMotor.cpp
    game/src/movement/MovementSystem.cpp
    game/src/player/PlayerView.cpp
    game/src/player/PlayerHealth.cpp
    game/src/player/PlayerCameraRig.cpp
    game/src/weapons/WeaponMetrics.cpp
    game/src/weapons/WeaponShot.cpp
    game/src/weapons/WeaponSimulation.cpp
    game/src/weapons/WeaponSystem.cpp
    game/src/net/MatchRules.cpp
    game/src/net/MatchCommandLine.cpp
    game/src/net/CommandMessages.cpp
    game/src/net/PredictionHistory.cpp
    game/src/net/SnapshotMessages.cpp
    game/src/net/FacilityMatch.cpp
    game/src/net/FacilityMatchClient.cpp
    game/src/net/FacilityMatchCodec.cpp
)
target_include_directories(nemisis_match PUBLIC game/include)
target_link_libraries(nemisis_match PUBLIC Novacore::Engine Threads::Threads)
target_sources(nemisis_game PRIVATE game/src/net/GameAppMatch.cpp)
target_link_libraries(nemisis_game PRIVATE nemisis_match)

add_executable(nemisis_match_server game/src/net/MatchServerMain.cpp)
target_link_libraries(nemisis_match_server PRIVATE nemisis_match)
nemisis_copy_mingw_runtime(nemisis_match_server)

foreach(suite IN ITEMS match_rules match_codec)
    add_executable(nemisis_${suite}_tests tests/${suite}_tests.cpp)
    target_link_libraries(nemisis_${suite}_tests PRIVATE nemisis_match)
    nemisis_copy_mingw_runtime(nemisis_${suite}_tests)
    add_test(NAME nemisis_${suite}_tests COMMAND nemisis_${suite}_tests)
endforeach()

find_package(Python3 3.10 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME nemisis_local_facility_duel
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/local_facility_duel.py"
            "$<TARGET_FILE:nemisis_game>")
    set_tests_properties(nemisis_local_facility_duel PROPERTIES TIMEOUT 40)
    add_test(NAME nemisis_hosted_match
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/hosted_match.py"
            "$<TARGET_FILE:nemisis_match_server>")
    set_tests_properties(nemisis_hosted_match PROPERTIES TIMEOUT 65)
endif()

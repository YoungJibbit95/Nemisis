if(CMAKE_VERSION VERSION_LESS 3.30)
    message(FATAL_ERROR "The optional ozz 0.17.0 probe requires CMake 3.30 or newer")
endif()

include(FetchContent)
foreach(ozz_option IN ITEMS
    ozz_build_tools ozz_build_fbx ozz_build_gltf ozz_build_data
    ozz_build_samples ozz_build_howtos ozz_build_tests)
    set(${ozz_option} OFF CACHE BOOL "Not needed by the CPU IK probe" FORCE)
endforeach()
if(MSVC AND DEFINED CMAKE_MSVC_RUNTIME_LIBRARY AND NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
    set(ozz_build_msvc_rt_dll OFF CACHE BOOL "Match the parent CRT" FORCE)
else()
    set(ozz_build_msvc_rt_dll ON CACHE BOOL "Match the parent CRT" FORCE)
endif()

# 0.17.0, exact commit and verified source archive. No third-party importer,
# offline tools, windowing or second animation/sampling runtime is selected.
FetchContent_Declare(ozz
    URL https://codeload.github.com/guillaumeblanc/ozz-animation/tar.gz/744eb9d99f606eda849acb0b1204f7a3dc20bca1
    URL_HASH SHA256=a29a8a014b571487d9a6dabebeed57627fc19bf1f0ebf531b0c76384840c3b74
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ozz)

add_executable(nemisis_ozz_arm_ik_probe tools/ozz_arm_ik_probe.cpp)
target_link_libraries(nemisis_ozz_arm_ik_probe PRIVATE Novacore::Engine ozz_animation)
target_compile_definitions(nemisis_ozz_arm_ik_probe PRIVATE NEMISIS_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
nemisis_copy_mingw_runtime(nemisis_ozz_arm_ik_probe)
add_custom_command(TARGET nemisis_ozz_arm_ik_probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/licenses/ozz-animation-MIT.txt
        $<TARGET_FILE_DIR:nemisis_ozz_arm_ik_probe>/ozz-animation-MIT.txt
    VERBATIM
)
add_test(NAME nemisis_ozz_arm_ik_probe COMMAND nemisis_ozz_arm_ik_probe)

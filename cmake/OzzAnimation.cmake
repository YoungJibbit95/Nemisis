include_guard(GLOBAL)

if(CMAKE_VERSION VERSION_LESS 3.30)
    message(FATAL_ERROR "The optional ozz 0.17.0 IK requires CMake 3.30 or newer")
endif()

include(FetchContent)
foreach(ozz_option IN ITEMS
    ozz_build_tools ozz_build_fbx ozz_build_gltf ozz_build_data
    ozz_build_samples ozz_build_howtos ozz_build_tests)
    set(${ozz_option} OFF CACHE BOOL "Not needed by hand IK" FORCE)
endforeach()
if(MSVC AND DEFINED CMAKE_MSVC_RUNTIME_LIBRARY AND NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
    set(ozz_build_msvc_rt_dll OFF CACHE BOOL "Match the parent CRT" FORCE)
else()
    set(ozz_build_msvc_rt_dll ON CACHE BOOL "Match the parent CRT" FORCE)
endif()

# ozz enables CMAKE_COMPILE_WARNING_AS_ERROR inside its own build scripts.
# Newer GCC releases report a false-positive -Wdangling-reference inside
# skeleton_utils.cc, which turns the pinned third-party source into a hard build
# failure before any Nemisis code is compiled. Keep warnings-as-errors off for
# the fetched dependency only; first-party targets still use their own flags.
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)

# 0.17.0, exact commit and verified source archive. No third-party importer,
# offline tools, windowing or second animation/sampling runtime is selected.
FetchContent_Declare(ozz
    URL https://codeload.github.com/guillaumeblanc/ozz-animation/tar.gz/744eb9d99f606eda849acb0b1204f7a3dc20bca1
    URL_HASH SHA256=a29a8a014b571487d9a6dabebeed57627fc19bf1f0ebf531b0c76384840c3b74
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ozz)

# Belt and braces: the fetched project sets the property again after the cache
# variable above, so drop the resulting -Werror from its own targets.
if(NOT MSVC)
    foreach(ozz_target IN ITEMS ozz_animation ozz_base)
        if(TARGET ${ozz_target})
            target_compile_options(${ozz_target} PRIVATE -Wno-error -Wno-dangling-reference)
        endif()
    endforeach()
endif()

function(nemisis_copy_ozz_notice target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/licenses/ozz-animation-MIT.txt
            $<TARGET_FILE_DIR:${target_name}>/ozz-animation-MIT.txt
        VERBATIM
    )
endfunction()

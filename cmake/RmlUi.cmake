# The shipping build remains dependency-light until the UI migration is accepted.
# Enable with -DNEMISIS_ENABLE_RMLUI=ON. The native renderer is provided by NovaCore.
option(NEMISIS_ENABLE_RMLUI "Build the native RmlUi showcase" OFF)
if(NEMISIS_ENABLE_RMLUI)
    get_target_property(ui_engine_definitions Novacore::Engine INTERFACE_COMPILE_DEFINITIONS)
    if(NOT "NOVACORE_UI_GEOMETRY_API=1" IN_LIST ui_engine_definitions)
        message(FATAL_ERROR "RmlUi requires the separate NovaCore UI geometry adapter revision. See docs/ui/UI_DESIGN_SYSTEM.md")
    endif()
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF)
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
    FetchContent_Declare(freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG 42608f77f20749dd6ddc9e0536788eaad70ea4b5)
    FetchContent_MakeAvailable(freetype)
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()
    set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
    set(RMLUI_PRECOMPILED_HEADERS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(rmlui
        GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
        GIT_TAG 58c751531f4c1a1eb2dc56f409b8e8a323a0c9ad)
    FetchContent_MakeAvailable(rmlui)
    add_executable(nemisis_ui_showcase_tests tests/ui_showcase_tests.cpp
        game/src/ui/UiShowcase.cpp game/src/ui/RmlRenderBridge.cpp game/src/input/InputBindings.cpp)
    target_include_directories(nemisis_ui_showcase_tests PRIVATE game/include)
    target_link_libraries(nemisis_ui_showcase_tests PRIVATE RmlUi::Core Novacore::Engine)
    nemisis_copy_mingw_runtime(nemisis_ui_showcase_tests)
    add_test(NAME nemisis_ui_showcase_tests COMMAND nemisis_ui_showcase_tests)
    set_tests_properties(nemisis_ui_showcase_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" LABELS "ui;cpu")
    foreach(ui_size IN ITEMS 1280x720 1920x1080 2560x1440)
        add_test(NAME nemisis_ui_vulkan_${ui_size}
            COMMAND ${CMAKE_COMMAND} "-DGAME_EXE=$<TARGET_FILE:nemisis_game>"
                "-DGAME_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DUI_SIZE=${ui_size}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/UiShowcaseSmoke.cmake")
        set_tests_properties(nemisis_ui_vulkan_${ui_size} PROPERTIES LABELS "ui;gpu;vulkan" TIMEOUT 60)
    endforeach()
    target_link_libraries(nemisis_game PRIVATE RmlUi::Core)
    target_compile_definitions(nemisis_game PRIVATE NEMISIS_HAS_RMLUI=1)
    target_sources(nemisis_game PRIVATE game/src/ui/UiShowcase.cpp game/src/ui/RmlRenderBridge.cpp)
else()
    target_compile_definitions(nemisis_game PRIVATE NEMISIS_HAS_RMLUI=0)
endif()

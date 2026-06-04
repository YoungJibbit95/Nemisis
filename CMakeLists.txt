cmake_minimum_required(VERSION 3.27)

project(Nemesis
    VERSION 0.1.0
    DESCRIPTION "6v6 Arena Shooter - Nemesis Game"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# For now, include the engine from parent or external location
# This assumes NovaCore is available in the build environment
find_package(novacore_engine REQUIRED)

option(NEMESIS_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(NEMESIS_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(NEMESIS_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# Main game executable
add_executable(nemesis_game game/src/main.cpp)
target_link_libraries(nemesis_game PRIVATE novacore_engine)

enable_testing()

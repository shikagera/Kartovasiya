cmake_minimum_required(VERSION 3.20)
project(Kartovasiya LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
    add_compile_options(/utf-8 /W3 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

# ── raylib ────────────────────────────────────────────────────────────────
# 1) vendored copy in external/raylib (git clone --branch 5.5)
# 2) otherwise download it once via FetchContent
set(RAYLIB_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/external/raylib")
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RMODELS OFF CACHE BOOL "" FORCE)

if(EXISTS "${RAYLIB_LOCAL}/CMakeLists.txt")
    message(STATUS "Kartovasiya: using vendored raylib at ${RAYLIB_LOCAL}")
    add_subdirectory("${RAYLIB_LOCAL}" raylib_build EXCLUDE_FROM_ALL)
else()
    message(STATUS "Kartovasiya: fetching raylib 5.5 from GitHub")
    include(FetchContent)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(raylib)
endif()

# ── game ──────────────────────────────────────────────────────────────────
file(GLOB_RECURSE KV_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(Kartovasiya ${KV_SOURCES})
target_include_directories(Kartovasiya PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(Kartovasiya PRIVATE raylib)

if(WIN32)
    target_link_libraries(Kartovasiya PRIVATE winmm)
    # release build = no console window
    set_target_properties(Kartovasiya PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_link_options(Kartovasiya PRIVATE
            "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>"
            "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>")
    endif()
endif()

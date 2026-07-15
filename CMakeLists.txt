cmake_minimum_required(VERSION 3.16)
project(stable_fluids LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put binaries in a predictable place.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Dependencies: raylib via FetchContent (no system install required).
# ---------------------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------------
# Application target.
# ---------------------------------------------------------------------------
add_executable(stable_fluids
    src/app/main.cpp
    src/solver/StamSolver.cpp
    src/render/Renderer.cpp
)

target_include_directories(stable_fluids PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(stable_fluids PRIVATE raylib)

# raylib needs these on Windows; harmless elsewhere.
if (WIN32)
    target_link_libraries(stable_fluids PRIVATE winmm)
endif()

if (MSVC)
    target_compile_options(stable_fluids PRIVATE /O2 /W3)
else()
    target_compile_options(stable_fluids PRIVATE -O2 -Wall -Wextra)
endif()

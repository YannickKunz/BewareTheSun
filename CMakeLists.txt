cmake_minimum_required(VERSION 3.11)

# Project Name
project(RenoDeAragon)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Raylib
find_package(raylib REQUIRED)

# Include Directories
include_directories(src)
include_directories(src/core)
include_directories(src/entities)
include_directories(src/world)

# Source Files
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/entities/Player.cpp
    src/entities/Spider.cpp
    src/entities/Roach.cpp
    src/world/Level.cpp
    src/world/LevelGenerator.cpp
    src/gfx/ProcArt.cpp
)

# Create Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link Libraries
target_link_libraries(${PROJECT_NAME} raylib)

# Copy Assets (Optional but recommended)
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

# Enforce uniform compilation
cmake_minimum_required(VERSION 4.3)

project(wheelTime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Let each subdir handle its own making
add_subdirectory(platform)
add_subdirectory(app)
add_subdirectory(launcher)


# Enforce uniform compilation
cmake_minimum_required(VERSION 4.3)

project(WheelTime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
qt_standard_project_setup()

# Let each subdir handle its own making
add_subdirectory(platform)
add_subdirectory(app)
add_subdirectory(launcher)


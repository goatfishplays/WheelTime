cmake_minimum_required(VERSION 4.3)
project(wheelTime)
add_executable(${PROJECT_NAME} WIN32 src/main.cpp)
# add_library(wheelLib src/lib.cpp) # static lib
# add_library(wheelLib SHARED src/lib.cpp) # dynamic lib

target_link_libraries(${PROJECT_NAME} wheelLib)

add_subdirectory(lib)
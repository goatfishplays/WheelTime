add_library(wheelLib lib.cpp) # static lib

target_include_directories(wheelLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
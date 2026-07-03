add_library(App 
    src/App.cpp
)

target_include_directories(App 
    PUBLIC 
        include
)

target_link_libraries(App
    PUBLIC 
        Platform
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
)

# set_target_properties(helloworld PROPERTIES
#     WIN32_EXECUTABLE ON
#     MACOSX_BUNDLE ON
# )
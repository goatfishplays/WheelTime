add_library(App 
    src/App.cpp
    src/HoverButton.cpp
    src/RadialMenuWidget.cpp
    src/Gui.cpp
    src/SettingsWindow.cpp
    src/Menu.cpp
    src/MenuConfigLoader.cpp
    src/Action.cpp
    src/ActionItems.cpp
    include/App/App.hpp
    include/App/HoverButton.hpp
    include/App/RadialMenuWidget.hpp
    include/App/Gui.hpp
    include/App/SettingsWindow.hpp
    include/App/Menu.hpp
    include/App/MenuConfigLoader.hpp
    include/App/Action.hpp
    include/App/ActionItems.hpp
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

add_library(App 
    src/App.cpp
    include/App/App.hpp
    src/HoverButton.cpp
    include/App/HoverButton.hpp
    src/RadialMenuWidget.cpp
    include/App/RadialMenuWidget.hpp
    src/Gui.cpp
    include/App/Gui.hpp
    src/Menu.cpp
    include/App/Menu.hpp
    src/Action.cpp
    include/App/Action.hpp
    src/ActionItems.cpp
    include/App/ActionItems.hpp
    src/ActionExecutionContext.cpp
    include/App/ActionExecutionContext.hpp
    src/ExecuteResult.cpp
    include/App/ExecuteResult.hpp
    src/MenuConfigLoader.cpp
    include/App/MenuConfigLoader.hpp
    src/SettingsWindow.cpp
    include/App/SettingsWindow.hpp
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

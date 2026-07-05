qt_add_executable(${PROJECT_NAME} 
    src/main.cpp
)
# add_executable(Launcher
#     src/main.cpp
# )

# target_include_directories(App 
#     PUBLIC 
#         include
# )

target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        App 
        Qt6::Widgets
        # Qt6::Core
)  
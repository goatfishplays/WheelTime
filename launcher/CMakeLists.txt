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

# Leverage modern Qt6 resource compilation
qt_add_resources(${PROJECT_NAME} "target"
    PREFIX "/styles"
    BASE "${CMAKE_SOURCE_DIR}/resources/styles"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/styles/defaultWheel.qss"
)
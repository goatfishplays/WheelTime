add_executable(${PROJECT_NAME} 
    src/main.cpp
)

# target_include_directories(App 
#     PUBLIC 
#         include
# )

target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        App
)
add_library(Platform)

# Interface code
target_include_directories(Platform
    PUBLIC
        include
)

# OS specific code
if(WIN32)
    # add_subdirectory(src/platformWindows)
    target_sources(Platform PRIVATE
        src/windows/Window.cpp
        src/windows/Execute.cpp
        src/windows/Inputs.cpp
    )
    target_link_libraries(Platform PUBLIC ws2_32 winhttp)
elseif(UNIX) # for now this is not implemented but just for later reference
    target_sources(Platform PRIVATE
        src/linux/Window.cpp
        src/linux/Execute.cpp
        src/linux/Inputs.cpp
    )
endif()
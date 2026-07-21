# Enforce uniform compilation
cmake_minimum_required(VERSION 4.3)

project(WheelTime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

# add qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Network)
qt_standard_project_setup()

# Let each subdir handle its own making
add_subdirectory(platform)
add_subdirectory(app)
add_subdirectory(launcher)

install(FILES "${CMAKE_SOURCE_DIR}/config/default_menu.json" DESTINATION config)

if(WIN32)
    get_target_property(_qt_qmake_location Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${_qt_bin_dir}"
        DOC "Qt deployment tool used to bundle runtime DLLs and plugins"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            set(_wheel_time_exe \"\${CMAKE_INSTALL_PREFIX}/WheelTime.exe\")
            if(EXISTS \"\${_wheel_time_exe}\")
                message(STATUS \"Deploying Qt runtime for \${_wheel_time_exe}\")
                execute_process(
                    COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --no-translations \"\${_wheel_time_exe}\"
                    TIMEOUT 180
                    RESULT_VARIABLE _windeployqt_result
                )
                if(NOT _windeployqt_result EQUAL 0)
                    message(FATAL_ERROR \"windeployqt failed with exit code \${_windeployqt_result}\")
                endif()
            endif()
        ")
    else()
        message(WARNING "windeployqt was not found. Installed packages may require Qt on PATH.")
    endif()
endif()

set(CPACK_PACKAGE_NAME "WheelTime")
set(CPACK_PACKAGE_VENDOR "WheelTime")
set(CPACK_PACKAGE_CONTACT "WheelTime")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A quick radial launcher for Windows actions and macros")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_FILE_NAME "WheelTime-portable-windows")
set(CPACK_GENERATOR "ZIP")

if(WIN32)
    set(CPACK_PACKAGE_EXECUTABLES "WheelTime" "WheelTime")
    set(CPACK_CREATE_DESKTOP_LINKS "WheelTime")
    set(CPACK_NSIS_DISPLAY_NAME "WheelTime")
    set(CPACK_NSIS_PACKAGE_NAME "WheelTime")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "WheelTime")
    set(CPACK_NSIS_INSTALL_ROOT "$LOCALAPPDATA")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "WheelTime.exe")
endif()

include(CPack)


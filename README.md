# Wheel Time

Wheel Time is a real-time application launcher designed to help users quickly open programs, scripts, or macros without leaving their current workflow. The main target users are gamers and general computer users who want quick access to common actions without memorizing many hotkeys.

## Project Goals

- Create a functional launcher interface
- Allow users to launch programs and scripts
- Support quick open/close behavior for low interruption
- Keep the interface lightweight and responsive

## MVP

The MVP is complete when:

- The launcher can be opened with a hotkey
- The launcher displays a visual list of actions
- The user can launch at least one external program or script
- The user can close the launcher quickly
- The launcher returns focus to the previous application
- The app builds and runs on at least two team members' Windows machines
- Basic usage is documented in the README

## Tech Stack

- Language: C++
- Core Libraries: Windows API, Qt
- Build System: CMake
- Development Tools: VSCode, Cursor, Git/GitHub, Doxygen

## Sprint 1 Goal

Create a basic Windows launcher prototype with a simple interface.

## Sprint 1 Tasks

- Set up GitHub repository
- Set up C++/CMake project
- Add README/build steps
- Research app launching in C++
- Research Windows API basics
- Research GUI/window options
- Create basic launcher window

## Sprint 2 Goal

Allow the launcher to open and close quickly using a hotkey and run basic actions.

## Sprint 2 Tasks

- Research Windows API `RegisterHotKey`
- Implement global hotkey to open launcher
- Add Escape key listener to close launcher
- Research window focus behavior
- Restore focus to previous app after launcher closes
- Create placeholder macro/script action
- Connect GUI buttons to app launching actions
- Test launching Notepad and Calculator from GUI
- Add save/load functions for menu actions
- Improve mouse events for radial wheel selection
- Test launcher behavior over full-screen apps
- Log bugs and open issues in GitHub
- Update README with hotkey usage instructions

## Build Instructions

### Requirements

- Windows
- CMake 4.3+
- Visual Studio Build Tools 2022 with Desktop development for C++
- Qt 6 MSVC 2022 64-bit

### Configure and Build

From the project root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"
cmake --build build --config Debug
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
- Core Libraries: Windows API
- Build System: CMake
- Development Tools: VSCode, Cursor, Git/GitHub

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

## Build Instructions

To be updated once the CMake project structure is finalized.

Expected general steps:

```bash
git clone <repo-url>
cd Wheel-Time
mkdir build
cd build
cmake ..
cmake --build .
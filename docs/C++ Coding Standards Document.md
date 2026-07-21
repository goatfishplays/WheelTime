# C++ Coding Standards

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Date:** 7/21/26  

## Purpose

This document defines the coding standards for the Wheel Time C++ codebase. The goal is to keep the code readable, consistent, and easy for all team members to understand and maintain.

---

## Header Files

- Every `.cpp` file should usually have an affiliated `.hpp` file with the same name.
- Small files such as `main.cpp` do not need a matching header file if they only contain program startup logic.
- Header files should contain class declarations, function declarations, constants, and shared types.
- Source files should contain function implementations and internal helper logic.
- Use include guards or `#pragma once` in every header file.
- Avoid putting large function implementations directly in header files unless there is a clear reason.

## Include Order

Use this order for include statements:

1. Related header file.
2. Standard C++ library headers.
3. Third-party library headers, such as Qt.
4. Windows API headers.
5. Project headers.

Example:

    #include "ActionExecutor.hpp"

    #include <iostream>
    #include <string>
    #include <vector>

    #include <QApplication>
    #include <QWidget>

    #include <windows.h>

    #include "AppAction.hpp"

## Indentation

- Use spaces for indentation.
- Use **2 spaces** per indentation level.
- Do not use tabs.
- Keep formatting consistent across all files.

## Naming

- Use descriptive names for variables, functions, classes, and files.
- Class names should use **PascalCase**.
  - Example: `ActionExecutor`, `RadialMenuWidget`
- Function and variable names should use **camelCase**.
  - Example: `launchApplication()`, `selectedAction`
- Constants should use **UPPER_CASE**.
  - Example: `DEFAULT_HOTKEY`, `MAX_ACTION_COUNT`
- File names should match the main class or feature when possible.
  - Example: `ActionExecutor.hpp`, `ActionExecutor.cpp`

## Functions

- Return type, function name, and parameters should be on the same line when practical.
- Keep functions focused on one main responsibility.
- Avoid very long functions. If a function becomes hard to read, split it into smaller helper functions.
- Use short but clear parameter names.
- Add a brief comment before important function declarations explaining the purpose, inputs, and output.

Example:

    // Launches an external application using the provided file path.
    // Returns true if the launch request succeeds.
    bool launchApplication(const std::string& filePath);

## Local Variables

- Declare variables as close to their first use as possible.
- Place each variable in the narrowest reasonable scope.
- Initialize variables when they are declared.
- Use `const` when a variable should not change.
- Avoid reusing one variable for multiple unrelated purposes.

Example:

    const std::string appPath = action.getPath();
    bool launched = executor.launchApplication(appPath);

## Braces

- Always use braces for `if`, `else`, `for`, `while`, and `do-while` statements.
- Use braces even when the body has only one line.

Good:

    if (isVisible) {
      hideLauncher();
    }

Avoid:

    if (isVisible)
      hideLauncher();

## Line Length

- Keep lines under **100 characters** whenever practical.
- Break long function calls or conditionals across multiple lines.
- Avoid making code harder to read just to force a line under 100 characters.

## Comments

- Write comments that explain **why** code exists, not what every obvious line does.
- Keep comments short and useful.
- Remove outdated comments when code changes.
- Use TODO comments only when the issue is real and should be handled later.

Example:

    // Restore focus so the user can return to the game after closing the launcher.
    restorePreviousWindowFocus();

## Magic Numbers

- Avoid hardcoded numeric values.
- Use named constants or `constexpr` variables instead.
- Constants should be placed near the code that uses them unless they are shared across files.

Example:

    constexpr int MENU_RADIUS = 140;
    constexpr int MAX_VISIBLE_ACTIONS = 8;

## Classes

- Keep data members private unless there is a strong reason not to.
- Keep public interfaces concise and easy to understand.
- Prefer small classes with clear responsibilities.
- Avoid mixing UI code, action execution logic, and settings logic in the same class.
- Use constructors to put objects into a valid starting state.

## Error Handling

- Check for failure cases when launching programs, opening files, saving settings, or registering hotkeys.
- Do not allow invalid user input to crash the application.
- Return clear success/failure values where useful.
- Log or document important failures during testing.

Example:

    if (!executor.launchApplication(filePath)) {
      // TODO: Show user-facing error message in settings/action menu.
      return false;
    }

## Qt / GUI Code

- Keep UI layout code separate from action execution logic when possible.
- UI button handlers should call into action/executor classes instead of directly containing all logic.
- Keep widget names descriptive.
- Keep the launcher interface lightweight and responsive.

## Windows API Code

- Keep Windows API-specific logic isolated when possible.
- Add short comments for Windows API calls that are not obvious.
- Check return values from Windows API functions such as hotkey registration, focus handling, and process launching.
- Avoid spreading platform-specific code throughout the entire project.

## Configuration and Settings

- User-configurable actions should be saved and loaded through a clear settings/configuration system.
- Avoid requiring users to edit code to add or change launcher actions.
- Validate paths, hotkeys, and action names before saving.
- Handle missing or invalid configuration files safely.

## Testing

- Manual tests should be documented in the test plan/report.
- Automated tests should be placed in the `app/tests` directory.
- Add unit tests for logic that can be tested without the GUI.
- Useful test areas include:
  - Action execution.
  - Action history.
  - Search/fuzzy matching.
  - Scheduler behavior.
  - Settings validation.
  - Invalid action paths.
- Before merging major changes, team members should build and run the project locally.

## Git and Pull Requests

- Use clear branch names when possible.
  - Example: `feature/search-palette`, `fix/hotkey-close`
- Keep pull requests focused on one main change.
- Write clear commit messages that describe what changed.
- Do not commit build artifacts, temporary files, or personal IDE files.
- Review code before merging into the main branch.
- Make sure the project builds before merging major code changes.

## Formatting

- Leave one blank line between function definitions.
- Do not leave trailing whitespace at the end of lines.
- Keep spacing around operators consistent.
- Keep related code grouped together.
- Run formatting or manually clean up code before submitting a pull request.

## Documentation

- Update the README when build steps, dependencies, or usage instructions change.
- Document major design decisions in the repository.
- Keep comments and documentation consistent with the current code.
- Remove outdated notes once they no longer apply.

## Summary

The main goal of these standards is consistency. Team members should write C++ code that is readable, modular, and easy to maintain. When in doubt, choose the style that makes the code easier for another teammate to understand and build on.

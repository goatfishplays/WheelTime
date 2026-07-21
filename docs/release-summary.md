# Release Summary

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Date:** 7/22/2026  

## Overview

Wheel Time Release 1.0 is a lightweight real-time application launcher designed to help users quickly open actions, scripts, programs, and hotkeys from a visual wheel interface. The main goal of the release is to give users one fast interface for running common actions without needing to leave their current workflow, remember many hotkeys, or search through the desktop.

The release focuses on Windows functionality, including a launcher overlay, hotkey activation, visual action selection, configurable actions, search-based launching, and basic automation/action execution.

---

## Key User Stories and Acceptance Criteria

### User Story 1: Launch Programs and Scripts Quickly

**User Story:**  
As a computer user, I want to be able to launch programs and execute scripts quickly so I can run things that I use often without having to exit to desktop or type anything in a terminal.

**Acceptance Criteria:**

- The team has researched at least two C++ methods for launching external programs or scripts.
- The team has compared system(), ShellExecute, and CreateProcess.
- The team has a working test or documented example for launching a basic Windows app such as Notepad or Calculator.
- The recommended launching approach is documented in the repository.
- The chosen approach is realistic for the MVP and can be built on in future sprints.

---

### User Story 2: Interface Does Not Obscure the Entire Screen

**User Story:**  
As a gamer, I want the interface to not obscure the entire screen so I can still see what is happening while performing actions.

**Acceptance Criteria:**

- The launcher interface has a basic window or GUI skeleton.
- The GUI does not take up the full screen.
- The interface includes placeholder buttons or visible UI elements for future launcher actions.
- The team has documented research on transparency, overlay behavior, or ways to avoid blocking the screen.
- The GUI prototype can be shown through code, a screenshot, or a design sketch.
- The interface is simple enough to be expanded in future sprints.

---

### User Story 3: Bring Up the Interface Quickly

**User Story:**  
As a gamer, I want to be able to bring up the interface quickly so I don't have to interrupt my playing to access an action.

**Acceptance Criteria:**

- The team has implemented a global hotkey listener using the Windows API.  
- Pressing the designated hotkey successfully renders the launcher interface on the screen.  
- The launcher overlay successfully renders on top of other active, full-screen applications.

---

### User Story 4: Exit the Interface Quickly

**User Story:**  
As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

**Acceptance Criteria:**

- The user can instantly close or hide the launcher by pressing the Escape key.  
- The launcher completely disappears from the screen when closed.  
- Upon closing, the user's mouse position is restored, and the operating system immediately returns window focus to the previously active application.

---

### User Story 5: Run Actions from a Visual Interface

**User Story:**  
As a computer user, I want to be able to run macros/hotkeys from a visual interface so I don't have to remember all of them.

**Acceptance Criteria:**

-  The codebase utilizes an abstraction layer (e.g. AppAction) so execution logic is not hardcoded directly inside UI button handlers.  
- The user can successfully launch at least two test applications (Notepad and Calculator) by clicking buttons in the UI.  
- The README is updated with instructions on how to use the hotkeys and trigger these actions.

---

### User Story 6: Settings Menu

**User Story:**  
As a general computer user, I want to have a settings menu so I can easily control how the app works and add new buttons without having to modify a text file.

**Acceptance Criteria:**

- The user can open a settings menu from the launcher.
- The settings menu allows the user to add a new action.
- The user can add the new action to the menu.
- The user can save settings changes.
- After saving, the new action is visible when the launcher menu is opened again.
- The user does not need to directly modify code or a text file to add the action.

---

### User Story 7: Customize Launcher Actions

**User Story:**  
As a user, I want to customize my launcher actions so I can choose the programs/scripts I use most often.

**Acceptance Criteria:**

- The user can open the settings/action management menu.
- The user can add a new launcher action without editing code directly.
- The user can choose a program or script for the custom action.
- The custom action can be saved to the user’s launcher configuration.
- The custom action appears in the launcher menu after it is saved.
- The user can run the custom action from the launcher interface.
- The user can delete or remove a custom action from the launcher.
- The system handles invalid or missing program/script paths without crashing.

---

### User Story 8: Search-Based Action Launcher

**User Story:**  
As a general user, I want to be able to run any action from somewhere so if I forget what menu it is associated with I can still use it.

**Acceptance Criteria:**

- The user can open the launcher menu.
- The user can select a search action item or search mode.
- The user can search for an existing action by name or keyword.
- Matching actions are displayed or selectable.
- Selecting a search result runs the correct action.
- Search works for at least two action types, such as app launching and hotkey/key simulation.
- The search behavior is documented in the repository.

---

### User Story 9: Lightweight Interface

**User Story:**  
As a gamer, I want the interface to be lightweight so I can run it without worrying about lowering my FPS.

**Acceptance Criteria:**

- Wheel Time can run in the background without noticeable slowdown.
- The launcher can open and close repeatedly without freezing.
- Running common actions does not noticeably slow the system.
- The interface remains responsive during normal use.
- Major performance concerns or bugs are documented.
- The codebase is cleaned up according to the team’s coding standards.
- Final documents are updated for the release.

---

## Known Problems

The following known problems, limitations, and design shortcuts are part of the Release 1.0 version.

### Windows-Focused Release

Wheel Time Release 1.0 mainly focuses on Windows functionality. Some user stories mention Linux/general launcher behavior, but full Linux support is not complete in this release. As such there is some code that may need to be moved to the platform layer or changed to support such environments(in particular search and the non-activating overlay code).

### Limited Automated GUI Testing

Most GUI and user-interaction behavior was tested manually. Automated tests exist for core logic such as action history, scheduler behavior, search palette behavior, and action item logic, but full end-to-end GUI automation is not complete.

### Edge Cases for Invalid Actions

The system is expected to handle invalid or failed actions safely, but more testing is needed for unusual file paths, missing programs, permission issues, unsupported scripts, and failed executable launches.

### Hotkey and Focus Edge Cases

The launcher supports hotkey behavior, but edge cases may still exist depending on the active application, full-screen mode, Windows permissions, or focus behavior. Some games or protected applications may block overlays or input simulation. The Windows key currently does not work for menu activation due to Windows built in hotkeys being hard to override. Simulated key inputs may be inconsistent when physically holding modifiers alongside them because of the way they are handled by the OS. Settings menu may steal focus due to needing keyboard inputs leading to needing to manually refocus your active application after.

### Search Limitations

Search-based launching works for expected test cases, but search quality may still need improvement for fuzzy matching, ranking, missing programs, duplicate action names, or unsupported application paths. Certain apps such as Steam have their own methods of exposing executables and not all of these methods may have been covered. Applications in your downloads folder or other non-standard install locations are currently not checked for.

### Settings Validation

The settings menu supports adding and saving actions, but more validation could be added for incorrect user input, duplicate actions, invalid hotkey bindings, and unsafe file selections.

### Design Shortcuts

Some test actions, default actions, or example programs may still be hardcoded or preconfigured for demonstration purposes. More work is needed to make all actions fully user-configurable and polished.

### Performance Testing Limitations

Basic performance and responsiveness were tested, but deeper benchmark testing is limited. More testing would be needed with actual games, high-load applications, and longer runtime sessions.

### Heavily Suggested to Set Exit on Action to True for Mouse Inputs

(This one is debatable whether it should be here or in non-issues) Currently due to the way Windows handles mouse inputs we cannot pass through simulated mouse clicks to the underlying application while the menu is open. This may result in infinite action loops caused by actions triggered by actions. This may be intentional and useful, allowing for infinitely running/self repeating macros but may also be accidental. It is recomended to ensure you exit the menu before simulating a left click if you do not want this behavior.

---

## Non-Issues
Some behaviors may seem unintuitive but are by design and are not considered problems.

### Cancel Requires Main Action to be Running

Some actions such as simulating keystrokes fire off a separate subaction for cleanup(running a keyup in this case). The main action when set to "Continue Immediately" will finish and thus be uncancelable, this is by design but may be unintuitive to some users, if you need the action to remain cancelable it is recommended to either unset "Continue Immediately" or add a delay to the end of the action that spans the excess duration.

### Actions Attempt Pause in Settings
Actions that contain time delays or durations will attempt to pause their delays and durations while the settings menu is open and may be canceled if the settings are updated while there to attempt to avoid invalid states.

### Menus Locked in Settings
Menus are locked while the settings menu is open. This is to prevent the user from not explicitly saving or canceling their changes. 

---

## Product Backlog

The following high-priority user stories and bug fixes should guide future work on Wheel Time.

### High-Priority User Stories

1. **Improve settings validation**  
   As a user, I want the settings menu to clearly reject invalid input so I do not accidentally create broken launcher actions.

2. **Improve custom action management**  
   As a user, I want to edit, delete, reorder, and rename launcher actions so I can fully control my wheel layout.

3. **Superior ricing support**  
   As a user, I want be able to customize the graphics so it can better fit with the rest of my system theming.

4. **Add stronger error messages**  
   As a user, I want clear error messages when an action fails so I know what went wrong and how to fix it.

5. **Expand automated testing and allow action item plugins**  
   As a developer, I want more automated tests and extensibility for settings, action validation, action items, and launcher behavior so future changes are safer and I can add custom behaviors.

6. **Improve full-screen application support**  
   As a gamer, I want Wheel Time to work reliably over full-screen and borderless-window applications so it is useful while gaming regardless of game.

7. **Improve performance testing**  
   As a gamer, I want benchmark results showing Wheel Time has low impact on system performance so I can trust it during games.

8. **Add full Linux support**  
   As a Linux user, I want Wheel Time to support Linux program launching and input behavior so I can use the same launcher outside Windows.

9. **Add action import/export**  
   As a user, I want to back up and share my launcher configurations so I can move them between devices, share them with friends, or restore them later.

10. **Polish UI and accessibility**  
   As a user, I want the default launcher to be visually clear, easy to read, customizable, and comfortable to use so it feels like a finished application.

11. **Extended input simulation**
    As a user, I want to be able to simulate any input(midi, mouse scrolls, mouse paths) from the launcher so I can perform a broad range of actions required by my specific software.

12. **Macro recording**
    As a NVim user, I want to be able to record macros at use time so I don't have to open settings to create a macro that I will only be using for a short time.

13. **Running actions display**
    As a user, I want to have an optional visual of what actions are currently running or queued so I can easier tell what is happening in the background.

14. **Deadzone/Custom Layouts**
    As a user, I want to be able to set deadzones and custom button layouts beyond the original wheel so I can pack more actions into 1 menu.

15. **Socket Receiver**
    As a user, I want to be able to receive socket messages and automatically execute actions in response so I can make use of the replies generated when sending out socket messages.

---

## High-Priority Bug Fixes

- Fix any cases where the launcher fails to appear above certain active windows.
- Fix any cases where focus does not return properly after closing the launcher.
- Fix invalid action paths causing failed launches.
- Fix search returning incorrect or poorly ranked results.
- Fix settings changes not saving or not appearing immediately in the launcher.
- Fix any UI scaling issues on different screen sizes or resolutions.
- Fix performance problems caused by repeated open/close behavior or long-running background tasks.

---

## Release Status

Wheel Time Release 1.0 is ready for project review with known limitations. The release supports the core project goal: a lightweight visual launcher that can open quickly, close quickly, run actions, support configurable behavior, and search for programs/actions.

The most important follow-on work is improving validation, polishing the settings/action workflow, expanding automated testing, and strengthening support for full-screen applications and edge cases.

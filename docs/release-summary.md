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

- The user can open the launcher interface.
- The user can select an action that launches an external program.
- The system can successfully launch test applications such as Notepad, Calculator, or Firefox.
- The launcher remains stable after launching an external program.
- Program launching is handled through the action system rather than being hardcoded only for one specific app.

---

### User Story 2: Interface Does Not Obscure the Entire Screen

**User Story:**  
As a gamer, I want the interface to not obscure the entire screen so I can still see what is happening while performing actions.

**Acceptance Criteria:**

- The launcher appears as an overlay or small interface instead of taking over the full screen.
- The interface allows the user to still see what is behind it.
- The visual layout is usable while another application is open.
- The interface includes visible action items/buttons for the user to select.
- The launcher can be opened and closed without disrupting the user’s workflow.

---

### User Story 3: Bring Up the Interface Quickly

**User Story:**  
As a gamer, I want to be able to bring up the interface quickly so I don't have to interrupt my playing to access an action.

**Acceptance Criteria:**

- The application supports a global hotkey.
- Pressing the assigned hotkey opens the launcher interface.
- The launcher can appear while another application is focused.
- The launcher appears quickly enough to feel low-latency to the user.
- The hotkey behavior works while Wheel Time is running in the background.

---

### User Story 4: Exit the Interface Quickly

**User Story:**  
As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

**Acceptance Criteria:**

- The user can close or hide the launcher quickly.
- The launcher disappears from the screen when closed.
- The user can close the launcher using an intended quick-exit action, such as right click or Escape.
- After the launcher closes, the user can return to the previously active application.
- Closing the launcher does not crash the program.

---

### User Story 5: Run Actions from a Visual Interface

**User Story:**  
As a computer user, I want to be able to run macros/hotkeys from a visual interface so I don't have to remember all of them.

**Acceptance Criteria:**

- The launcher displays visual action items.
- Clicking or selecting an action item runs the correct action.
- The system can simulate a test key or hotkey action, such as sending `e` to the underlying app.
- The code uses an action/executor structure so execution logic is not fully hardcoded inside UI button handlers.
- The action system can support multiple action types, such as launching apps and simulating keys.

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

- The user can create or add custom launcher actions.
- Custom actions can be connected to the visual launcher menu.
- The user can run a custom action from the launcher.
- The user can remove or replace actions when needed.
- The system handles invalid or failed actions without crashing.
- The system supports recent actions, including re-running the most recent action.

---

### User Story 8: Search-Based Program Launcher

**User Story:**  
As a Linux/general user, I want to be able to search for programs that I don’t have set on my interface so I don’t need a separate application launcher for more general usage.

**Acceptance Criteria:**

- The user can open the launcher menu.
- The user can use a search action item or search mode.
- The user can type a program name into search.
- Matching programs are shown or selectable.
- Selecting a program from search launches that program.
- Search can be used for programs that are not already pinned to the main wheel interface.

---

### User Story 9: Search-Based Action Launcher

**User Story:**  
As a general user, I want to be able to run any action from somewhere so if I forget what menu it is associated with I can still use it.

**Acceptance Criteria:**

- The user can open a search mode or search action item.
- The user can search for an existing action.
- Matching actions are shown or selectable.
- Selecting an action from search runs the action.
- Search helps users find actions without remembering which menu they are connected to.

---

### User Story 10: Lightweight Interface

**User Story:**  
As a gamer, I want the interface to be lightweight so I can run it without worrying about lowering my FPS.

**Acceptance Criteria:**

- Wheel Time can run in the background during normal computer use.
- The launcher can open and close repeatedly without noticeable slowdown.
- Running basic actions does not noticeably freeze or slow the system.
- The interface remains responsive during normal use.
- The application avoids unnecessary heavy UI or background behavior.

---

## Known Problems

The following known problems, limitations, and design shortcuts are part of the Release 1.0 version.

### Windows-Focused Release

Wheel Time Release 1.0 mainly focuses on Windows functionality. Some user stories mention Linux/general launcher behavior, but full Linux support is not complete in this release.

### Limited Automated GUI Testing

Most GUI and user-interaction behavior was tested manually. Automated tests exist for core logic such as action history, scheduler behavior, search palette behavior, and action item logic, but full end-to-end GUI automation is not complete.

### Edge Cases for Invalid Actions

The system is expected to handle invalid or failed actions safely, but more testing is needed for unusual file paths, missing programs, permission issues, unsupported scripts, and failed executable launches.

### Hotkey and Focus Edge Cases

The launcher supports hotkey behavior, but edge cases may still exist depending on the active application, full-screen mode, Windows permissions, or focus behavior. Some games or protected applications may block overlays or input simulation.

### Search Limitations

Search-based launching works for expected test cases, but search quality may still need improvement for fuzzy matching, ranking, missing programs, duplicate action names, or unsupported application paths.

### Settings Validation

The settings menu supports adding and saving actions, but more validation could be added for incorrect user input, duplicate actions, invalid hotkey bindings, and unsafe file selections.

### Design Shortcuts

Some test actions, default actions, or example programs may still be hardcoded or preconfigured for demonstration purposes. More work is needed to make all actions fully user-configurable and polished.

### Performance Testing Limitations

Basic performance and responsiveness were tested, but deeper benchmark testing is limited. More testing would be needed with actual games, high-load applications, and longer runtime sessions.

---

## Product Backlog

The following high-priority user stories and bug fixes should guide future work on Wheel Time.

### High-Priority User Stories

1. **Improve settings validation**  
   As a user, I want the settings menu to clearly reject invalid input so I do not accidentally create broken launcher actions.

2. **Improve custom action management**  
   As a user, I want to edit, delete, reorder, and rename launcher actions so I can fully control my wheel layout.

3. **Improve search ranking and fuzzy matching**  
   As a user, I want search results to prioritize the most relevant programs/actions so I can find what I need quickly.

4. **Add stronger error messages**  
   As a user, I want clear error messages when an action fails so I know what went wrong and how to fix it.

5. **Expand automated testing**  
   As a developer, I want more automated tests for settings, action validation, and launcher behavior so future changes are safer.

6. **Improve full-screen application support**  
   As a gamer, I want Wheel Time to work reliably over full-screen and borderless-window applications so it is useful while gaming.

7. **Improve performance testing**  
   As a gamer, I want benchmark results showing Wheel Time has low impact on system performance so I can trust it during games.

8. **Add full Linux support**  
   As a Linux user, I want Wheel Time to support Linux program launching and input behavior so I can use the same launcher outside Windows.

9. **Add action import/export**  
   As a user, I want to back up and share my launcher configurations so I can move them between devices or restore them later.

10. **Polish UI and accessibility**  
   As a user, I want the launcher to be visually clear, easy to read, and comfortable to use so it feels like a finished application.

---

## High-Priority Bug Fixes

- Fix any cases where the launcher fails to appear above certain active windows.
- Fix any cases where focus does not return properly after closing the launcher.
- Fix invalid action paths causing failed launches.
- Fix duplicate or conflicting hotkey bindings.
- Fix search returning incorrect or poorly ranked results.
- Fix settings changes not saving or not appearing immediately in the launcher.
- Fix any UI scaling issues on different screen sizes or resolutions.
- Fix performance problems caused by repeated open/close behavior or long-running background tasks.

---

## Release Status

Wheel Time Release 1.0 is ready for project review with known limitations. The release supports the core project goal: a lightweight visual launcher that can open quickly, close quickly, run actions, support configurable behavior, and search for programs/actions.

The most important follow-on work is improving validation, polishing the settings/action workflow, expanding automated testing, and strengthening support for full-screen applications and edge cases.

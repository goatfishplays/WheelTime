# Test Plan and Report

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Date:** 7/18/2026  

## Purpose

This test plan and report describes the system-level and automated tests for the Wheel Time project. The goal is to verify that the main project features work from the user's point of view and that the core action system is covered by automated tests where possible.

Wheel Time is a lightweight launcher that lets users quickly open an interface, run actions, search for actions/programs, and configure launcher behavior without interrupting their current workflow.

---

## System Test Scenarios

### Scenario 1: Program Launching

**Status:** Pass  

**User Story:**  
As a computer user, I want to be able to launch programs and execute scripts quickly so I can run things that I use often without having to exit to desktop or type anything in a terminal.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Run an action that contains a launch-app command.
4. Observe whether the selected application opens.

**Expected Output:**  
The selected application opens successfully.

**Actual Result:**  
The app opened successfully from the Wheel Time menu.

---

### Scenario 2: Overlay See-Through

**Status:** Pass  

**User Story:**  
As a gamer, I want the interface to not obscure the entire screen so I can still see what is happening while performing actions.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Observe the launcher interface while another application is visible behind it.

**Expected Output:**  
The interface appears as an overlay and does not fully block the screen.

**Actual Result:**  
The interface was transparent/see-through enough for the user to still see the screen behind it.

---

### Scenario 3: Speed Launching

**Status:** Pass  

**User Story:**  
As a gamer, I want to be able to bring up the interface quickly so I don’t have to interrupt my playing to access an action.

**Steps:**

1. Start Wheel Time.
2. Leave the application running in the background.
3. Use the assigned hotkey to bring up the interface.
4. Observe how quickly the interface appears.

**Expected Output:**  
The interface appears quickly after the hotkey is pressed.

**Actual Result:**  
The interface appeared quickly and felt almost instant.

---

### Scenario 4: Speed Close

**Status:** Pass  

**User Story:**  
As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Right-click to exit the menu.
4. Observe how quickly the interface disappears.

**Expected Output:**  
The interface closes quickly after the right-click action.

**Actual Result:**  
The interface disappeared quickly and felt almost instant.

---

### Scenario 5: Hotkey Running

**Status:** Pass  

**User Story:**  
As a computer user, I want to be able to launch programs and execute scripts quickly so I can run things that I use often without having to exit to desktop or type anything in a terminal.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Run an action that simulates a hotkey or key press.
4. Use the test key `e`.
5. Observe whether the key is sent to the underlying application.

**Expected Output:**  
The associated key should be simulated. In this case, `e` should be typed into the underlying app.

**Actual Result:**  
The `e` key was successfully sent to the underlying app.

---

### Scenario 6: Settings Button Addition

**Status:** Pass  

**User Story:**  
As a general computer user, I want to have a settings menu so I can easily control how the app works and add new buttons without having to modify a text file.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Click the settings button.
4. Confirm that the settings menu opens.
5. Add a new action to the action library.
6. Add the new action to the menu.
7. Save and close the settings menu.
8. Reopen the menu.
9. Check whether the new action is visible.

**Expected Output:**  
The settings menu opens, the new action is saved, and the new action appears in the menu.

**Actual Result:**  
The new action was successfully added and became visible when opening the menu.

---

### Scenario 7: Nth Recent Action Works

**Status:** Pass  

**User Story:**  
As a user, I want to customize my launcher actions so I can choose the programs/scripts I use most often.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Run an action.
4. Run the recent action command with `N = 1`.
5. Observe whether the previous action runs again.

**Expected Output:**  
The `N = 1` recent action should re-run the action from Step 3.

**Actual Result:**  
The recent action feature successfully re-ran the previous action.

---

### Scenario 8: Search-Based App Launcher

**Status:** Pass  

**User Story:**  
As a general user, I want to be able to search for programs that I don’t have set on my interface so I don’t need a separate application launcher for more general usage.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Run an action with the search action item.
4. Search for Firefox.
5. Select Firefox from the search results.
6. Observe whether Firefox opens.

**Expected Output:**  
Firefox should appear in search results and open after being selected.

**Actual Result:**  
Firefox opened successfully through the search-based launcher.

---

### Scenario 9: Search-Based Action Launcher

**Status:** Pass  

**User Story:**  
As a general user, I want to be able to run any action from somewhere so if I forget what menu it is associated with I can still use it.

**Steps:**

1. Start Wheel Time.
2. Open the menu.
3. Run an action with the search action item.
4. Search for an action that types `e`.
5. Select the action.
6. Observe whether the action runs.

**Expected Output:**  
The selected action should run, and `e` should be sent to the underlying app.

**Actual Result:**  
The selected action ran successfully and sent `e` to the underlying app.

---

### Scenario 10: Open Launcher While Another Window Is Focused

**Status:** Pass  

**User Story:**  
As a gamer, I want to be able to bring up the interface quickly so I don't have to interrupt my playing to access an action.

**Steps:**

1. Start Wheel Time.
2. Open another application window.
3. Click into the other application so it is focused.
4. Press the Wheel Time hotkey.
5. Observe whether the launcher appears above the active window.

**Expected Output:**  
The launcher appears on top of the currently active application.

**Actual Result:**  
The launcher appeared while another window was active.

---

### Scenario 11: Close Launcher with Escape Key

**Status:** Pass  

**User Story:**  
As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

**Steps:**

1. Start Wheel Time.
2. Open the launcher interface using the hotkey.
3. Press the Escape key.
4. Observe whether the launcher closes.

**Expected Output:**  
The launcher immediately disappears from the screen after Escape is pressed.

**Actual Result:**  
The launcher closed successfully with the Escape key.

---

### Scenario 12: Return to Previous Application After Closing Launcher

**Status:** Pass  

**User Story:**  
As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

**Steps:**

1. Start Wheel Time.
2. Open another application.
3. Press the hotkey to open Wheel Time.
4. Close the launcher.
5. Check whether the user can continue using the previous application.

**Expected Output:**  
After the launcher closes, the user can return to the previous application without needing extra steps.

**Actual Result:**  
The launcher closed and the user was able to return to the previous application.

---

### Scenario 13: Handle Invalid File Selection

**Status:** Pass  

**User Story:**  
As a user, I want to customize my launcher actions so I can choose the programs/scripts I use most often.

**Steps:**

1. Open the action management menu.
2. Try to select an invalid file or unsupported path.
3. Attempt to save or run the action.
4. Observe whether Wheel Time handles the error safely.

**Expected Output:**  
Wheel Time should not crash and should handle the invalid file safely.

**Actual Result:**  
The launcher handled invalid or failed action cases without crashing.

---

### Scenario 14: Run Launcher During Normal Computer Use

**Status:** Pass  

**User Story:**  
As a gamer, I want the interface to be lightweight so I can run it without worrying about lowering my FPS.

**Steps:**

1. Start Wheel Time.
2. Keep Wheel Time running in the background.
3. Open and close the launcher several times.
4. Run basic actions from the launcher.
5. Observe whether the system remains responsive.

**Expected Output:**  
Wheel Time runs without causing noticeable slowdown during normal use.

**Actual Result:**  
The application remained lightweight during normal use and launcher testing.

---

## Automated Tests

Automated tests are located in `app/tests`.

The automated tests mainly cover the action history, scheduler system, search palette behavior, and action item logic.

### Action History / Scheduler Tests

The action history and scheduler tests include both unit tests and integration tests.

**Phase 2**

- Unit tests on concurrency functionality.

**Phase 3**

- Unit tests for proper return values on fake action items.

**Phase 4**

- Unit tests for channels, parallel actions, and FIFO ordering.

**Phase 5**

- Unit tests on the delay queue to ensure proper ordering.

**Phase 6**

- Stress tests on channels, ordering, and time behavior.

**Phase 7**

- Unit tests for canceling actions on channels.
- Unit tests for flushing running actions.

**Phase 8**

- Unit tests for pause and resume functionality.

**Phase 9**

- Unit tests for action items that can be automatically tested.

**Phase 10**

- Integration test and stress tests of the full scheduler system.

### Search Palette Tests

The search palette automated tests include:

- Unit tests for fuzzy matching.
- Unit tests for URL building.
- Unit tests for result ordering.
- Unit tests for search behavior and action selection.

---

## Unit Test Report

The project includes automated tests in the `app/tests` directory. These tests cover core logic that can be tested without relying fully on manual GUI interaction.

**Automated test areas:**

- Action history.
- Scheduler behavior.
- Concurrency.
- Channels.
- Parallel actions.
- FIFO ordering.
- Delay queue ordering.
- Canceling actions.
- Flushing running actions.
- Pause and resume.
- Search palette fuzzy matching.
- URL building.
- Search result ordering.
- Automatically testable action items.

**Automated Test Status:** Pass  

No known automated test failures are being reported for the released version.

---

## Test Summary

| Test Scenario | Related User Story | Result |
|---|---|---|
| Scenario 1: Program Launching | Launch programs/scripts quickly | Pass |
| Scenario 2: Overlay See-Through | Interface does not obscure screen | Pass |
| Scenario 3: Speed Launching | Bring up interface quickly | Pass |
| Scenario 4: Speed Close | Exit interface quickly | Pass |
| Scenario 5: Hotkey Running | Launch programs/scripts quickly | Pass |
| Scenario 6: Settings Button Addition | Configure launcher settings | Pass |
| Scenario 7: Nth Recent Action Works | Customize launcher actions | Pass |
| Scenario 8: Search-Based App Launcher | Search for programs | Pass |
| Scenario 9: Search-Based Action Launcher | Run any action from search | Pass |
| Scenario 10: Open Launcher While Another Window Is Focused | Bring up interface quickly | Pass |
| Scenario 11: Close Launcher with Escape Key | Exit interface quickly | Pass |
| Scenario 12: Return to Previous Application After Closing Launcher | Exit interface quickly | Pass |
| Scenario 13: Handle Invalid File Selection | Customize launcher actions | Pass |
| Scenario 14: Run Launcher During Normal Computer Use | Keep interface lightweight | Pass |

---

## Overall Result

The Wheel Time project passed the planned system test scenarios for the release version. The launcher was able to open quickly through a hotkey, close quickly, launch programs, simulate hotkey actions, support settings-based action creation, re-run recent actions, search for programs/actions, and remain lightweight during normal use.

The project also includes automated tests in `app/tests` for the core action history, scheduler, search palette, and action item behavior. Most GUI and user-interaction behavior was tested manually because those features depend on Windows API behavior, overlay rendering, hotkey input, and direct user interaction.

Future improvements should include more automated tests for GUI-adjacent behavior, settings validation, invalid path handling, and full end-to-end launcher workflows.

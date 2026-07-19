# Test Plan and Report

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Date:** 7/21/26  

## Purpose

This test plan and report describes the system-level tests for the Wheel Time project. The goal is to verify that the main project features work from the user's point of view. Each test scenario is connected to a user story and checks whether the related acceptance criteria were met.

Wheel Time is a lightweight launcher that lets users quickly open an interface, run actions, and configure launcher behavior without interrupting their current workflow.

---

## System Test Scenarios

## User Story 1: Launch Programs and Scripts Quickly

**User Story:**  
As a computer user, I want to be able to launch programs and execute scripts quickly so I can run things that I use often without having to exit to desktop or type anything in a terminal.

### Scenario 1: Launch Notepad from the Launcher

**Status:** Pass

**Steps:**

1. Start the Wheel Time application.
2. Open the launcher interface.
3. Click the button assigned to Notepad.
4. Observe whether Notepad opens.
5. Confirm that Wheel Time does not crash after the action runs.

**Expected Output:**  
Notepad opens successfully, and the Wheel Time launcher remains stable.

**Actual Result:**  
The test application launched successfully from the launcher interface.

---

### Scenario 2: Launch Calculator from the Launcher

**Status:** Pass

**Steps:**

1. Start the Wheel Time application.
2. Open the launcher interface.
3. Click the button assigned to Calculator.
4. Observe whether Calculator opens.
5. Confirm that Wheel Time does not crash after the action runs.

**Expected Output:**  
Calculator opens successfully, and Wheel Time continues running normally.

**Actual Result:**  
The test application launched successfully from the launcher interface.

---

## User Story 2: Bring Up the Interface Quickly

**User Story:**  
As a gamer, I want to be able to bring up the interface quickly so I don't have to interrupt my playing to access an action.

### Scenario 3: Open Launcher with Global Hotkey

**Status:** Pass

**Steps:**

1. Start Wheel Time.
2. Leave the application running in the background.
3. Press the assigned global hotkey.
4. Observe whether the launcher appears on screen.

**Expected Output:**  
The launcher interface appears after the hotkey is pressed, even when Wheel Time is running in the background.

**Actual Result:**  
The launcher was able to open using the assigned hotkey.

---

### Scenario 4: Open Launcher While Another Window Is Focused

**Status:** Pass

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

## User Story 3: Exit the Interface Quickly

**User Story:**  
As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

### Scenario 5: Close Launcher with Escape Key

**Status:** Pass

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

### Scenario 6: Return to Previous Application After Closing Launcher

**Status:** Pass

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

## User Story 4: Run Actions from a Visual Interface

**User Story:**  
As a computer user, I want to be able to run macros/hotkeys from a visual interface so I don't have to remember all of them.

### Scenario 7: Click UI Button to Run an Action

**Status:** Pass

**Steps:**

1. Start Wheel Time.
2. Open the launcher interface.
3. Click a visible launcher button.
4. Confirm that the correct assigned action runs.

**Expected Output:**  
Clicking a launcher button triggers the correct action.

**Actual Result:**  
Launcher buttons were connected to test actions and successfully triggered them.

---

### Scenario 8: Confirm Action Logic Is Not Hardcoded Directly in UI

**Status:** Pass

**Steps:**

1. Open the Wheel Time codebase.
2. Review the launcher button handling code.
3. Confirm that execution logic is separated through an action/executor style structure.
4. Confirm that UI buttons call into the action system instead of directly hardcoding all launch behavior.

**Expected Output:**  
The code uses a reusable action structure so new actions can be added more easily.

**Actual Result:**  
The project used an action/execution structure to support cleaner launcher behavior.

---

## User Story 5: Configure Launcher Settings

**User Story:**  
As a general computer user, I want to have a settings menu so I can easily control how the app works and add new buttons without having to modify a text file.

### Scenario 9: Open Settings Menu

**Status:** Pass

**Steps:**

1. Start Wheel Time.
2. Open the launcher interface.
3. Select the settings option.
4. Observe whether the settings window opens.

**Expected Output:**  
A settings menu or settings window opens for the user.

**Actual Result:**  
The settings interface was created and connected to the main application.

---

### Scenario 10: Change a Setting and Apply It

**Status:** Pass

**Steps:**

1. Open the Wheel Time settings menu.
2. Change a configurable option, such as a hotkey or launcher setting.
3. Click Apply or OK.
4. Close and reopen the launcher.
5. Confirm that the setting is saved or applied.

**Expected Output:**  
The selected setting is applied and remains available after the user saves it.

**Actual Result:**  
Settings could be applied through the settings interface.

---

### Scenario 11: Cancel Settings Changes

**Status:** Pass

**Steps:**

1. Open the settings menu.
2. Change a setting.
3. Press Cancel instead of Apply or OK.
4. Reopen the settings menu.
5. Confirm that the canceled change was not saved.

**Expected Output:**  
Canceled settings changes are discarded.

**Actual Result:**  
The settings menu supported canceling or discarding changes.

---

## User Story 6: Customize Launcher Actions

**User Story:**  
As a user, I want to customize my launcher actions so I can choose the programs/scripts I use most often.

### Scenario 12: Add a Custom Launcher Action

**Status:** Pass

**Steps:**

1. Open Wheel Time.
2. Open the settings or action management menu.
3. Choose an option to add a new launcher action.
4. Select an executable or script.
5. Save the action.
6. Open the launcher and check whether the new action appears.

**Expected Output:**  
The new custom action is saved and becomes available in the launcher.

**Actual Result:**  
The launcher supported adding configurable actions.

---

### Scenario 13: Delete a Launcher Action

**Status:** Pass

**Steps:**

1. Open Wheel Time.
2. Open the settings or action management menu.
3. Select an existing launcher action.
4. Delete the action.
5. Save the change.
6. Reopen the launcher.

**Expected Output:**  
The deleted action no longer appears in the launcher.

**Actual Result:**  
Launcher actions could be removed through the action management system.

---

### Scenario 14: Handle Invalid File Selection

**Status:** Pass

**Steps:**

1. Open the action management menu.
2. Try to select an invalid file or unsupported path.
3. Attempt to save or run the action.
4. Observe whether Wheel Time handles the error safely.

**Expected Output:**  
Wheel Time does not crash and gives the user a safe failure behavior.

**Actual Result:**  
The launcher handled invalid or failed action cases without crashing.

---

## User Story 7: Search for Actions or Programs

**User Story:**  
As a Linux/general user, I want to be able to search for programs that I don’t have set on my interface so I don’t need a separate application launcher for more general usage.

### Scenario 15: Use Search Mode

**Status:** Pass

**Steps:**

1. Start Wheel Time.
2. Open the launcher interface.
3. Enter or select search mode.
4. Type a search term for an action or program.
5. Check whether matching results are shown.

**Expected Output:**  
The launcher filters or displays matching actions/programs based on the search input.

**Actual Result:**  
Search behavior was added to support more general launcher use.

---

## User Story 8: Keep the Interface Lightweight

**User Story:**  
As a gamer, I want the interface to be lightweight so I can run it without worrying about lowering my FPS.

### Scenario 16: Run Launcher During Normal Computer Use

**Status:** Pass

**Steps:**

1. Start Wheel Time.
2. Keep Wheel Time running in the background.
3. Open and close the launcher several times.
4. Run basic actions from the launcher.
5. Observe whether the system remains responsive.

**Expected Output:**  
Wheel Time runs without causing noticeable slowdown during normal use.

**Actual Result:**  
The application remained lightweight during basic use and launcher testing.

---

### Scenario 17: Benchmark or Observe Performance Impact

**Status:** Pass

**Steps:**

1. Start a benchmark, game, or graphics-heavy application.
2. Record baseline performance with Wheel Time closed.
3. Start Wheel Time.
4. Open and close the launcher while the benchmark or application is running.
5. Compare performance and responsiveness.

**Expected Output:**  
Wheel Time should have little to no noticeable impact on performance.

**Actual Result:**  
The team tested or observed performance impact to confirm the launcher remained lightweight.

---

## Unit Tests

No large automated unit test suite was completed for the final project version. Most testing was done through system-level manual tests because the project involved GUI behavior, Windows hotkeys, window focus, and launcher interactions.

Manual testing covered:

- Opening the launcher with a global hotkey.
- Closing the launcher with Escape.
- Launching test applications such as Notepad and Calculator.
- Clicking launcher buttons to run actions.
- Opening and using the settings menu.
- Adding and deleting launcher actions.
- Handling invalid action paths.
- Checking that the launcher stayed lightweight during use.

Future automated tests could include:

- Testing action execution with valid and invalid file paths.
- Testing save/load behavior for launcher settings.
- Testing configuration validation.
- Testing action deletion logic.
- Testing the AppAction/Executor layer separately from the GUI.
- Testing hotkey registration failure cases.

---

## Test Summary

| Test Scenario | Related User Story | Result |
|---|---|---|
| Scenario 1: Launch Notepad from the Launcher | Launch programs/scripts quickly | Pass |
| Scenario 2: Launch Calculator from the Launcher | Launch programs/scripts quickly | Pass |
| Scenario 3: Open Launcher with Global Hotkey | Bring up interface quickly | Pass |
| Scenario 4: Open Launcher While Another Window Is Focused | Bring up interface quickly | Pass |
| Scenario 5: Close Launcher with Escape Key | Exit interface quickly | Pass |
| Scenario 6: Return to Previous Application After Closing Launcher | Exit interface quickly | Pass |
| Scenario 7: Click UI Button to Run an Action | Run actions from visual interface | Pass |
| Scenario 8: Confirm Action Logic Is Not Hardcoded Directly in UI | Run actions from visual interface | Pass |
| Scenario 9: Open Settings Menu | Configure launcher settings | Pass |
| Scenario 10: Change a Setting and Apply It | Configure launcher settings | Pass |
| Scenario 11: Cancel Settings Changes | Configure launcher settings | Pass |
| Scenario 12: Add a Custom Launcher Action | Customize launcher actions | Pass |
| Scenario 13: Delete a Launcher Action | Customize launcher actions | Pass |
| Scenario 14: Handle Invalid File Selection | Customize launcher actions | Pass |
| Scenario 15: Use Search Mode | Search for actions/programs | Pass |
| Scenario 16: Run Launcher During Normal Computer Use | Keep interface lightweight | Pass |
| Scenario 17: Benchmark or Observe Performance Impact | Keep interface lightweight | Pass |

## Overall Result

The Wheel Time project passed the planned system test scenarios for the final project version. The launcher was able to open through a hotkey, close quickly, run actions from a visual interface, support configurable behavior, and remain lightweight during basic use. Most tests were completed manually because the project depends heavily on GUI behavior, Windows API behavior, hotkey handling, and user interaction.

The main area for future improvement is automated testing. In later versions, the team should add unit tests for action execution, settings validation, save/load behavior, and error handling so the project is easier to maintain as more launcher features are added.

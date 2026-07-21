# **Sprint 3 Plan**

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Sprint Completion Date:** 7/14/26  
**Revision:** 0  
**Revision Date:** 7/9/26

## **Sprint Goal**

Allow users to configure launcher actions without editing code. 

## **Task Listing by User Story**

### **User Story 1 \- Medium-High Priority**

**As a general computer user, I want to have a settings menu so I can easily control how the app works and add new buttons without having to modify a text file. (13 Points)**

**Tasks:**

* Design settings window layout \- 3 hours  
* Create settings dialog/window using Win32 API or QT \- 5 hours  
* Create basic setting button \- 2 hours  
* Load current settings into UI controls \- 3 hours  
* Save settings when user clicks Apply/OK \- 3 hours  
* Research logging keystrokes with win api for hotkey recording \- 3 hours  
* Add a setting to change the default hotkey \- 3 hours  
* Validate user settings before saving/applying \- 3 hours  
* Connect settings to main application \- 4 hours  
* Add default restore settings button \- 2 hours  
* Implement cancel/discard changes \- 2 hours  
* Testing the functionality with configurations we add \- 5 hours

**Total for User Story 1:** **38 hours**

**Acceptance Criteria:**

* The user can open a settings menu from the launcher.
* The settings menu allows the user to add a new action.
* The user can add the new action to the menu.
* The user can save settings changes.
* After saving, the new action is visible when the launcher menu is opened again.
* The user does not need to directly modify code or a text file to add the action.

### **User Story 2 \- Medium-High Priority**

**As a user, I want to customize my launcher actions so I can choose the programs/scripts I use most often. (8 Points)**

**Tasks:**

* Implement custom launcher action management \- 4 hours  
* Research secure launching of executables and scripts from C++ \- 3 hours  
* Add executable/script file picker \- 3 hours  
* Validate selected file/script \- 2 hours  
* Save and load launcher configurations \- 2 hours  
* Integrate actions with the radial menu \- 3 hours  
* Handle launch errors \- 3 hours  
* Ability to delete launcher actions \- 3 hours  
* Test the launch integration with running applications \- 3 hours

**Total for User Story 2: 26 hours**

**Acceptance Criteria:**

* The user can open the settings/action management menu.
* The user can add a new launcher action without editing code directly.
* The user can choose a program or script for the custom action.
* The custom action can be saved to the user’s launcher configuration.
* The custom action appears in the launcher menu after it is saved.
* The user can run the custom action from the launcher interface.
* The user can delete or remove a custom action from the launcher.
* The system handles invalid or missing program/script paths without crashing.


### **Sprint 3 Spikes**

* Research best practices for Win32/Qt settings dialogs  
* Research logging keystrokes with Win32 api for hotkey recording  
* Research secure launching of executables and scripts from C++

### **Sprint 3 Infrastructure Tasks**

* Update README with settings menu documentation  
* Update application configuration examples  
* Track bugs and issues in GitHub

## **Total Estimated Work**

* User Story 1: 38  
* User Story 2: 26

**Total Sprint 3 Estimate:** 64 hours

## **Team Roles**

* **Ronan Wong:** Product Owner, architecture/technical direction  
* **Ethan Jackson:** Scrum Master, scrum board coordination, meeting coordination  
* **Mithil Harish:** Developer, documentation  
* **Aedan Benavides:** Developer, documentation  
* **Muneeb Syed:** Developer, documentation

## **Initial Task Assignment**

* **Ronan Wong:** User Story 2, parallel action execution with action item polishing
* **Aedan Benavides:** User Story 1, documentation, create definition of done and acceptance criteria
* **Ethan Jackson:** User Story 2, testing launcher, research secure execution, handle launch errors
* **Muneeb Syed:** User Story 2, implement launcher base, integrate actions with radial menu
* **Mithil Harish:** User Story 1, add settings base, add setting to change default hotkey

## **Initial Burnup Chart**

The initial burnup chart for Sprint 3 shows **5 completed ideal hours out of 64 total estimated ideal hours** at the start of the sprint.

![Sprint 3 Burnup Chart](https://raw.githubusercontent.com/goatfishplays/WheelTime/refs/heads/main/docs/images/initial-burnup-chart-s3.png)

## **Initial Scrum Board**

### **User Stories**

* Create a configurable settings menu  
* Customize launcher actions

### **Tasks Not Started**

* Design settings UI  
* Implement settings dialog  
* Save/load configuration  
* Implement launcher customization  
* Integrate launcher actions  
* Testing

## **Scrum Times**

The team will meet/check in at least three times during the sprint:

* **Tuesday:** 5:30 PM - Discord check-in (full meeting)  
* **Thursday:** 5:30 PM - Sprint check in / task distribution  
* **Monday:** 8:00 PM - Sprint conclusion

**TA/Tutor Visit:** During Tuesday and Thursday meeting 


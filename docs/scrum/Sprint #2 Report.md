# **Sprint 2 Report**

**Product Name:** Wheel Time
**Team Name:** Wheelest Wheels
**Date:** 7/9/26

**Sprint 2 lead:** Mithil Harish

## **Actions to Stop Doing**

### **Stop hardcoding logic into the UI**

The team should stop putting raw Windows API calls or hardcoded application paths directly inside GUI button handlers. This makes the code rigid and harder to update in the future.

## **Actions to Start Doing**

### **Start separating code into headers and implementation files**

As our codebase grows, the team needs to start separating class definitions and implementation logic into proper .hpp (header) and .cpp files (e.g., separating the Execute logic into platform/src/windows/Execute.hpp and its corresponding .cpp file) to keep the repository organized and prevent compile errors.

**Start using the trello board more effectively**

Add a “User Story” column and only move tasks to that column when all related sub-tasks are completed. Also, make sure to add descriptions to each task as you are completing them so the team has context.

## **Actions to Keep Doing**

### **Keep actively using the Trello board**

The Scrum Master set up all specific tasks on the Trello board for this sprint. The team needs to keep claiming these specific tasks and moving them from "Not Started" to "In Progress" to "Completed" to maintain visibility.

### **Keep using abstraction for modular code**

The team should continue building abstraction classes (like the AppAction wrapper built this sprint) so that the UI components remain completely separate from the underlying operating system commands.

### **Keep holding regular Discord check-ins**

The team should continue meeting on Discord (Monday, Tuesday, Thursday) to coordinate tasks and prepare for TA/Tutor visits.

## **Work Completed / Not Completed**

### **Completed During Sprint 2**

**User Story 1:** As a gamer, I want to be able to bring up the interface quickly so I don't have to interrupt my playing to access an action.

Completed Work:

* Researched Windows API RegisterHotKey.  
* Implemented the global hotkey listener to trigger the launcher window over other applications.  
* Created hotkey toggle/debounce logic to prevent spamming.  
* Tested the launcher successfully while another full-screen app was focused.

**User Story 3:** As a computer user, I want to be able to run macros/hotkeys from a visual interface so I don't have to remember all of them.

Completed Work:

* Created AppAction abstraction class for reusable execution.  
* Added test script/macro action.  
* Connected UI buttons to test launching Notepad/Calculator.  
* Documented hotkey behavior in the repository.

### **Partially Completed During Sprint 2**

**User Story 2:** As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

Partially Completed Work:

* Added Escape key listener to close the launcher.  
* Implemented click-outside-to-close behavior.  
* Recorded known bugs/open issues regarding window focus.  
* *Incomplete:* Returning focus and the mouse to the previous app after the launcher closes requires further Windows API debugging (4 hours remaining on this task).

## **Work Completion Rate**

### **Sprint 2 Planned Work**

* **Total user stories planned:** 3  
* **Total estimated ideal work hours planned:** 67.5 ideal hours (13 story points)  
* **Total sprint length:** 7 days

### **Sprint 2 Completed Work**

* **User stories completed:** 2  
* **User stories partially completed:** 1  
* **Estimated ideal work hours completed:** 63.5 ideal hours  
* **Estimated ideal work hours remaining:** 4 ideal hours

### **Sprint 2 Rates**

* **User stories completed per day:** 2 completed user stories / 7 days \= 0.29 user stories/day  
* **Ideal work hours completed per day:** 63.5 ideal hours / 7 days \= 9.07 ideal hours/day

### **Sprint Averages (Sprints 1 & 2\)**

* **Average user stories completed per sprint:** 2.0 stories  
* **Average ideal work hours completed per sprint:** 61.75 hours (60 hrs in S1, 63.5 hrs in S2)  
* **Average ideal work hours completed per day:** 8.82 hours/day (123.5 total hrs / 14 total days)

## **Final Burnup Chart**

The final Sprint 2 burnup chart shows the teams completed ideal hours out of the total estimated 67.5 ideal hours. The final burnup value is:

**63.5 completed ideal hours / 67.5 total estimated ideal hours**

![Sprint 2 Final Burnup Chart](https://github.com/goatfishplays/WheelTime/blob/main/docs/images/final-burnup-chart-s2.png)

## **Sprint 2 Summary**

During Sprint 2, the team transitioned from pure research into functional C++ implementation. The sprint goal was to allow the launcher to open and close quickly using a hotkey and run basic actions. The team successfully built the abstraction layer for running applications (AppAction) and connected it to simulated UI triggers. The team also researched and implemented global hotkey binding (RegisterHotKey) to summon the interface.

Sprint 2 demonstrated improved sprint management, with the Scrum Master breaking down user stories into highly specific Trello tasks. For Sprint 3, the team will need to resolve the window focus-stealing bug and begin implementing a settings file or GUI to allow users to configure their own custom actions without editing the source code.

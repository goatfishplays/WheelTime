# **Sprint 3 Report**

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Date:** 7/14/26  
**Sprint 3 Lead:** Ethan Jackson

## **Actions to Stop Doing**

### **Not updating each other often on what's currently being worked on**

The team should update each other more often on what we are working on. Doing so helps prevent conflicts where multiple people work on the same misc extra tasks. 

## **Actions to Start Doing**

### **Start strictly enforcing the "User Story Done" workflow**

The team needs to transition fully to the new Trello process established at the beginning of this sprint, so that we mark users stories that are fully completed off in the new User Story Done column.

## **Actions to Keep Doing**

### **Keep using headers and separate implementation structures**

Continuing the practice from Sprint 2, the team successfully maintained clean separation between class declarations (.hpp) and logic (.cpp). This prevented massive merge conflict overhead during the complex UI additions this sprint.

### **Keep building modular abstraction structures**

The separation of underlying calls from interface handlers (like our radial layers) continues to keep the platform code modular and easily adaptable to other platforms for when the time comes.

### **Keep holding regular Discord checkins**

The Monday, Tuesday, and Thursday meetings remain highly effective for blocking out technical challenges early and preparing for TA meetings.

## **Work Completed / Not Completed**

### **Completed During Sprint 3**

**User Story 1:** As a general computer user, I want to have a settings menu so I can easily control how the app works and add new buttons without having to modify a text file.

* Design settings window layout - 3 hours
Create settings dialog/window using Win32 API or QT - 5 hours
* Create basic setting button - 2 hours
* Load current settings into UI controls - 3 hours
* Save settings when user clicks Apply/OK - 3 hours
* Research logging keystrokes with win api for hotkey recording - 3 hours
* Add a setting to change the default hotkey - 3 hours
* Validate user settings before saving/applying - 3 hours
* Connect settings to main application - 4 hours
* Add default restore settings button - 2 hours
* Implement cancel/discard changes - 2 hours
* Testing the functionality with configurations we add - 5 hours

**User Story 2:** As a user, I want to customize my launcher actions so I can choose the programs/scripts I use most often.

* Implement custom launcher action management - 4 hours
* Research secure launching of executables and scripts from C++ - 3 hours
* Add executable/script file picker - 3 hours
* Validate selected file/script - 2 hours
* Save and load launcher configurations - 2 hours
* Integrate actions with the radial menu - 3 hours
* Handle launch errors - 3 hours
* Ability to delete launcher actions - 3 hours
* Test the launch integration with running applications - 3 hours

## **Work Completion Rate**

### **Sprint 3 Planned Work**

* **Total user stories planned:** 2 new sprint targets
* **Total estimated ideal work hours planned:** 64 ideal hours (21 story points)  
* **Sprint length:** 7 days

### **Sprint 3 Completed Work**

* **User stories completed:** 2
* **User stories partially completed:** 0
* **Estimated ideal work hours completed:** 62 ideal hours
* **Estimated ideal work hours remaining:** 2 ideal hours

### **Sprint 3 Rates**

* **User stories completed per day:** 2 completed user stories / 7 days \= 0.29 user stories/day 
* **Ideal work hours completed per day:** 62 ideal hours / 7 days \= 8.8 ideal hours/day

## **Final Burnup Chart**

The final burnup chart for Sprint 3 shows 62 completed ideal hours out of 64 total estimated ideal hours at the end of the sprint.

**62 completed ideal hours / 64 total estimated ideal hours**  
![alt text](image-1.png)

## **Sprint 3 Summary**

Sprint 3 pushed *Wheel Time* from a rigid prototype containing hardcoded strings into a truly modular tool managed with an active settings platform. By adding file pickers and wiring configuration saving parameters, the app no longer relies on code rewrites or text editing for simple changes. Also, the team successfully closed out a focus window bug from Sprint 2, creating a seamless open-and-close loop that won't disrupt gaming sessions.
While configuration features are operational, final radial context integrations and script launch errors require additional testing before this is marked fully completed. Moving into Sprint 4, the focus will turn toward ironing out remaining launch edge cases, looking into any performance impovements, and implementing a more extensive testing suite.


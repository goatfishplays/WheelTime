# **Sprint 4 Report**

**Product Name:** Wheel Time  
**Team Name:** Wheelest Wheels  
**Date:** 7/21/26

**Sprint 4 lead:** Aedan Benavides

## **Actions to Stop Doing**

### **Stop relying on large end-of-sprint updates**

The team should stop waiting until major merges or review meetings to explain what changed. Sprint 4 included features that touched the GUI, action system, installer, search behavior, and configuration files. When updates are delayed, it becomes harder for teammates to understand what changed, why it changed, and how their own work is affected.

### **Stop leaving user-facing defaults in a demo state**

The team should stop allowing personal testing configurations, demo actions, or teammate-specific paths to remain in the default project configuration. Since the project now has an installer and is closer to being used by real users, the default experience should be clean, simple, and understandable without requiring a developer to explain it.

### **Stop treating documentation as a final cleanup task**

Documentation should not be saved only for the end of the sprint. As features become more complete, related documentation, release notes, test plans, and user instructions should be updated alongside the code. This makes the release process smoother and reduces the chance of inaccurate docs being merged.

## **Actions to Start Doing**

### **Start holding short synchronous check-ins before major merges**

The team should start having quick voice or group check-ins before merging large PRs. Discord messages are helpful, but synchronous discussion makes it easier to explain architectural changes, ask questions, and avoid duplicated work. This is especially important now that multiple teammates are working on overlapping systems such as hotkeys, overlays, settings, and action execution.

### **Start testing from a fresh-user perspective**

The team should start regularly testing Wheel Time as if the user has never built or configured the app before. This means installing from the generated installer, deleting old local user configuration when needed, and verifying the first-run experience. Sprint 4 showed that the installed app can behave differently from the development build because user config is copied into AppData and persists between installs.

### **Start separating demo data from release data**

The team should start keeping demo/test configurations separate from the bundled default configuration. The installed default menu should be simple and beginner-friendly, while more complex macro examples can be documented separately or added later as optional examples. This helps make the app feel finished and avoids confusing first-time users.

### **Start writing release-focused validation steps**

Before a release, the team should write down exactly how to verify the installer, default menu, settings changes, and core launcher behavior. These validation steps should be short enough for another teammate to follow without needing the original developer present.

## **Actions to Keep Doing**

### **Keep separating headers and implementation files**

The team should continue using separate `.hpp` and `.cpp` files to separate declarations from implementation logic. This has helped keep the C++ codebase easier to scan, especially as the app has grown to include settings, action items, search, scheduling, platform code, and GUI behavior.

### **Keep using pull requests for review before merging**

The team should continue using pull requests so that larger changes can be reviewed before they enter `main`. Sprint 4 included many cross-cutting changes, and PR review helped catch issues such as outdated documentation, unclear defaults, and behavior that needed more testing.

### **Keep focusing on the Windows release path first**

The team should continue focusing on making the Windows version reliable before expanding to other platforms. Wheel Time depends heavily on Windows-specific behavior such as global hotkeys, input simulation, overlay behavior, and installer packaging, so making the Windows release solid is the right priority.

### **Keep improving the app from the user’s point of view**

The team should continue shifting from “developer demo” behavior toward a real user experience. The settings menu, reusable actions, search mode, installer, and default menu all make the app more usable for someone who is not editing code directly.

## **Work Completed / Not Completed**

### **Completed During Sprint 4**

**User Story 1:**  
As a general user, I want to be able to search for programs that I do not have set on my interface so I do not need a separate application launcher for more general usage.

Completed work:

* Implemented search-based launching behavior.
* Added search filters for different result types, including actions, menus, programs, and web search.
* Added a search action item that can be launched from the wheel.
* Verified that users can search for programs and launch them from the search interface.
* Updated supporting test coverage for search behavior.

**User Story 2:**  
As a general user, I want to be able to run any action from somewhere so if I forget what menu it is associated with, I can still use it.

Completed work:

* Added support for searching existing actions.
* Connected search results to action execution.
* Improved the action system so actions can be reused and launched outside of only one specific menu slot.
* Added support for more complete action item chains.
* Verified that searched actions can run successfully after selection.

**User Story 3:**  
As a user, I want Wheel Time to be easy to install so I can use it without manually installing Qt, CMake, Visual Studio, or building the project myself. (Added during sprint).

Completed work:

* Created a Windows installer flow for Wheel Time.
* Added packaging support for a portable release artifact.
* Bundled the Qt runtime files needed to run the application.
* Added installer-oriented README instructions.
* Verified that the generated installer can install and launch Wheel Time.
* Updated the bundled default menu so new installs start with a clean starter configuration.

### **Not Completed During Sprint 4**

All planned Sprint 4 user stories were completed.

## **Work Completion Rate**

### **Sprint 4 Planned Work**

* **Total user stories planned:** 3
* **Total estimated ideal work hours planned:** 73 ideal hours
* **Total story points planned:** 21 story points
* **Total sprint length:** 7 days

### **Sprint 4 Completed Work**

* **User stories completed:** 3 (added durng sp
* **User stories partially completed:** 0
* **Estimated ideal work hours completed:** 73 ideal hours
* **Estimated ideal work hours remaining:** 0 ideal hours

### **Sprint 4 Rates**

* **User stories completed per day:** 3 completed user stories / 7 days = 0.43 user stories/day
* **Ideal work hours completed per day:** 73 ideal hours / 7 days = 10.43 ideal hours/day

## **Final Burnup Chart**

![Sprint 4 Final Burnup Chart](https://raw.githubusercontent.com/goatfishplays/WheelTime/refs/heads/main/docs/images/final-burnup-chart-s4.png)

## **Sprint 4 Summary**

During Sprint 4, the team focused on turning Wheel Time from a development prototype into a more complete release candidate. The biggest areas of progress were search, action execution, settings integration, and release packaging. Users can now search for programs or actions, run actions from outside a single fixed menu slot, configure launcher behavior through the settings interface, and install the app without manually setting up Qt, CMake, or Visual Studio.

Sprint 4 also improved the first-time user experience. The team updated the installer flow, cleaned up the default menu, and made the README more useful for people downloading Wheel Time instead of building it from source. These changes make the project easier to demo, review, and hand off to new users.

Overall, Sprint 4 completed all planned user stories and moved Wheel Time into a stronger Release 1 state. The next priorities should be final bug fixes, release validation, documentation accuracy, and polish based on teammate and reviewer feedback.

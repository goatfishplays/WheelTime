# **Sprint 2 Plan**

**Product Name:** Wheel Time

**Team Name:** Wheelest Wheels

**Sprint Completion Date:** 7/7/26

**Revision:** 0

**Revision Date:** 7/2/26

## **Sprint Goal**

Allow the launcher to open and close quickly using a hotkey and run basic actions.

## **Task Listing by User Story**

### **User Story 1 \- Medium-High Priority**

As a gamer, I want to be able to bring up the interface quickly so I don't have to interrupt my playing to access an action.

**Tasks:**

* Research Windows API RegisterHotKey \- 5 hours  
* Implement global hotkey to open launcher \- 8 hours  
* Create hotkey toggle/debounce logic to prevent spamming \- 4 hours  
* Test launcher while another full-screen app is focused \- 7 hours

**Total for User Story 1:** 24 hours

**Acceptance Criteria:**

* The team has implemented a global hotkey listener using the Windows API.  
* Pressing the designated hotkey successfully renders the launcher interface on the screen.  
* The launcher overlay successfully renders on top of other active, full-screen applications.

### **User Story 2 \- Medium-High Priority**

As a gamer, I want to be able to exit the interface quickly so that if something happens while I have it up I can react to it quickly.

**Tasks:**

* Add Escape key listener to close launcher \- 6 hours  
* Implement click-outside-to-close behavior \- 4 hours  
* Return focus and mouse to the previous app after launcher closes \- 8 hours  
* Record (or fix) known bugs/open issues regarding window focus \- 4 hours

**Total for User Story 2:** 22 hours

**Acceptance Criteria:**

* The user can instantly close or hide the launcher by pressing the Escape key.  
* The launcher completely disappears from the screen when closed.  
* Upon closing, the user's mouse position is restored, and the operating system immediately returns window focus to the previously active application.

### **User Story 3 \- High Priority**

As a computer user, I want to be able to run macros/hotkeys from a visual interface so I don't have to remember all of them.

**Tasks:**

* Create an AppAction abstraction class for reusable execution \- 6 hours  
* Add test script/macro action \- 5 hours  
* Connect UI buttons to test launching Notepad/Calculator \- 8.5 hours  
* Document hotkey behavior in the repository \- 2 hours

**Total for User Story 3:** 21.5 hours

**Acceptance Criteria:**

* The codebase utilizes an abstraction layer (e.g. AppAction) so execution logic is not hardcoded directly inside UI button handlers.  
* The user can successfully launch at least two test applications (Notepad and Calculator) by clicking buttons in the UI.  
* The README is updated with instructions on how to use the hotkeys and trigger these actions.

### **User Story 4 \- Low Priority (Backlog)**

As a linux user, I want to have my uses cached so I can quickly execute the most recently used actions again faster.

**Tasks:**

* *Note: Pushed to backlog as current sprint focuses heavily on Windows API core functionality.*

**Total for User Story 4:** n/a

## **Sprint 2 Spikes**

* Research Windows API global hotkeys and window focus stealing.  
* Research simple macro/script execution in C++.

## **Sprint 2 Infrastructure Tasks**

* Update README with hotkey usage instructions and Sprint 2 build notes.  
* Track bugs and open issues in the GitHub repository.  
* Update CMake build targets to include new Windows execution .cpp files.  
* Merge UI branches and verify the app builds and tests locally on Windows.

## **Total Estimated Work**

* **User Story 1:** 24 hours  
* **User Story 2:** 22 hours  
* **User Story 3:** 21.5 hours  
* **User Story 4:** n/a

**Total Sprint 2 Estimate:** 67.5 ideal hours

## **Team Roles**

* **Ronan Wong:** Product Owner, architecture/technical direction, GUI integration  
* **Mithil Harish:** Scrum Master, scrum board coordination, meeting coordination, developer  
* **Ethan Jackson:** Developer, Windows API / hotkey research  
* **Aedan Benavides:** Developer, documentation, GUI support  
* **Muneeb Syed:** Developer, documentation, execution logic

## **Initial Task Assignment**

* **Ronan Wong:** User Story 2, hooked QT up to the project, created the radial menu and full GUI interface, and handled prior window/mouse position restoration.  
* **Aedan Benavides: U**ser Story 2, GUI stuff, UI styling, and interface support.  
* **Ethan Jackson:** User Stories 1 and 2, implemented the hotkey system with window show/hide, implemented close window with Escape, and fixed compile/build errors.  
* **Muneeb Syed:** User Story 3, connected launcher GUI buttons to test actions, updated action handling to use Executor, updated CMake build targets, and updated README.  
* **Mithil Harish:** User Story 1 and 3, Scrum Master duties, test launching Notepad/Calculator logic, and hotkey research/setup.

## **Initial Burnup Chart** (https://github.com/goatfishplays/WheelTime/blob/main/docs/images/initial-burnup-chart-s2.png)

**The initial burnup chart for Sprint 2 should show 0 completed ideal hours out of 67.5 total estimated ideal hours at the start of the sprint.**

**The Sprint 2 burnup chart will be updated as tasks are completed and should be available in the team workspace/lab.**

## **Initial Scrum Board**

### **User Stories**

* Bring up the interface quickly (hotkeys)  
* Exit the interface quickly (focus restoration)  
* Run macros/hotkeys from a visual interface

### **Tasks Not Started**

* Return focus and mouse to previous app after launcher closes  
* Create hotkey toggle/debounce logic  
* Implement click-outside-to-close behavior  
* Research Windows API RegisterHotKey  
* Implement global hotkey to open launcher  
* Test launcher while another full-screen app is focused  
* Add test script/macro action  
* Test launching Notepad/Calculator from UI  
* Document hotkey behavior in the repository  
* Add Escape key listener to close launcher  
* Record/fix known bugs/open issues regarding window focus

## **Scrum Times**

**The team will meet/check in at least three times during the sprint:**

* **Monday: 8:00 PM \- Sprint conclusion / planning**  
* **Tuesday: 5:30 PM \- Discord check-in (full meeting)**  
* **Thursday: 5:30 PM \- Sprint check-in / task distribution**

**TA/Tutor Visit: During Tuesday and Thursday meetings.**


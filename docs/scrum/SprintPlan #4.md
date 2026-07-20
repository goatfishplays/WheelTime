# Sprint 4 Plan

**Product Name:** Wheel Time

**Team Name:** Wheelest Wheels

**Sprint Completion Date:** 7/21/26

**Revision:** 0

**Revision Date:** 7/15/26

## Sprint Goal

The goal of Sprint 4 is to polish the final release of Wheel Time by improving search/action access, cleaning up the codebase, updating documentation, and testing the launcher for performance and reliability. By the end of the sprint, the team should have a stronger release candidate that is easier to use, better documented, and ready for final project review.

## Task Listing by User Story

### User Story 1 - Medium-High Priority

**As a general user, I want to be able to run any action from somewhere so if I forget what menu it is associated with I can still use it.**

**Tasks:**

* Design search/action palette flow for finding actions. **5 hours**
* Implement search action item that opens the search interface. **8 hours**
* Add filtering for action names and action types. **6 hours**
* Connect search results to existing launcher actions. **7 hours**
* Test searching for and running a hotkey action. **5 hours**
* Test searching for and running an app-launch action. **5 hours**
* Document search action behavior in the README. **3 hours**

**Total for User Story 1:** 39 hours

**Acceptance Criteria:**

* The user can open the launcher menu.
* The user can select a search action item or search mode.
* The user can search for an existing action by name or keyword.
* Matching actions are displayed or selectable.
* Selecting a search result runs the correct action.
* Search works for at least two action types, such as app launching and hotkey/key simulation.
* The search behavior is documented in the repository.

---

### User Story 2 - Medium Priority

**As a gamer, I want the interface to be lightweight so I can run it without worrying about lowering my FPS.**

**Tasks:**

* Review launcher performance during normal open/close use. **4 hours**
* Test repeated menu opening and closing for slowdown or freezing. **5 hours**
* Benchmark or observe performance with Wheel Time running in the background. **6 hours**
* Identify any UI or scheduler behavior that could cause unnecessary lag. **5 hours**
* Clean up code according to coding standards document. **5 hours**
* Fix small bugs or UI issues found during testing. **5 hours**
* Update final documentation and release notes. **4 hours**

**Total for User Story 2:** 34 hours

**Acceptance Criteria:**

* Wheel Time can run in the background without noticeable slowdown.
* The launcher can open and close repeatedly without freezing.
* Running common actions does not noticeably slow the system.
* The interface remains responsive during normal use.
* Major performance concerns or bugs are documented.
* The codebase is cleaned up according to the team’s coding standards.
* Final documents are updated for the release.

---

## Sprint 4 Spikes

* Research ways to improve search result ordering and filtering.
* Research performance testing methods for lightweight desktop applications.
* Investigate any remaining window focus, overlay, or hotkey edge cases before release.

## Sprint 4 Infrastructure Tasks

* Update all required project documents.
* Clean up code according to the coding standards document.
* Track bugs and open issues in GitHub.
* Prepare final release notes.
* Verify the project builds successfully before final submission.
* Prepare screenshots/demo material for the final presentation.

## Total Estimated Work

* User Story 1: 39 hours
* User Story 2: 34 hours

**Total Sprint 4 Estimate:** 73 ideal hours

## Team Roles

* **Ronan Wong:** Product Owner, architecture/technical direction, release planning
* **Aedan Benavides:** Scrum Master, sprint coordination, developer, documentation
* **Ethan Jackson:** Developer, testing, documentation
* **Mithil Harish:** Developer, testing, documentation
* **Muneeb Syed:** Developer, documentation, release support

## Initial Task Assignment

* **Ronan Wong:** User Story 1 and User Story 2, architecture review, search/action integration support, and final product direction
* **Aedan Benavides:** Scrum Master duties, scrum board coordination, documentation updates, and Sprint 4 progress tracking
* **Ethan Jackson:** User Story 1, search action item implementation, search testing, and documentation support
* **Muneeb Syed:** User Story 2, performance testing, final documentation updates, release notes, and code cleanup support
* **Mithil Harish:** User Story 1 and User Story 2, action search testing, bug fixes, and final build verification

## Initial Burnup Chart

![Sprint 4 Burnup Chart](https://raw.githubusercontent.com/goatfishplays/WheelTime/refs/heads/main/docs/images/initial-burnup-chart-s4.png)

**Burnup Chart Notes:**  
The burnup chart tracks the total Sprint 4 scope compared to the amount of work completed over time. The planned scope begins at 73 ideal hours and stays constant unless new tasks are added. Work completed starts at 0 hours and increases as the team finishes tasks throughout the sprint.

## Initial Scrum Board

### User Stories

* Run any action from search
* Keep the interface lightweight

### Tasks Not Started

* Design search/action palette flow
* Implement search action item
* Add filtering for action names and action types
* Connect search results to launcher actions
* Test searching for and running hotkey actions
* Test searching for and running app-launch actions
* Review launcher performance during normal use
* Test repeated menu opening and closing
* Benchmark/observe background performance
* Identify possible UI or scheduler lag
* Clean up code according to coding standards
* Fix small bugs or UI issues
* Update final documentation and release notes
* Verify final build
* Prepare final demo material

## Scrum Times

The team will meet/check in at least three times during the sprint:

* **Tuesday:** 5:30 PM - Discord check-in / task distribution
* **Thursday:** 5:30 PM - Sprint check-in / TA-tutor visit
* **Sunday:** 5:30 PM - Sprint check-in / wrap-up preparation
* **Monday:** 8:00 PM - Sprint conclusion / final review preparation
* **Tuesday:** 8:00 PM - Final Presentation and Projecet Review Practice

**TA/Tutor Visit:** During Tuesday and Thursday meetings.

# Sprint 1 Plan

**Product Name:** Wheel Time

**Team Name:** Wheelest Wheels

**Sprint Completion Date:** 6/30/26

**Revision:** 0

**Revision Date:** 6/30/26

## Sprint Goal

The goal of Sprint 1 is to create the foundation for a basic Windows launcher prototype with a simple interface. By the end of the sprint, the team should have the project repository/build structure started, initial documentation, and research completed for launching programs and creating a basic launcher window.

## Task Listing by User Story

### User Story 1 - High Priority

**As a computer user, I want to be able to launch programs and execute scripts quickly so I can run things that I use often without having to exit to desktop or type anything in a terminal.**

**Tasks:**

* Research C++ methods for launching external programs/scripts. **5 hours**
* Compare `system()`, `ShellExecute`, and `CreateProcess` for the MVP. **5 hours**
* Create a small test or research note for launching Notepad/Calculator. **5 hours**
* Document recommended app-launching approach in the repo. **2.5 hours**
* Complete required setup. **7.5 hours**

**Total for User Story 1:** 25 hours

**Acceptance Criteria:**

* The team has researched at least two C++ methods for launching external programs or scripts.
* The team has compared `system()`, `ShellExecute`, and `CreateProcess`.
* The team has a working test or documented example for launching a basic Windows app such as Notepad or Calculator.
* The recommended launching approach is documented in the repository.
* The chosen approach is realistic for the MVP and can be built on in future sprints.

---

### User Story 2 - Low Priority

**As a gamer, I want the interface to not obscure the entire screen so I can still see what is happening while performing actions.**

**Tasks:**

* Research GUI/window options for a small launcher interface. **5 hours**
* Research Windows API basics for creating a window. **7.5 hours**
* Research transparency/overlay options for future implementation. **5 hours**
* Create rough notes or design sketch for the basic launcher window. **2.5 hours**
* Complete skeleton for UI code. **15 hours**

**Total for User Story 2:** 35 hours

**Acceptance Criteria:**

* The launcher interface has a basic window or GUI skeleton.
* The GUI does not take up the full screen.
* The interface includes placeholder buttons or visible UI elements for future launcher actions.
* The team has documented research on transparency, overlay behavior, or ways to avoid blocking the screen.
* The GUI prototype can be shown through code, a screenshot, or a design sketch.
* The interface is simple enough to be expanded in future sprints.

---

## Sprint 1 Spikes

* Learn how Windows API works and interacts with graphics/window elements.
* Research app launching in C++.
* Research GUI/window options.

## Sprint 1 Infrastructure Tasks

* Create GitHub repository.
* Set up C++/CMake project.
* Add README/build steps.
* Create basic launcher window.
* Organize project files and documentation.

## Total Estimated Work

* User Story 1: 25 hours
* User Story 2: 35 hours

**Total Sprint 1 Estimate:** 60 ideal hours

## Team Roles

* **Ronan Wong:** Product Owner, architecture/technical direction, repository setup support
* **Muneeb Syed:** Scrum Master, scrum board coordination, meeting coordination
* **Ethan Jackson:** Developer, GUI/window research and prototype support
* **Aedan Benavides:** Developer/documentation, README, app-launching research, Sprint 1/Sprint 2 planning support
* **Mithil Harish:** Developer, Windows API/GUI research and prototype support

## Initial Task Assignment

* **Ronan Wong:** User Story 2, create GitHub repo and help set up C++/CMake project
* **Aedan Benavides:** User Story 2, organize scrum board and track Sprint 1 task progress
* **Ethan Jackson:** User Story 2, research GUI/window options and basic launcher window approach
* **Muneeb Syed:** User Story 1 and User Story 2, write README/build steps and research app launching in C++
* **Mithil Harish:** User Story 1, research Windows API basics and transparency/window behavior

## Initial Burnup Chart

![Sprint 1 Burnup Chart](images/initial-burnup-chart-s1.png)

**Burnup Chart Notes:**
The burnup chart tracks the total Sprint 1 scope compared to the amount of work completed over time. The planned scope begins at 60 ideal hours and stays constant unless new tasks are added. Work completed starts at 0 hours and increases as the team finishes tasks throughout the sprint.

## Initial Scrum Board

### User Stories

* Launch programs/scripts quickly
* Set up project structure/build tools
* Keep interface from obscuring full screen

### Tasks Not Started

* Research app launching in C++
* Compare `system()`, `ShellExecute`, and `CreateProcess`
* Create README/build steps
* Set up C++/CMake project
* Research GUI/window options
* Research Windows API window basics
* Create basic launcher window
* Document Sprint 1 work
* GitHub repository setup
* App launching research
* Build prototype

## Scrum Times

The team will meet/check in at least three times during the sprint:

* **Wednesday:** 8:00 PM - Discord meeting
* **Friday:** 5:30 PM - Discord meeting
* **Tuesday:** 5:30 PM - Sprint wrap-up / task board update meeting

**TA/Tutor Visit:** Not planned for Sprint 1. TA/tutor visits will begin in Sprint 2.

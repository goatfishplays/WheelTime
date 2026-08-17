## Known Problems

The following known problems, limitations, and design shortcuts are part of the Release 1.0 version.

### Windows-Focused Release

Wheel Time Release 1.0 mainly focuses on Windows functionality. Some user stories mention Linux/general launcher behavior, but full Linux support is not complete in this release. As such there is some code that may need to be moved to the platform layer or changed to support such environments(in particular search and the non-activating overlay code).

### Limited Automated GUI Testing

Most GUI and user-interaction behavior was tested manually. Automated tests exist for core logic such as action history, scheduler behavior, search palette behavior, and action item logic, but full end-to-end GUI automation is not complete. Some issues such settings menu clipping depending on the resolution may be present.

### Edge Cases for Invalid Actions

The system is expected to handle invalid or failed actions safely, but more testing is needed for unusual file paths, missing programs, permission issues, unsupported scripts, and failed executable launches.

### Hotkey and Focus Edge Cases

The launcher supports hotkey behavior, but edge cases may still exist depending on the active application, full-screen mode, Windows permissions, or focus behavior. Some games or protected applications may block overlays or input simulation. Simulated key inputs may be inconsistent when physically holding modifiers alongside them because of the way they are handled by the OS. Settings menu may steal focus due to needing keyboard inputs leading to needing to manually refocus your active application after.

### Full Screen Games May Or May Not Work

Depending on if the game runs in exclusive fullscreen mode or not the UI will not be able to render on top. To fix this we would need to perform render pipeline injection which is currently above my paygrade :sob:

### Search Limitations

Search-based launching works for expected test cases, but search quality may still need improvement for fuzzy matching, ranking, missing programs, duplicate action names, or unsupported application paths. Certain apps such as Steam have their own methods of exposing executables and not all of these methods may have been covered. Applications in your downloads folder or other non-standard install locations are currently not checked for.

### Settings Validation

The settings menu supports adding and saving actions, but more validation could be added for incorrect user input, duplicate actions, invalid hotkey bindings, and unsafe file selections.

### Design Shortcuts

Some test actions, default actions, or example programs may still be hardcoded or preconfigured for demonstration purposes. More work is needed to make all actions fully user-configurable and polished.

### Performance Testing Limitations

Basic performance and responsiveness were tested, but deeper benchmark testing is limited. More testing would be needed with actual games, high-load applications, and longer runtime sessions.

### Heavily Suggested to set deferUntilExit to True for Mouse Inputs

Currently, due to the way Windows handles mouse inputs, Wheel Time cannot pass simulated mouse clicks through to the underlying application while the menu is open. This may result in repeated action loops caused by actions triggering other actions or your inputs being eaten. That behavior may be intentional for self-repeating macros, but users should enable exit-on-action + deferUntilExit before simulating a left click if they do not want the launcher to stay open.

---



## Non-Issues

Some behaviors may seem unintuitive but are by design and are not considered problems.

### Cancel Requires Main Action to be Running

Some actions such as simulating keystrokes fire off a separate subaction for cleanup (running a keyup in this case). The main action, when set to "Continue Immediately", will finish and thus be uncancelable. This is by design but may be unintuitive to some users. If the action needs to remain cancelable, it is recommended to either unset "Continue Immediately" or add a delay to the end of the action that spans the excess duration.

### Actions Attempt Pause in Settings

Actions that contain time delays or durations will attempt to pause their delays and durations while the settings menu is open and may be canceled if the settings are updated while there to attempt to avoid invalid states.

### Menus Locked in Settings

Menus are locked while the settings menu is open. This is to prevent the user from not explicitly saving or canceling their changes. 

---



## Product Backlog

The following high-priority user stories and bug fixes should guide future work on Wheel Time.

### Original Release Plan Backlog Stories

1. **Superior ricing support (v1 landed)**
  Bundled light/dark QSS plus an optional user overlay, with global/per-menu ring radius, start angle, center deadzone, and mouse-on-open offset. Per-menu QSS files and outer selection limits are still future work.
2. **Expand automated testing and allow action item plugins**
  As a developer, I want more automated tests and extensibility for settings, action validation, action items, and launcher behavior so future changes are safer and I can add custom behaviors.
3. **Add full Linux support**
  As a Linux user, I want Wheel Time to support Linux program launching and input behavior so I can use the same launcher outside Windows.
4. **Macro recording**
  As a NVim user, I want to be able to record macros at use time so I don't have to open settings to create a macro that I will only be using for a short time.



### Other Future User Story Ideas

1. **Improve settings validation**
  As a user, I want the settings menu to clearly reject invalid input so I do not accidentally create broken launcher actions.
2. **Improve custom action management**
  As a user, I want to edit, delete, reorder, and rename launcher actions so I can fully control my wheel layout.
3. **Add stronger error messages**
  As a user, I want clear error messages when an action fails so I know what went wrong and how to fix it.
4. **Improve full-screen application support**
  As a gamer, I want Wheel Time to work reliably over full-screen and borderless-window applications so it is useful while gaming regardless of game.
5. **Improve performance testing**
  As a gamer, I want benchmark results showing Wheel Time has low impact on system performance so I can trust it during games.
6. **Add action import/export**
  As a user, I want to back up and share my launcher configurations so I can move them between devices, share them with friends, or restore them later.
7. **Polish UI and accessibility**
  As a user, I want the default launcher to be visually clear, easy to read, customizable, and comfortable to use so it feels like a finished application.
8. **Extended input simulation**
  As a user, I want to be able to simulate any input(midi, mouse scrolls, mouse paths) from the launcher so I can perform a broad range of actions required by my specific software.
9. **Running actions display**
  As a user, I want to have an optional visual of what actions are currently running or queued so I can easier tell what is happening in the background.
10. **Deadzone/Custom Layouts**
  Inner center deadzone is in ricing v1. Remaining: outer selection limits and custom button layouts beyond the original wheel.
11. **Socket Receiver**
  As a user, I want to be able to receive socket messages and automatically execute actions in response so I can make use of the replies generated when sending out socket messages.


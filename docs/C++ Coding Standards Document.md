# C++ Coding Standards

**Product / CMake / binary:** WheelTime (display name: Wheel Time)  
**Team:** Wheelest Wheels  
**Date:** 7/21/26  

## Purpose

Coding standards for the WheelTime C++ codebase: readable, consistent, and maintainable.

Applies to active code under `app/`, `platform/`, and `launcher/`. When older code disagrees, new and touched code moves toward these rules. Reference shape: `Scheduler.hpp` and newer ActionItems.

---

## Layout and identity

| Surface | Convention | Notes |
|---------|------------|--------|
| Repo / product / CMake / binary | **WheelTime** | Display name may be “Wheel Time” |
| Include path root | `App/…`, `Platform/…` | Folder name stays short |
| Namespaces | `Application`, `Platform` | Do **not** use `namespace App` |
| OS implementations | `platform/src/<os>/` | e.g. `windows/` (lowercase OS id) |

Do **not** rename `include/App` → `include/Application` for style alone: path prefix `App`, namespace `Application`. Platform must not depend on App.

---

## Header and source files

- Headers: `.hpp` + `#pragma once`. Sources: `.cpp`.
- Usually one `.hpp` per `.cpp` with the same stem (`main.cpp` is an exception).
- Headers declare; sources implement. Avoid large implementations in headers without a clear reason.
- Names: **PascalCase** matching the primary type (`Scheduler.hpp`, `MouseButton.cpp`).
- Domains as folders: `ActionItems/`, not `action_items/`.
- Tests: `app/tests/<topic>_tests.cpp` with a CMake target of the same stem. Do not leave orphan test sources unwired in CMake.

## Include order

Blank line between groups. Canonical order (matches `Scheduler.hpp` and most headers):

1. Corresponding header (in `.cpp` only)
2. C++ standard library
3. Third-party (Qt, etc.)
4. Project headers (`App/…`, `Platform/…`)
5. Windows API headers (platform / OS-specific files only)

Do **not** mass-reformat existing files for include order alone. New and touched code moves toward this order.

| From | Include | Form |
|------|---------|------|
| App / launcher | App headers | `#include "App/Scheduler.hpp"` |
| App / launcher | Platform headers | `#include <Platform/Execute.hpp>` |
| Platform | Other Platform headers | `"Platform/…"` or `<Platform/…>` — stay consistent within `platform/src/` |
| Anywhere | Qt / std | `#include <QWidget>`, `#include <vector>` |

Example (`.cpp`):

    #include "App/Scheduler.hpp"

    #include <atomic>
    #include <memory>
    #include <vector>

    #include <QTimer>

    #include "App/Action.hpp"
    #include <Platform/Execute.hpp>

## Naming

### Types

- Classes, structs, enums, type aliases: **PascalCase** (prefer `enum class`).
- ActionItem implementations: **`AI_`** + PascalCase (`AI_Keystroke`, not `AI_nthRecent`).
- Prefer full words in public API (`InputReceiver`, not `InputRcvr`).

### Functions and methods

- **camelCase** (`launchApplication()`, `selectedAction`).
- Booleans: prefer `is` / `has` / `can` (`isPaused`, `isCancelable`).
- Accessors: one style per type — prefer bare nouns (`channel()`, `name()`, `absoluteMousePosition()`) or consistent `getX()`, never both for the same field.
- Prefer full words for mutators/size (`removeAction`, `itemCount`); no leading underscore on parameters.

### Members, locals, constants

- Private / protected members: **`m_`** + camelCase.
- Public fields only for small DTOs / ActionItem value fields (`AI_Delay::durationMs`); do not mix `m_` and bare private fields on the same class.
- Locals and parameters: camelCase, no `m_`.
- Constants: `kCamelCase` or PascalCase enum / `constexpr`; avoid `SCREAMING_SNAKE` except for macros.

### Domain vocabulary

- One name per concept across app and platform boundaries.
- JSON / config tags and assets: **snake_case** (`launch_app`, `nth_recent`, `keystroke`); do not force PascalCase onto wire formats.
- Prefer a single canonical tag per item kind; do not add duplicate aliases without a migration plan.

## Functions

- Keep signature on one line when practical; one responsibility per function; split when hard to read.
- Prefer a short comment on important declarations (purpose, inputs, output).

Example:

    // Launches an external application using the provided file path.
    // Returns true if the launch request succeeds.
    bool launchApplication(const std::string& filePath);

## Local variables

- Declare near first use, in the narrowest scope; initialize on declaration; prefer `const`; do not reuse one variable for unrelated purposes.

## Braces and formatting

- **4 spaces**, no tabs. Always brace `if` / `else` / `for` / `while` / `do-while`, even for one-liners.
- Prefer Allman for new blocks in Allman files; **match the file you edit**. Dual brace placement (Allman vs K&R) is intentional until a shared `.clang-format` exists — do not reformat untouched regions for preference alone.
- Namespace body: prefer types at column 0 (`namespace Application { … }`, scheduler style).
- Lines under **100 characters** when practical.
- Language: **C++23** for active CMake targets unless a subdirectory documents otherwise.
- One blank line between function definitions; no trailing whitespace; consistent operator spacing.
- Until a shared `.clang-format` exists, match neighboring code. Clean up before submitting a PR.

Good:

    if (isVisible)
    {
        hideLauncher();
    }

Avoid:

    if (isVisible)
        hideLauncher();

## Comments and docs

- Explain **why**, not what every obvious line does. Keep comments short; remove outdated ones.
- TODOs only when real, specific, and actionable; no drive-by jokes in public headers.
- Prefer `///` for member briefs; longer notes as block comments above the declaration.
- New or substantially edited headers use compact Doxygen only:

```cpp
/**
 * @file Example.hpp
 * @brief One-line purpose.
 */
```

- Remove all `@author` lines; do not add new ones. Do not invent a second full Doxygen banner style.
- Generated HTML under `docs/html` is produced by Doxygen (`docs/Doxyfile`); it is gitignored — regenerate with `cd docs && doxygen Doxyfile` when needed. Do not treat generated HTML as a style source.
- Update the README when build steps, dependencies, or usage change. Document major design decisions; drop outdated notes.

## Magic numbers

- Prefer named `constexpr` / constants near the use site (or shared when used across files). Naming follows the Constants rule above.

    constexpr int kMenuRadius = 140;
    constexpr int kMaxVisibleActions = 8;

## Classes and API shape

- Keep members private unless there is a strong reason (DTO exception above).
- Prefer small classes with clear responsibilities. New types must not mix UI, action execution, and settings. `App` is the allowed composition root that wires Gui, Scheduler, hotkeys, and config until a future split; do not grow that mix into additional types.
- Construct objects into a valid starting state.
- Prefer clear ownership:
  - `unique_ptr` for polymorphic ActionItem sequences and runtime Menu storage (`App::loadedMenus`).
  - Menus reference actions by stable id.
  - Non-owning `Menu*` / `Action*` observers (`activeMenu()`, `findMenuById()`) are fine.
- Mark side-effect-free queries `[[nodiscard]]` when ignoring the result is a bug.
- Prefer `noexcept` on trivial getters/setters and moves when accurate.
- Prefer typed results (`ExecuteResult`) over ambiguous `bool` / out-params when that pattern is already used nearby.
- Put units in names when non-obvious (`durationMs`); do not silently mix seconds and milliseconds.
- Prefer `nullptr`, `override`, `= default` / `= delete` where appropriate.

## Ownership and RAII

- Prefer RAII. Do not use raw owning pointers.
- Qt widgets may use parented `new` (`new QWidget(parent)`); the Qt parent owns lifetime. That is the only approved raw-`new` ownership pattern.
- Prefer `std::unique_ptr` for exclusive ownership elsewhere.

## Threading and scheduling

- Scheduler owns every live `ActionExecutionContext`, channel state, and delay parking.
- Workers never make scheduling decisions; they only execute work the scheduler dispatches.
- Preserve strict FIFO ordering within channels.
- Prefer `std::jthread` over `std::thread` for new code.
- Minimize lock contention; keep shared mutable state behind clear owners (scheduler thread, mutex-guarded queues/history).
- Avoid global state beyond the existing App singleton composition root.

## Error handling

- Check failures when launching programs, opening files, saving settings, or registering hotkeys.
- Do not let invalid user input crash the app.
- Prefer typed results (`ExecuteResult`, status enums) when that pattern is already used nearby.
- Exceptions only for programmer/contract errors (e.g. null Action passed to a context ctor). Do not invent a second error style for the same layer.
- Log or document important failures during testing.

## Qt / GUI

- Keep UI layout separate from action execution; handlers call into action/executor types.
- Descriptive widget names; keep the launcher overlay lightweight and responsive.
- Parented Qt `new` is allowed (see Ownership).

## Windows / platform

- Keep OS-specific logic in `platform/src/<os>/`. Comment non-obvious API calls; check return values (hotkeys, focus, process launch).

## Configuration

- Save/load user actions through the settings/config system; do not require code edits to add actions.
- Validate paths, keystrokes, and names before saving; handle missing or invalid config safely.

## Testing

- Manual tests live in the test plan/report; automated tests under `app/tests`.
- Hand-rolled `main` + `bool test…()` helpers; keep that pattern consistent within a file.
- File-scoped `using namespace Application;` is allowed in `.cpp` and test mains. Prefer specific `using Application::Type;` in headers; never put broad `using namespace` in headers.
- Share fakes via a common helper when a third copy would appear.
- Tests may include App and Platform headers. Cover logic that can run without the GUI (actions, history, search, scheduler, settings validation).
- Build and run locally before merging major changes.

## CMake

- Libraries: PascalCase (`App`, `Platform`). Executables: `WheelTime`.
- Test targets: snake_case stems matching the `.cpp` file name.
- List new sources in the owning `CMakeLists.txt`; wire or remove orphans.

## Git and pull requests

- Clear branch names (`feature/search-palette`, `fix/hotkey-close`).
- Focused PRs; clear commit messages; no build artifacts or personal IDE files.
- Review and confirm the project builds before merging major changes.

## Deliberately allowed

- JSON snake_case vs C++ PascalCase (wire format ≠ C++ naming).
- `App/` path vs `Application` namespace (historical; don’t churn paths for style alone).
- Public fields on small ActionItem DTOs (infrastructure types use `m_`).
- `App` as composition root wiring Gui + Scheduler + settings/hotkeys.
- Qt parented-`new` for widgets.
- Dual brace placement across subsystems until clang-format lands.
- File-scoped `using namespace Application;` in `.cpp` / tests.

## Summary

Prefer consistency. When in doubt, match `app/include/App/Scheduler.hpp` and ActionItems under `app/include/App/ActionItems/`, and choose the form that is easiest for another teammate to follow.

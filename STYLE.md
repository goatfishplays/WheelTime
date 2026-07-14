# WheelTime style guide

Conventions for active code under `app/`, `platform/`, and `launcher/`.
When older code disagrees with this file, new and touched code should move toward
these rules. Treat `Scheduler.hpp` / newer ActionItems as the reference shape.

---

## Layout and identity

| Surface | Convention | Notes |
|---------|------------|--------|
| Repo / product | **WheelTime** | Display name may be “Wheel Time” |
| CMake `project()` / binary | **WheelTime** | Matches repo / product name |
| Include path root | `App/…`, `Platform/…` | Folder name stays short |
| Namespaces | `Application`, `Platform` | Do **not** use `namespace App` |
| OS implementations | `platform/src/<os>/` | e.g. `windows/` (lowercase OS id) |

Do **not** rename `include/App` → `include/Application` for style alone.
Document the split instead: path prefix `App`, namespace `Application`.

---

## Files and directories

- Headers: `.hpp` + `#pragma once` (no include guards).
- Sources: `.cpp`.
- Names: **PascalCase** matching the primary type (`Scheduler.hpp`, `MouseButton.cpp`).
- Domains as folders: `ActionItems/`, not `action_items/`.
- Tests: `app/tests/<topic>_<kind>.cpp` with CMake target of the same stem.
  - Use `*_tests` for kept regression suites; `*_smoke` only for disposable phase checks.
  - Prefer `action_items` over glued `actionitems` in new names.
- Assets / JSON: **snake_case** or existing resource names; do not force PascalCase onto wire formats.

---

## Naming

### Types

- Classes, structs, enums, type aliases: **PascalCase**.
- Prefer `enum class` with PascalCase enumerators.
- ActionItem implementations: prefix **`AI_`** + PascalCase remainder.
  - Good: `AI_Keystroke`, `AI_MouseButton`, `AI_NthRecent`.
  - Bad: `AI_nthRecent` (lowercase after prefix).
- Avoid unexplained abbreviations in new public API (`InputReceiver` over `InputRcvr` for new names; existing abbreviations may stay until renamed intentionally).

### Functions and methods

- **camelCase**.
- Booleans: prefer `is` / `has` / `can` prefixes (`isPaused`, `isCancelable`).
- Accessors: pick **one** style per type and stick to it.
  - Preferred for library types: bare nouns for trivial getters (`channel()`, `name()`), or consistent `getX()` — not both for the same field.
  - Do not add a second alias (`getItems()` **and** `items()`).
- Prefer full words for mutators/size: `removeAction`, `itemCount` / `numItems` — avoid `remAction`, `len` in new or refurbished API.
- Parameters: no leading underscore (`inputBind`, not `_inputBind`).

### Members and locals

- Private / protected data members: **`m_`** + camelCase (`m_mailbox`, `m_channel`).
- Public fields are allowed only for small DTOs / ActionItem value fields (`AI_Delay::duration`). Document that choice on the type; do not mix `m_` and bare private fields on the same class.
- Locals and parameters: camelCase, no `m_`.
- Constants: `kCamelCase` or `PascalCase` enum / `constexpr`; avoid `SCREAMING_SNAKE` except for macros.

### Domain vocabulary

- One name per concept across app and platform boundaries.
  - Example debt: cancel “latest” vs “most recent” — consolidate when those headers are edited.
- JSON / config type tags: **snake_case** (`launch_app`, `nth_recent`, `hotkey`).
  - Prefer a single canonical tag per item kind; do not add duplicate aliases (`hotkey` vs `keystroke`) without a migration plan.

---

## Includes

Order (blank line between groups):

1. Corresponding header (in `.cpp`)
2. Project headers
3. Third-party (Qt, etc.)
4. C++ standard library

Style:

| From | Include | Form |
|------|---------|------|
| App / launcher code | App headers | `#include "App/Scheduler.hpp"` |
| App / launcher code | Platform headers | `#include <Platform/Execute.hpp>` |
| Platform headers | Other Platform headers | `"Platform/…"` or `<Platform/…>` — pick one per file set; implementations under `platform/src/` should be consistent with each other |
| Anywhere | Qt / std | `#include <QWidget>`, `#include <vector>` |

Platform must not depend on App.

---

## Classes and API shape

- Prefer clear ownership: `unique_ptr` for polymorphic sequences; menus reference actions by stable id.
- Mark side-effect-free queries `[[nodiscard]]` when ignoring the result is a bug (`cancelable()`, `execute` results).
- Prefer `noexcept` on trivial getters/setters and moves when accurate.
- Out parameters and boolean “success” returns: prefer typed results (`ExecuteResult`) over ambiguous `bool` when already used nearby.
- Units in names or comments when non-obvious (`durationMs` vs bare `duration`); do not silently mix seconds and milliseconds for similar fields.

---

## Comments and docs

New or substantially edited headers should use the compact form:

```cpp
/**
 * @file Example.hpp
 * @brief One-line purpose.
 */
```

Rules:

- No placeholder authors (`your name (you@domain.com)`).
- Optional `@author` with a real identity is fine on Platform headers that already use it; do not invent a second full Doxygen banner style for App.
- Prefer `///` for member briefs; longer notes as block comments above the declaration.
- Keep TODOs actionable and specific; avoid drive-by jokes in public headers.
- Generated Doxygen under `docs/html` is not a style source; regenerate after namespace/API renames.

---

## Formatting and language

- Language: C++23 for active CMake targets unless a subdirectory documents otherwise. Do not quietly lower the standard on new targets.
- Indentation: 4 spaces (match surrounding file).
- Braces: Allman-style / existing file style — **match the file you edit**; do not reformat untouched regions for preference alone.
- Namespace body: prefer types at column 0 inside `namespace Application { … }` (scheduler style). When editing heavily indented older GUI headers, migrate the touched type toward that layout only if the diff stays focused.
- Prefer `nullptr`, `override`, `= default` / `= delete` where appropriate.

A `.clang-format` may be added later to encode brace/indent rules; until then, match neighboring code.

---

## Tests

- Hand-rolled `main` + `bool test…()` helpers is the current pattern; keep it consistent within a file.
- Prefer `using Application::SpecificType;` over broad `using namespace Application;` in new tests when practical.
- Share fakes (`CountingItem`, delay stubs) via a small common helper when a third copy would appear — do not keep forking duplicates across phase files.
- Tests may include both App and Platform headers.

---

## CMake

- Library targets: PascalCase (`App`, `Platform`).
- Executables: match the chosen product spelling (`WheelTime`).
- Test targets: snake_case stems matching the `.cpp` file name.
- New sources must be listed in the owning `CMakeLists.txt`. Orphan headers/sources in-tree should be wired or removed, not left ambiguous.

---

## What this guide deliberately allows

- **JSON snake_case vs C++ PascalCase** — wire format is not C++ naming.
- **`App/` path vs `Application` namespace** — historical; document, don’t churn paths.
- **Public fields on ActionItems** — small serializable items stay simple; infrastructure types use `m_`.

---

## Known debt (fix toward this guide when touching)

1. Mixed `getX()` accessors vs bare nouns on older GUI types (`getName` vs `channel`)
2. Public fields on `App` / `Menu` (hotkey binds, collections) vs `m_` infrastructure style
3. Dual JSON load alias `keystroke` for canonical `hotkey` (keep until configs migrate)
4. Test target naming (`*_smoke` vs `*_tests`, glued `actionitems`) and duplicated fakes
5. Abbreviated Platform names (`InputRcvr`) retained for ABI chill — rename when convenient
6. Generated Doxygen under `docs/html` may still mention obsolete names until regenerated

When in doubt: match `app/include/App/Scheduler.hpp` and the ActionItems under `app/include/App/ActionItems/`, then sharpen names on the next focused cleanup PR.

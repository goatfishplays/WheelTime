# WheelTime

WheelTime is a Windows radial launcher for quick actions, app launches, hotkeys, search, and small macro chains. It is designed for people who want fast access to common actions without leaving their current workflow.

## Features

- Global hotkeys open configured radial menus.
- Radial wheel executes reusable actions.
- Settings editor manages menus, menu hotkeys, action slots, and action items.
- Built-in action items include keystrokes, app launch, delay, search palette, cancel, menu switching, mouse actions, socket actions, and close.
- Search palette can search actions, menus, programs, and optional web search.
- User settings are saved to a writable app config folder after first run.

## Get WheelTime

### Option 1: Installer (Recommended Download)

[Download WheelTime Installer](https://github.com/goatfishplays/WheelTime/releases/latest/download/WheelTimeSetup.exe)

- Installs per user under `%LOCALAPPDATA%\WheelTime`.
- Does not require admin rights.
- Adds normal app shortcuts when installer support is available.
- Windows may show an unknown-publisher warning because the installer is not code-signed.

### Option 2: Portable Zip (Portable option as backup)

Download `WheelTime-portable-windows.zip` from a release.

1. Extract the zip anywhere.
2. Open the extracted folder.
3. Run `WheelTime.exe`.

The portable package includes the Qt runtime files, so users do not need Qt, CMake, Visual Studio, or the source repo.

## Basic Usage

Run `WheelTime.exe`, then use the configured menu hotkeys from `config/default_menu.json`.

Current default menu hotkeys:

- `Ctrl+Shift+Backtick` opens `Main`.

In the launcher:

- Move the mouse to select a wheel action.
- Left click to run the selected action.
- Right click to close the overlay.
- Use `Settings` or `Edit` to change menus and actions.

On first run, WheelTime copies the bundled default config into the user's app config folder. After that, settings edits save to the user config, not the installed template.

## Developer Setup

### Requirements

- Windows
- CMake 4.3+
- Visual Studio Build Tools 2022 with Desktop development for C++
- Qt 6 MSVC 2022 64-bit

### Configure And Build

From the project root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"
cmake --build build --config Debug
```

Run the app from the project root:

```powershell
.\build\Debug\WheelTime.exe
```

If you are inside a subfolder such as `docs`, go back to the repo root first:

```powershell
cd "C:\Coding Projects\115A\WheelTime"
```

### Build Tests

The CMake build also creates test executables in `build\Debug`.

Examples:

```powershell
.\build\Debug\scheduler_tests.exe
.\build\Debug\search_palette_tests.exe
```

## Release Packaging

From the project root:

```powershell
.\scripts\package-windows.ps1
```

The script:

- configures a Release build in `build-package`
- builds `WheelTime.exe`
- installs a portable staging folder
- runs Qt deployment to copy required Qt DLLs/plugins
- creates `artifacts\WheelTime-portable-windows.zip`
- creates `artifacts\WheelTimeSetup.exe` only when NSIS is installed

If Qt is installed somewhere else, pass its MSVC prefix:

```powershell
.\scripts\package-windows.ps1 -QtPrefix "C:\Qt\6.11.1\msvc2022_64"
```

NSIS is optional. Without NSIS, the portable zip is still produced.

## Repository Layout

- `launcher/` contains the executable entry point.
- `app/` contains menus, actions, settings UI, search palette, scheduler, and radial UI.
- `platform/` contains Windows-specific input, window, launching, and key/mouse execution code.
- `config/default_menu.json` is the bundled default config template.
- `resources/styles/defaultWheel.qss` contains the Qt styling.
- `scripts/package-windows.ps1` builds release artifacts.

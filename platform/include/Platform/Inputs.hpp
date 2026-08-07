/**
 * @file Inputs.hpp
 * @brief OS-agnostic mouse position and global hotkey registration.
 */

#pragma once

#include <memory>
#include <string>

namespace Platform
{

/// @brief Invoked when a registered hotkey chord fires (RegisterHotKey or LL hook).
using HotkeyTriggeredHandler = void (*)(int hotkeyId, void *userData);

/**
 * @brief One-shot chord capture for settings recording.
 *
 * @p mod / @p vk are 0 when the user cancelled with Escape.
 */
using ChordCaptureHandler = void (*)(int mod, int vk, void *userData);

// TODO: Define a shared legend for special keys, mouse buttons, MIDI, etc.
struct InputBinding
{
    int input;
    int mod;
    /// @brief When true, capture via WH_KEYBOARD_LL instead of RegisterHotKey.
    bool useLowLevelHook = false;
};

struct Vec2
{
    int x;
    int y;
};

/// @brief OS-agnostic input receiver for cursor position and hotkeys.
class InputReceiver
{
public:
    InputReceiver();
    ~InputReceiver();

    InputReceiver(const InputReceiver &) = delete;
    InputReceiver &operator=(const InputReceiver &) = delete;

    /// @brief Get absolute mouse position in screen coordinates.
    ///
    /// Note: Position is x: left to right, y: top to bottom.
    /// TODO: Clarify multi-monitor coordinate space (virtual desktop vs primary).
    Vec2 absoluteMousePosition();

    /// @brief Moves the system cursor to an absolute screen position.
    void setAbsoluteMousePosition(Vec2 position);

    /// @brief Registers an input to be tracked (keyboard hotkeys for now).
    void registerInputBinding(InputBinding);

    /// @brief Unregisters an input being tracked.
    void unregisterInputBinding(InputBinding);

    /**
     * @brief Handler for LL-hook (and optionally other) hotkey deliveries.
     *
     * Called from the keyboard hook thread context for LL matches — keep it
     * short (e.g. queue work onto the UI loop). RegisterHotKey still arrives
     * via WM_HOTKEY / isHotkeyMessage.
     */
    void setHotkeyTriggeredHandler(HotkeyTriggeredHandler handler, void *userData);

    /**
     * @brief Install (or reuse) the LL hook to capture the next physical chord.
     *
     * Used by the settings trigger recorder so shell-reserved chords like Win+R
     * can be bound. Escape reports (0, 0). Call endChordCapture when finished.
     */
    void beginChordCapture(ChordCaptureHandler handler, void *userData);
    /// @brief Stops one-shot capture and uninstalls the hook when unused.
    void endChordCapture();

    /// @brief Check if a native message is a hotkey message.
    static bool isHotkeyMessage(void *message, int &hotkeyIdOut);

    /// @brief True if @p vk is currently physically down.
    [[nodiscard]] bool isVirtualKeyDown(int vk) const;

    /**
     * @brief True if the hotkey chord is still held.
     *
     * Requires @p vk down and every modifier bit in @p mod still down.
     * @p mod uses the same flags as RegisterHotKey (without MOD_NOREPEAT).
     */
    [[nodiscard]] bool isChordHeld(int mod, int vk) const;

    /**
     * @brief True when the primary key and every required modifier are up.
     *
     * Used for execute-on-release so injected keys are not combined with
     * leftover Ctrl/Shift/Alt/Win from the launcher chord.
     */
    [[nodiscard]] bool isChordFullyReleased(int mod, int vk) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Platform

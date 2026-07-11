/**
 * @file ActionItems.hpp
 * @brief Declares the individual building blocks that compose an `Action`.
 */

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <Platform/Inputs.hpp>

namespace Application
{
    /**
     * @brief Runtime/editor-visible discriminator for supported action item types.
     *
     * The settings UI uses this to decide which editor to show and how to
     * summarize items in the action sequence list.
     */
    enum class ActionItemKind
    {
        Base,
        LaunchApp,
        Script,
        Delay,
        Menu,
        Close,
        Keystroke,
        Socket,
        NthRecent,
        NthFrequent
    };

    /**
     * @brief Base class for one executable step inside an `Action`.
     */
    class ActionItem
    {
    public:
        ActionItem();
        virtual ~ActionItem();
        virtual std::unique_ptr<ActionItem> clone() const;
        virtual ActionItemKind kind() const;
        /**
         * @brief To be overwritten performs the said action
         *
         * TODO: Think this needs to be passed the application, discuss this later
         */
        virtual void execute();
    };

    /// @brief Advanced fallback that launches an arbitrary app or script path.
    class AI_Script : public ActionItem
    {
    public:
        AI_Script(std::string _filepath = "");
        std::string filepath;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Launches either a curated preset target or a custom app target.
     *
     * This is the non-scripter-friendly launch item surfaced first in the
     * settings toolbox. Presets keep a friendly display name in config while
     * still resolving to a concrete launch target at runtime.
     */
    class AI_LaunchApp : public ActionItem
    {
    public:
        AI_LaunchApp(std::string _presetId = "custom", std::string _customTarget = "");
        std::string presetId;
        std::string customTarget;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Simulates a hotkey with optional modifiers and hold behavior.
     */
    class AI_Keystroke : public ActionItem
    {
    public:
        AI_Keystroke(int _keycode = 0, int _modifiers = 0, float _holdDuration = 0.0f, bool _proceed = false);
        /// @brief Virtual-key code for the main key in the combo.
        int keycode;
        /// @brief Windows MOD_* bitmask for Ctrl/Alt/Shift/Win.
        int modifiers;
        /// @brief Optional hold time, in seconds, before considering the item done.
        float holdDuration;
        /// @brief Continue to next `ActionItem` immediately if `true` else, waits till keystroke finished
        // * This isn't needed from a technical standpoint as it can be accomplished with the delay AI but from a user standpoint it should be helpful
        // * Note that non-global hotkeys will probs be eaten by this window if not preceeded by AI_Close
        bool proceed;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /// @brief Waits before the next action item in the sequence executes.
    class AI_Delay : public ActionItem
    {
    public:
        AI_Delay(int _duration = 0);
        int duration;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /// @brief Switches the launcher to another menu by stable menu ID.
    class AI_Menu : public ActionItem
    {
    public:
        AI_Menu(std::string _menuId = "");
        std::string menuId;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /// @brief Closes the currently visible launcher UI.
    class AI_Close : public ActionItem
    {
    public:
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Sends a socket message
     *
     */
    class AI_Socket : public ActionItem
    {
    public:
        std::string socketMsg;
        std::string outputDst;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Executes the `n`th most recent unique action
     *
     */
    class AI_nthRecent : public ActionItem
    {
    public:
        int n;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Executes the `n`th most frequently used action
     *
     */
    class AI_nthFrequent : public ActionItem
    {
    public:
        int n;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };
}

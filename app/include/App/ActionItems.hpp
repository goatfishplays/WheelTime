
/**
 * @brief Represents an individual action to be taken
 *
 * Can be extended via inheritance
 *
 */

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <Platform/Inputs.hpp>

namespace Application
{
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

    /**
     * @brief Runs a specified script
     *
     */
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
     * @brief Launches a preset app target or a browsed custom app target
     *
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
     * @brief Runs a specified keystroke
     *
     */
    class AI_Keystroke : public ActionItem
    {
    public:
        AI_Keystroke(int _keycode = 0, int _modifiers = 0, float _holdDuration = 0.0f, bool _proceed = false);
        int keycode;
        int modifiers;
        float holdDuration;
        /// @brief Continue to next `ActionItem` immediately if `true` else, waits till keystroke finished
        // * This isn't needed from a technical standpoint as it can be accomplished with the delay AI but from a user standpoint it should be helpful
        // * Note that non-global hotkeys will probs be eaten by this window if not preceeded by AI_Close
        bool proceed;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Delays the next action in sequence
     *
     */
    class AI_Delay : public ActionItem
    {
    public:
        AI_Delay(int _duration = 0);
        int duration;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Opens another menu
     *
     */
    class AI_Menu : public ActionItem
    {
    public:
        AI_Menu(std::string _menuId = "");
        std::string menuId;
        std::unique_ptr<ActionItem> clone() const override;
        ActionItemKind kind() const override;
        void execute() override;
    };

    /**
     * @brief Closes the menu
     *
     */
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

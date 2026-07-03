
/**
 * @brief Represents an individual action to be taken
 *
 * Can be extended via inheritance
 *
 */

#pragma once
#include <vector>
#include <Platform/Inputs.hpp>
#include "App/ActionItems.hpp"

namespace Application
{

    class ActionItem
    {
    public:
        ActionItem();
        ~ActionItem();
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
        std::string filepath;
        void execute() override;
    };

    /**
     * @brief Runs a specified keystroke
     *
     */
    class AI_Keystroke : public ActionItem
    {
    public:
        int keycode;
        int modifiers;
        float holdDuration;
        /// @brief Continue to next `ActionItem` immediately if `true` else, waits till keystroke finished
        // * This isn't needed from a technical standpoint as it can be accomplished with the delay AI but from a user standpoint it should be helpful
        // * Note that non-global hotkeys will probs be eaten by this window if not preceeded by AI_Close
        bool proceed;
        void execute() override;
    };

    /**
     * @brief Delays the next action in sequence
     *
     */
    class AI_Delay : public ActionItem
    {
    public:
        float duration;
        void execute() override;
    };

    /**
     * @brief Opens another menu
     *
     */
    class AI_Menu : public ActionItem
    {
    public:
        std::string settingsFilepath;
        void execute() override;
    };

    /**
     * @brief Closes the menu
     *
     */
    class AI_Close : public ActionItem
    {
    public:
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
        void execute() override;
    };
}
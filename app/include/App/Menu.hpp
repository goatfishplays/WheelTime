/**
 * @file Menu.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <string>
#include <vector>
#include <Platform/Inputs.hpp>
#include "App/Action.hpp"
#include "App/Menu.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Application
{

    class Menu
    {
    public:
        /// @brief InputBind to trigger this menu, if not set then this will likely only be being used as a recursive menu
        // TODO: Might be better to have app control hotkeys
        Platform::InputBind *inputBind;

        /// @brief On hotkey release will automatically execute the action under the mouse
        bool executeOnRelease;

        /// @brief Automatically exit the menu on performing an action(as opposed to individual actions requiring exit, allows people to spam buttons easier)
        // TODO just make it so that it prepends exit if true
        bool exitOnAction;

        Menu(Platform::InputBind *_inputBind = nullptr,
             bool _executeOnRelease = false,
             bool _exitOnAction = false,
             std::string name = "Unnamed Menu",
             std::vector<Action> _actions = {});
        ~Menu();

        /**
         * @brief Add an action to this menu
         *
         * @param ind Index of action to add
         * @param action Action to add
         */
        void addAction(int ind, Action action);

        /**
         * @brief Remove an action from this menu
         *
         * @param ind Index of action to remove
         */
        void remAction(int ind);

        /**
         * @brief Gets the number of actions in this menu
         *
         * @return int
         */
        int numActions();

void Menu::save(const std::string &filepath)
{
    json j;
    j["name"] = getName();
    j["executeOnRelease"] = executeOnRelease;
    j["exitOnAction"] = exitOnAction;

    if (inputBind != nullptr)
        j["inputBind"] = { {"input", inputBind->input}, {"mod", inputBind->mod} };
    else
        j["inputBind"] = nullptr;

    json actionsJson = json::array();
    for (const auto &action : actions)
        actionsJson.push_back(action.toJson());
    j["actions"] = actionsJson;

    std::ofstream file(filepath);
    if (!file.is_open())
        return;

    file << j.dump(4);
    this->filepath = filepath;
}


void Menu::load(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return;

    json j;
    try { file >> j; }
    catch (const json::parse_error &) { return; } 
    name = j.value("name", "Unnamed Menu");
    executeOnRelease = j.value("executeOnRelease", false);
    exitOnAction = j.value("exitOnAction", false);

    if (j.contains("inputBind") && !j["inputBind"].is_null())
    {
        if (inputBind == nullptr)
            inputBind = new Platform::InputBind();
        inputBind->input = j["inputBind"].value("input", 0);
        inputBind->mod   = j["inputBind"].value("mod", 0);
    }
    else
    {
        delete inputBind;
        inputBind = nullptr;
    }

    actions.clear();
    if (j.contains("actions") && j["actions"].is_array())
    {
        for (const auto &actionJson : j["actions"])
        {
            Action action;
            action.fromJson(actionJson);
            actions.push_back(std::move(action));
        }
    }

    this->filepath = filepath;
}

}

        std::string getName() const;

        /// @brief Generally avoid directly modifying this
        std::vector<Action> actions;

    private:
        std::string name;
        std::string filepath; // TODO: need to auto delete old on change?
    };
}
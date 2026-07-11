/**
 * @file MenuConfigLoader.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <QString>
#include <vector>

#include "App/Action.hpp"
#include "App/Menu.hpp"

namespace Application
{
    /**
     * @brief Converts between the JSON config file and runtime menu/action models.
     *
     * This is the single format boundary for the app. It accepts both the older
     * menu-owned action schema and the newer shared action-library schema so the
     * rest of the runtime can stay on one internal model.
     */
    class MenuConfigLoader
    {
    public:
        /// @brief Loads actions and menus from disk into runtime objects.
        static bool loadConfig(const QString &filepath, std::vector<Action> &actions, std::vector<Menu *> &menus);
        /// @brief Saves the current shared action library and menu references.
        static bool saveConfig(const QString &filepath, const std::vector<Action> &actions, const std::vector<Menu *> &menus);
        /// @brief Finds the default repo-local config path using common run dirs.
        static QString defaultConfigPath();
    };
}

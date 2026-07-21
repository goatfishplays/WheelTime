/**
 * @file MenuConfigLoader.hpp
 * @brief JSON config ↔ runtime menu/action model conversion.
 */

#pragma once

#include <QString>
#include <vector>

#include "App/App.hpp"
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
        static bool loadConfig(const QString &filepath, AppConfig &appConfig, std::vector<Action> &actions, std::vector<Menu *> &menus);
        /// @brief Saves the current shared action library and menu references.
        static bool saveConfig(const QString &filepath, const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu *> &menus);
        /// @brief Returns the writable user config path, seeding it from the bundled default when needed.
        static QString defaultConfigPath();
    };
}

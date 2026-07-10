#pragma once

#include <QString>
#include <vector>

#include "App/Action.hpp"
#include "App/Menu.hpp"

namespace Application
{
    class MenuConfigLoader
    {
    public:
        static bool loadConfig(const QString &filepath, std::vector<Action> &actions, std::vector<Menu *> &menus);
        static bool saveConfig(const QString &filepath, const std::vector<Action> &actions, const std::vector<Menu *> &menus);
        static QString defaultConfigPath();
    };
}

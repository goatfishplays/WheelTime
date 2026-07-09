#pragma once

#include <QString>
#include <vector>

#include "App/Menu.hpp"

namespace Application
{
    class MenuConfigLoader
    {
    public:
        static bool loadMenus(const QString &filepath, std::vector<Menu *> &menus);
        static bool loadDefaultMenus(std::vector<Menu *> &menus);
        static bool saveMenus(const QString &filepath, const std::vector<Menu *> &menus);
        static QString defaultConfigPath();
    };
}

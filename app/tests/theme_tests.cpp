/**
 * @file theme_tests.cpp
 * @brief Tests for rice resolve, overlay path handling, and config round-trip.
 */

#include "App/Action.hpp"
#include "App/ActionItems.hpp"
#include "App/Menu.hpp"
#include "App/MenuConfigLoader.hpp"
#include "App/Theme.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace Application;

namespace
{

bool testResolveInheritsWhenEmpty()
{
    RiceSettings global;
    global.buttonRadiusFraction = 0.40;
    global.buttonRadiusPx = 180;
    global.startAngleDegrees = 15.0;
    global.innerDeadzoneFraction = 0.25;
    global.innerDeadzonePx = 40;

    const RiceSettings resolved = Theme::resolve(global, RiceOverrides{});
    if (resolved != global)
    {
        std::cerr << "resolve: empty overrides should inherit all global rice\n";
        return false;
    }
    return true;
}

bool testResolveRadiusOverrideClearsGlobalPx()
{
    RiceSettings global;
    global.buttonRadiusFraction = 0.32;
    global.buttonRadiusPx = 200;
    global.startAngleDegrees = 10.0;
    global.innerDeadzoneFraction = 0.2;

    RiceOverrides overrides;
    overrides.buttonRadiusFraction = 0.40;

    const RiceSettings resolved = Theme::resolve(global, overrides);
    if (resolved.buttonRadiusFraction != 0.40)
    {
        std::cerr << "resolve: fraction override not applied\n";
        return false;
    }
    if (resolved.buttonRadiusPx.has_value())
    {
        std::cerr << "resolve: fraction override should not inherit global px\n";
        return false;
    }
    if (resolved.startAngleDegrees != 10.0 || resolved.innerDeadzoneFraction != 0.2)
    {
        std::cerr << "resolve: non-radius fields should inherit\n";
        return false;
    }
    return true;
}

bool testResolveDeadzonePxOverride()
{
    RiceSettings global;
    global.innerDeadzoneFraction = 0.35;

    RiceOverrides overrides;
    overrides.innerDeadzonePx = 80;

    const RiceSettings resolved = Theme::resolve(global, overrides);
    if (!resolved.innerDeadzonePx.has_value() || *resolved.innerDeadzonePx != 80)
    {
        std::cerr << "resolve: deadzone px override missing\n";
        return false;
    }
    if (resolved.innerDeadzoneFraction != 0.0)
    {
        std::cerr << "resolve: px-only deadzone override should reset fraction to 0\n";
        return false;
    }
    return true;
}

bool testResolveMouseOffsetPartialOverride()
{
    RiceSettings global;
    global.mouseOpenOffsetXFraction = 0.10;
    global.mouseOpenOffsetYFraction = 0.04;

    RiceOverrides overrides;
    overrides.mouseOpenOffsetXFraction = -0.05;

    const RiceSettings resolved = Theme::resolve(global, overrides);
    if (resolved.mouseOpenOffsetXFraction != -0.05)
    {
        std::cerr << "resolve: mouse X override not applied\n";
        return false;
    }
    if (resolved.mouseOpenOffsetYFraction != 0.04)
    {
        std::cerr << "resolve: mouse Y should inherit when only X is overridden\n";
        return false;
    }
    return true;
}

bool testResolveClampsMouseOffset()
{
    RiceSettings global;
    global.mouseOpenOffsetXFraction = 0.9;
    global.mouseOpenOffsetYFraction = -0.9;

    const RiceSettings fromGlobal = Theme::resolve(global, RiceOverrides{});
    if (fromGlobal.mouseOpenOffsetXFraction != kMaxMouseOpenOffsetFraction
        || fromGlobal.mouseOpenOffsetYFraction != kMinMouseOpenOffsetFraction)
    {
        std::cerr << "resolve: global mouse offset should clamp to +/-0.5\n";
        return false;
    }
    return true;
}

bool testResolveClampsRadius()
{
    RiceSettings global;
    global.buttonRadiusFraction = 0.99;

    RiceOverrides overrides;
    overrides.buttonRadiusFraction = -0.1;

    const RiceSettings fromGlobal = Theme::resolve(global, RiceOverrides{});
    if (fromGlobal.buttonRadiusFraction != kMaxButtonRadiusFraction)
    {
        std::cerr << "resolve: global radius should clamp to max\n";
        return false;
    }

    const RiceSettings fromOverride = Theme::resolve(global, overrides);
    if (fromOverride.buttonRadiusFraction != kMinButtonRadiusFraction)
    {
        std::cerr << "resolve: override radius should clamp to min\n";
        return false;
    }
    return true;
}

bool testComposeEmptyOverlay()
{
    const QString sheet = Theme::composeStylesheet(false, QString());
    if (sheet.contains("this-must-not-appear"))
    {
        std::cerr << "compose: empty overlay path should not append junk\n";
        return false;
    }
    return true;
}

bool testComposeAppendsOverlay()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "compose: temp dir failed\n";
        return false;
    }

    const QString overlayPath = dir.filePath("overlay.qss");
    QFile overlay(overlayPath);
    if (!overlay.open(QFile::WriteOnly | QFile::Truncate))
    {
        std::cerr << "compose: could not write overlay\n";
        return false;
    }
    overlay.write("QLabel#radialCenterLabel { color: #ff00ff; }\n");
    overlay.close();

    const QString sheet = Theme::composeStylesheet(false, overlayPath);
    if (!sheet.contains("radialCenterLabel") || !sheet.contains("#ff00ff"))
    {
        std::cerr << "compose: overlay contents were not appended\n";
        return false;
    }
    return true;
}

bool testResolveOverlayPathRelative()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "overlay path: temp dir failed\n";
        return false;
    }

    const QString configPath = dir.filePath("default_menu.json");
    const QString resolved = Theme::resolveOverlayPath("themes/my.qss", configPath);
    const QString expected = QFileInfo(QDir(dir.path()).filePath("themes/my.qss")).absoluteFilePath();
    if (QFileInfo(resolved).absoluteFilePath() != expected)
    {
        std::cerr << "overlay path: relative resolve mismatch\n";
        return false;
    }
    if (!Theme::resolveOverlayPath("  ", configPath).isEmpty())
    {
        std::cerr << "overlay path: blank should be empty\n";
        return false;
    }
    return true;
}

bool testConfigRiceRoundTrip()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "roundtrip: temp dir failed\n";
        return false;
    }

    std::vector<Action> actions;
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Close>());
        actions.emplace_back(std::move(items), "Close", "", "action-close", 0);
    }

    RiceOverrides menuRice;
    menuRice.buttonRadiusFraction = 0.40;
    menuRice.startAngleDegrees = 30.0;
    menuRice.mouseOpenOffsetXFraction = 0.03;
    menuRice.mouseOpenOffsetYFraction = 0.0;

    auto menu = std::make_unique<Menu>(
        0, 0, false, false, true, false, false, "Rice Menu",
        std::vector<std::string>{"action-close"}, "menu-rice");
    menu->setRice(menuRice);

    std::vector<std::unique_ptr<Menu>> menus;
    menus.push_back(std::move(menu));

    AppConfig appConfig;
    appConfig.darkMode = true;
    appConfig.themeOverlayPath = "themes/my-tweaks.qss";
    appConfig.rice.buttonRadiusFraction = 0.28;
    appConfig.rice.innerDeadzoneFraction = 0.35;
    appConfig.rice.startAngleDegrees = 5.0;
    appConfig.rice.mouseOpenOffsetXFraction = -0.08;
    appConfig.rice.mouseOpenOffsetYFraction = 0.12;

    const QString path = dir.filePath("roundtrip.json");
    if (!MenuConfigLoader::saveConfig(path, appConfig, actions, menus))
    {
        std::cerr << "roundtrip: save failed\n";
        return false;
    }

    AppConfig reloadedConfig;
    std::vector<Action> reloadedActions;
    std::vector<std::unique_ptr<Menu>> reloadedMenus;
    if (!MenuConfigLoader::loadConfig(path, reloadedConfig, reloadedActions, reloadedMenus))
    {
        std::cerr << "roundtrip: load failed\n";
        return false;
    }

    if (!reloadedConfig.darkMode || reloadedConfig.themeOverlayPath != appConfig.themeOverlayPath)
    {
        std::cerr << "roundtrip: theme fields not preserved\n";
        return false;
    }
    if (reloadedConfig.rice.buttonRadiusFraction != 0.28
        || reloadedConfig.rice.innerDeadzoneFraction != 0.35
        || reloadedConfig.rice.startAngleDegrees != 5.0
        || reloadedConfig.rice.mouseOpenOffsetXFraction != -0.08
        || reloadedConfig.rice.mouseOpenOffsetYFraction != 0.12)
    {
        std::cerr << "roundtrip: global rice not preserved\n";
        return false;
    }
    if (reloadedMenus.size() != 1 || reloadedMenus.front()->rice() != menuRice)
    {
        std::cerr << "roundtrip: per-menu rice not preserved\n";
        return false;
    }
    return true;
}

bool testOldConfigLoadsWithoutRice()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "legacy: temp dir failed\n";
        return false;
    }

    const QString path = dir.filePath("legacy.json");
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        std::cerr << "legacy: write failed\n";
        return false;
    }
    file.write(R"({
        "darkMode": false,
        "actions": [
            { "id": "action-close", "name": "Close", "items": [ { "type": "close" } ] }
        ],
        "menus": [
            { "id": "menu-main", "name": "Main", "actionIds": [ "action-close" ] }
        ]
    })");
    file.close();

    AppConfig appConfig;
    std::vector<Action> actions;
    std::vector<std::unique_ptr<Menu>> menus;
    if (!MenuConfigLoader::loadConfig(path, appConfig, actions, menus))
    {
        std::cerr << "legacy: load failed\n";
        return false;
    }
    if (!appConfig.themeOverlayPath.isEmpty())
    {
        std::cerr << "legacy: overlay path should default empty\n";
        return false;
    }
    if (appConfig.rice.buttonRadiusFraction != kDefaultButtonRadiusFraction
        || appConfig.rice.innerDeadzoneFraction != 0.0
        || appConfig.rice.mouseOpenOffsetXFraction != 0.0
        || appConfig.rice.mouseOpenOffsetYFraction != kDefaultMouseOpenOffsetYFraction
        || !riceOverridesEmpty(menus.front()->rice()))
    {
        std::cerr << "legacy: rice should stay at defaults\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const std::pair<const char *, bool (*)()> tests[] = {
        {"resolve_inherits", testResolveInheritsWhenEmpty},
        {"resolve_radius_clears_px", testResolveRadiusOverrideClearsGlobalPx},
        {"resolve_deadzone_px", testResolveDeadzonePxOverride},
        {"resolve_mouse_partial", testResolveMouseOffsetPartialOverride},
        {"resolve_clamps_mouse", testResolveClampsMouseOffset},
        {"resolve_clamps_radius", testResolveClampsRadius},
        {"compose_empty_overlay", testComposeEmptyOverlay},
        {"compose_appends_overlay", testComposeAppendsOverlay},
        {"overlay_path_relative", testResolveOverlayPathRelative},
        {"config_rice_roundtrip", testConfigRiceRoundTrip},
        {"old_config_without_rice", testOldConfigLoadsWithoutRice},
    };

    int failed = 0;
    for (const auto &[name, fn] : tests)
    {
        std::cout << "[RUN ] " << name << '\n';
        if (!fn())
        {
            std::cout << "[FAIL] " << name << '\n';
            ++failed;
        }
        else
        {
            std::cout << "[PASS] " << name << '\n';
        }
    }

    if (failed != 0)
    {
        std::cerr << failed << " theme test(s) failed\n";
        return 1;
    }

    std::cout << "Theme tests passed\n";
    return 0;
}

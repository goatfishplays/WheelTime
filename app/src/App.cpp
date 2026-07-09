/**
 * @file App.cpp
 * @author goatfishplays@gmail.com
 * @brief Contains the implementation of the actual app
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "App/App.hpp"
#include "App/MenuConfigLoader.hpp"
#include <Platform/Window.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Execute.hpp>
#include <QApplication>
#include <QPushButton>
#include <QAbstractNativeEventFilter>

using namespace Application;

class HotkeyFilter : public QAbstractNativeEventFilter
{
public:
    HotkeyFilter(App *app) : m_app(app) {}

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        int hotkeyId = 0;
        if (Platform::InputRcvr::isHotkeyMessage(message, hotkeyId))
        {
            m_app->onHotkeyTriggered(hotkeyId);
            return true; // Handled
        }
        return false;
    }

private:
    App *m_app;
};

App::App()
    : activeMenu(nullptr),
      gui(0),
      priorMousePos{},
      priorWindow{},
      inputRcvr(),
      executor(),
      m_hotkeyFilter(nullptr),
      m_configPath(MenuConfigLoader::defaultConfigPath())
{
    // Register global hotkey Alt + Space (mod: 0x0001, vk: 0x20)
    Platform::InputBind bind;
    bind.mod = 0x0001; // MOD_ALT
    bind.input = 0x20; // VK_SPACE
    m_inputRcvr.registerInputBinding(bind);

    // Install native event filter to capture WM_HOTKEY
    m_hotkeyFilter = new HotkeyFilter(this);
    qApp->installNativeEventFilter(m_hotkeyFilter);

    // Connect escapePressed signal from Gui to hide and return focus
    QObject::connect(&gui, &Gui::escapePressed, [this]()
                     { hideGui(); });

    if (!MenuConfigLoader::loadMenus(m_configPath, loadedMenus))
    {
        std::vector<Action> fallbackActions;
        fallbackActions.push_back(Action({}, "Config missing"));
        loadedMenus.push_back(new Menu(nullptr, false, false, "Config Error", fallbackActions));
    }

    // gui.show();
    showGui(loadedMenus[0]);
}

App::~App()
{
    if (m_hotkeyFilter)
    {
        qApp->removeNativeEventFilter(m_hotkeyFilter);
        delete m_hotkeyFilter;
        m_hotkeyFilter = nullptr;
    }

    int numMenus = loadedMenus.size();
    for (int i = numMenus - 1; i >= 0; i--)
    {
        delete loadedMenus[i];
    }

    // Unregister hotkey Alt + Space (mod: 0x0001, vk: 0x20)
    Platform::InputBind bind;
    bind.mod = 0x0001;
    bind.input = 0x20;
    m_inputRcvr.unregisterInputBinding(bind);
}

void App::onHotkeyTriggered(int hotkeyId)
{
    if (gui.isVisible() && gui.isActiveWindow())
    {
        hideGui();
    }
    else
    {
        showGui(activeMenu); // TODO: Replace this with the menu associated with the pressed hotkey
    }
}

Menu *App::getActiveMenu() { return activeMenu; }

void App::gatherPriors()
{
    // Do not gather priors if we are just swapping to a submenu
    if (gui.isVisible())
    {
        return;
    }
    priorMousePos = inputRcvr.getAbsoluteMousePosition();
    priorWindow.getActiveWindow();
}

void App::restorePriors()
{
    executor.setMousePos(priorMousePos.x, priorMousePos.y);
    priorWindow.focus();
}

void App::showGui(Menu *menu)
{
    if (menu == nullptr)
    {
        return;
    }

    gatherPriors();
    activeMenu = menu;
    gui.setMenu(*activeMenu);

    // TODO: test make sure this is fine to do
    gui.showNormal();
    gui.raise();
    gui.activateWindow();

    Platform::Window appWindow(reinterpret_cast<void *>(gui.winId()));
    appWindow.focus();
}

void App::hideGui()
{
    gui.hide();
    restorePriors();
}

void App::executeAction(int actionInd)
{
    if (activeMenu != nullptr && actionInd >= 0 && actionInd < static_cast<int>(activeMenu->actions.size()))
    {
        activeMenu->actions[actionInd].execute();
    }
}

bool App::saveMenus()
{
    return MenuConfigLoader::saveMenus(m_configPath, loadedMenus);
}

void App::refreshActiveMenu()
{
    if (activeMenu != nullptr)
    {
        gui.setMenu(*activeMenu);
    }
}

QString App::getConfigPath() const
{
    return m_configPath;
}

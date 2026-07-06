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
#include <Platform/Window.hpp>
#include <QApplication>
#include <QPushButton>
#include <QAbstractNativeEventFilter>

using namespace Application;

class HotkeyFilter : public QAbstractNativeEventFilter
{
public:
    HotkeyFilter(App* app) : m_app(app) {}
    
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        int hotkeyId = 0;
        if (Platform::InputRcvr::isHotkeyMessage(message, hotkeyId)) {
            m_app->onHotkeyTriggered(hotkeyId);
            return true; // Handled
        }
        return false;
    }

private:
    App* m_app;
};

// TODO: Mbe just pass QApp in
App::App(int &argc, char **argv)
    : activeMenu(nullptr),
      priorMousePos{},
      priorWindow{},
      gui(0),
      m_hotkeyFilter(nullptr)
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
    QObject::connect(&gui, &Gui::escapePressed, [this]() {
        gui.hide();
        priorWindow.focus();
    });

    gui.show();
}

App::~App()
{
    if (m_hotkeyFilter) {
        qApp->removeNativeEventFilter(m_hotkeyFilter);
        delete m_hotkeyFilter;
        m_hotkeyFilter = nullptr;
    }

    // Unregister hotkey Alt + Space (mod: 0x0001, vk: 0x20)
    Platform::InputBind bind;
    bind.mod = 0x0001;
    bind.input = 0x20;
    m_inputRcvr.unregisterInputBinding(bind);
}

void App::onHotkeyTriggered(int hotkeyId)
{
    if (gui.isVisible() && gui.isActiveWindow()) {
        gui.hide();
        priorWindow.focus();
    } else {
        priorWindow.getActiveWindow();
        
        gui.showNormal();
        gui.raise();
        gui.activateWindow();

        Platform::Window appWindow(reinterpret_cast<void*>(gui.winId()));
        appWindow.focus();
    }
}

Menu *App::getActiveMenu() { return activeMenu; }

void App::setActiveMenu(Menu &menu)
{
}

void App::exitMenu()
{
}

void App::runAction(Action &action) {}
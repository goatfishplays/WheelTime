#include "App/MenuConfigLoader.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>
#include <QDebug>

#include <algorithm>
#include <memory>

#include "App/Action.hpp"
#include "App/ActionItems.hpp"

using namespace Application;

namespace
{
    QStringList defaultConfigCandidates()
    {
        const QString appDir = QCoreApplication::applicationDirPath();
        return {
            QDir::current().filePath("config/default_menu.json"),
            QDir::current().filePath("../config/default_menu.json"),
            QDir::current().filePath("../../config/default_menu.json"),
            QDir(appDir).filePath("config/default_menu.json"),
            QDir(appDir).filePath("../config/default_menu.json"),
            QDir(appDir).filePath("../../config/default_menu.json")};
    }

    QString slugify(const QString &seed, const QString &prefix)
    {
        QString slug = seed.trimmed().toLower();
        for (int i = 0; i < slug.size(); ++i)
        {
            if (!slug[i].isLetterOrNumber())
            {
                slug[i] = '-';
            }
        }
        while (slug.contains("--"))
        {
            slug.replace("--", "-");
        }
        slug.remove(QRegularExpression("^-+"));
        slug.remove(QRegularExpression("-+$"));
        if (slug.isEmpty())
        {
            slug = prefix;
        }
        return prefix + "-" + slug;
    }

    std::string makeUniqueId(const QString &seed, const QString &prefix, std::vector<std::string> &existingIds)
    {
        QString candidate = slugify(seed, prefix);
        QString uniqueCandidate = candidate;
        int suffix = 2;
        while (std::find(existingIds.begin(), existingIds.end(), uniqueCandidate.toStdString()) != existingIds.end())
        {
            uniqueCandidate = QString("%1-%2").arg(candidate).arg(suffix++);
        }
        existingIds.push_back(uniqueCandidate.toStdString());
        return uniqueCandidate.toStdString();
    }

    bool readJsonFile(const QString &filepath, QJsonDocument &document)
    {
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Could not open menu config:" << filepath;
            return false;
        }

        QJsonParseError parseError;
        document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            qWarning() << "Invalid menu config JSON:" << filepath << parseError.errorString();
            return false;
        }

        return true;
    }

    std::vector<std::unique_ptr<ActionItem>> parseItems(const QJsonArray &itemArray)
    {
        std::vector<std::unique_ptr<ActionItem>> items;

        // Item parsing is the main schema extension point. Adding editor/runtime
        // support for a new action item should generally only require a new
        // branch here plus matching save/summary/editor logic elsewhere.
        for (const QJsonValue &itemValue : itemArray)
        {
            const QJsonObject itemObject = itemValue.toObject();
            const QString type = itemObject.value("type").toString();

            if (type == "script")
            {
                items.push_back(std::make_unique<AI_Script>(itemObject.value("path").toString().toStdString()));
            }
            else if (type == "launch_app")
            {
                items.push_back(std::make_unique<AI_LaunchApp>(
                    itemObject.value("presetId").toString("custom").toStdString(),
                    itemObject.value("customTarget").toString().toStdString()));
            }
            else if (type == "delay")
            {
                items.push_back(std::make_unique<AI_Delay>(itemObject.value("duration").toInt(0)));
            }
            else if (type == "hotkey" || type == "keystroke")
            {
                items.push_back(std::make_unique<AI_Keystroke>(
                    itemObject.value("keycode").toInt(0),
                    itemObject.value("modifiers").toInt(0),
                    static_cast<float>(itemObject.value("holdDuration").toDouble(0.0)),
                    itemObject.value("proceed").toBool(false)));
            }
            else if (type == "close")
            {
                items.push_back(std::make_unique<AI_Close>());
            }
            else if (type == "menu")
            {
                items.push_back(std::make_unique<AI_Menu>(itemObject.value("menuId").toString().toStdString()));
            }
            else if (type == "mouse_move")
            {
                items.push_back(std::make_unique<AI_MouseMove>(
                    itemObject.value("x").toInt(0),
                    itemObject.value("y").toInt(0)));
            }
            else if (type == "mouse_button")
            {
                // Legacy: down=false was an explicit release; map to MouseButtonRelease.
                if (itemObject.contains("down") && !itemObject.value("down").toBool(true))
                {
                    items.push_back(std::make_unique<AI_MouseButtonRelease>(
                        itemObject.value("button").toInt(0),
                        itemObject.value("modifiers").toInt(0)));
                }
                else
                {
                    items.push_back(std::make_unique<AI_MouseButton>(
                        itemObject.value("button").toInt(0),
                        static_cast<float>(itemObject.value("holdDuration").toDouble(0.0)),
                        itemObject.value("proceed").toBool(false),
                        itemObject.value("modifiers").toInt(0)));
                }
            }
            else if (type == "mouse_button_release")
            {
                items.push_back(std::make_unique<AI_MouseButtonRelease>(
                    itemObject.value("button").toInt(0),
                    itemObject.value("modifiers").toInt(0)));
            }
            else if (type == "key_release")
            {
                items.push_back(std::make_unique<AI_KeyRelease>(
                    itemObject.value("keycode").toInt(0),
                    itemObject.value("modifiers").toInt(0)));
            }
            else if (type == "cancel")
            {
                const QString level = itemObject.value("level").toString("latest");
                CancelLevel cancelLevel = CancelLevel::MostRecent;
                if (level == "channel")
                {
                    cancelLevel = CancelLevel::Channel;
                }
                else if (level == "all")
                {
                    cancelLevel = CancelLevel::All;
                }
                else
                {
                    // Wire format: "latest" (canonical). Also accept "most_recent" / legacy "action".
                    cancelLevel = CancelLevel::MostRecent;
                }
                items.push_back(std::make_unique<AI_Cancel>(
                    cancelLevel,
                    static_cast<uint32_t>(itemObject.value("channel").toInt(0))));
            }
            else if (type == "nth_recent")
            {
                items.push_back(std::make_unique<AI_NthRecent>(itemObject.value("n").toInt(1)));
            }
            else if (type == "nth_frequent")
            {
                items.push_back(std::make_unique<AI_NthFrequent>(itemObject.value("n").toInt(1)));
            }
            else if (type == "socket")
            {
                const QString protocolText = itemObject.value("protocol").toString("udp").trimmed().toLower();
                Platform::SocketProtocol protocol = Platform::SocketProtocol::Udp;
                if (protocolText == "tcp")
                {
                    protocol = Platform::SocketProtocol::Tcp;
                }
                else if (protocolText == "http")
                {
                    protocol = Platform::SocketProtocol::Http;
                }
                else if (protocolText == "websocket" || protocolText == "ws")
                {
                    protocol = Platform::SocketProtocol::WebSocket;
                }
                else if (protocolText != "udp" && !protocolText.isEmpty())
                {
                    qWarning() << "Unknown socket protocol '" << protocolText
                               << "'; defaulting to udp";
                }

                const QString destination = itemObject.contains("destination")
                                                ? itemObject.value("destination").toString()
                                                : itemObject.value("outputDst").toString();
                const QString message = itemObject.contains("message")
                                            ? itemObject.value("message").toString()
                                            : itemObject.value("socketMsg").toString();
                items.push_back(std::make_unique<AI_Socket>(
                    protocol,
                    destination.toStdString(),
                    message.toStdString(),
                    itemObject.value("httpMethod").toString("POST").toStdString()));
            }
            else
            {
                qWarning() << "Unsupported action item type:" << type;
            }
        }

        return items;
    }

    Action parseLibraryAction(const QJsonObject &actionObject)
    {
        const std::string id = actionObject.value("id").toString().toStdString();
        const std::string name = actionObject.value("name").toString("Unnamed Action").toStdString();
        const std::string icon = actionObject.value("icon").toString().toStdString();
        const uint32_t channel = static_cast<uint32_t>(actionObject.value("channel").toInt(0));
        return Action(parseItems(actionObject.value("items").toArray()), name, icon, id, channel);
    }

    bool loadNewSchema(const QJsonObject &rootObject, std::vector<Action> &actions, std::vector<Menu *> &menus)
    {
        const QJsonArray actionsArray = rootObject.value("actions").toArray();
        const QJsonArray menusArray = rootObject.value("menus").toArray();
        if (menusArray.isEmpty())
        {
            qWarning() << "Config contains no menus.";
            return false;
        }

        for (const QJsonValue &actionValue : actionsArray)
        {
            actions.push_back(parseLibraryAction(actionValue.toObject()));
        }

        for (const QJsonValue &menuValue : menusArray)
        {
            const QJsonObject menuObject = menuValue.toObject();
            std::vector<std::string> actionIds;
            for (const QJsonValue &idValue : menuObject.value("actionIds").toArray())
            {
                actionIds.push_back(idValue.toString().toStdString());
            }

            menus.push_back(new Menu(
                nullptr,
                menuObject.value("executeOnRelease").toBool(false),
                menuObject.value("exitOnAction").toBool(false),
                menuObject.value("name").toString("Unnamed Menu").toStdString(),
                actionIds,
                menuObject.value("id").toString().toStdString()));
        }

        return !menus.empty();
    }

    bool loadLegacySchema(const QJsonObject &rootObject, std::vector<Action> &actions, std::vector<Menu *> &menus)
    {
        const QJsonArray menusArray = rootObject.value("menus").toArray();
        if (menusArray.isEmpty())
        {
            return false;
        }

        // Older config files stored full action definitions inside each menu.
        // We lift those actions into the shared library on load so the rest of
        // the app can operate entirely on the newer reusable-action model.
        std::vector<std::string> actionIds;
        std::vector<std::string> menuIds;

        for (const QJsonValue &menuValue : menusArray)
        {
            const QJsonObject menuObject = menuValue.toObject();
            const std::string menuId = makeUniqueId(menuObject.value("name").toString("Menu"), "menu", menuIds);

            std::vector<std::string> menuActionIds;
            for (const QJsonValue &actionValue : menuObject.value("actions").toArray())
            {
                const QJsonObject actionObject = actionValue.toObject();
                const QString name = actionObject.value("name").toString("Unnamed Action");
                const QString type = actionObject.value("type").toString();
                const std::string actionId = makeUniqueId(QString("%1-%2").arg(QString::fromStdString(menuId), name), "action", actionIds);

                std::vector<std::unique_ptr<ActionItem>> items;
                if (type == "script")
                {
                    items.push_back(std::make_unique<AI_LaunchApp>("custom", actionObject.value("path").toString().toStdString()));
                }

                actions.push_back(Action(std::move(items), name.toStdString(), "", actionId));
                menuActionIds.push_back(actionId);
            }

            menus.push_back(new Menu(
                nullptr,
                menuObject.value("executeOnRelease").toBool(false),
                menuObject.value("exitOnAction").toBool(false),
                menuObject.value("name").toString("Unnamed Menu").toStdString(),
                menuActionIds,
                menuId));
        }

        return !menus.empty();
    }

    QJsonObject serializeItem(const ActionItem &item)
    {
        QJsonObject itemObject;
        switch (item.kind())
        {
        case ActionItemKind::LaunchApp:
            itemObject.insert("type", "launch_app");
            itemObject.insert("presetId", QString::fromStdString(static_cast<const AI_LaunchApp &>(item).presetId));
            itemObject.insert("customTarget", QString::fromStdString(static_cast<const AI_LaunchApp &>(item).customTarget));
            break;
        case ActionItemKind::Script:
            itemObject.insert("type", "script");
            itemObject.insert("path", QString::fromStdString(static_cast<const AI_Script &>(item).filepath));
            break;
        case ActionItemKind::Delay:
            itemObject.insert("type", "delay");
            itemObject.insert("duration", static_cast<const AI_Delay &>(item).duration);
            break;
        case ActionItemKind::Keystroke:
            itemObject.insert("type", "hotkey");
            itemObject.insert("keycode", static_cast<const AI_Keystroke &>(item).keycode);
            itemObject.insert("modifiers", static_cast<const AI_Keystroke &>(item).modifiers);
            itemObject.insert("holdDuration", static_cast<const AI_Keystroke &>(item).holdDuration);
            itemObject.insert("proceed", static_cast<const AI_Keystroke &>(item).proceed);
            break;
        case ActionItemKind::Close:
            itemObject.insert("type", "close");
            break;
        case ActionItemKind::Menu:
            itemObject.insert("type", "menu");
            itemObject.insert("menuId", QString::fromStdString(static_cast<const AI_Menu &>(item).menuId));
            break;
        case ActionItemKind::MouseMove:
            itemObject.insert("type", "mouse_move");
            itemObject.insert("x", static_cast<const AI_MouseMove &>(item).x);
            itemObject.insert("y", static_cast<const AI_MouseMove &>(item).y);
            break;
        case ActionItemKind::MouseButton:
            itemObject.insert("type", "mouse_button");
            itemObject.insert("button", static_cast<const AI_MouseButton &>(item).button);
            itemObject.insert("modifiers", static_cast<const AI_MouseButton &>(item).modifiers);
            itemObject.insert("holdDuration", static_cast<const AI_MouseButton &>(item).holdDuration);
            itemObject.insert("proceed", static_cast<const AI_MouseButton &>(item).proceed);
            break;
        case ActionItemKind::MouseButtonRelease:
            itemObject.insert("type", "mouse_button_release");
            itemObject.insert("button", static_cast<const AI_MouseButtonRelease &>(item).button);
            itemObject.insert("modifiers", static_cast<const AI_MouseButtonRelease &>(item).modifiers);
            break;
        case ActionItemKind::KeyRelease:
            itemObject.insert("type", "key_release");
            itemObject.insert("keycode", static_cast<const AI_KeyRelease &>(item).keycode);
            itemObject.insert("modifiers", static_cast<const AI_KeyRelease &>(item).modifiers);
            break;
        case ActionItemKind::Cancel:
        {
            const auto &cancel = static_cast<const AI_Cancel &>(item);
            itemObject.insert("type", "cancel");
            switch (cancel.level)
            {
            case CancelLevel::Channel:
                itemObject.insert("level", "channel");
                itemObject.insert("channel", static_cast<int>(cancel.channel));
                break;
            case CancelLevel::All:
                itemObject.insert("level", "all");
                break;
            case CancelLevel::MostRecent:
            default:
                itemObject.insert("level", "latest");
                itemObject.insert("channel", static_cast<int>(cancel.channel));
                break;
            }
            break;
        }
        case ActionItemKind::NthRecent:
            itemObject.insert("type", "nth_recent");
            itemObject.insert("n", static_cast<const AI_NthRecent &>(item).n);
            break;
        case ActionItemKind::NthFrequent:
            itemObject.insert("type", "nth_frequent");
            itemObject.insert("n", static_cast<const AI_NthFrequent &>(item).n);
            break;
        case ActionItemKind::Socket:
        {
            const auto &socket = static_cast<const AI_Socket &>(item);
            itemObject.insert("type", "socket");
            switch (socket.protocol)
            {
            case Platform::SocketProtocol::Tcp:
                itemObject.insert("protocol", "tcp");
                break;
            case Platform::SocketProtocol::Http:
                itemObject.insert("protocol", "http");
                break;
            case Platform::SocketProtocol::WebSocket:
                itemObject.insert("protocol", "websocket");
                break;
            case Platform::SocketProtocol::Udp:
            default:
                itemObject.insert("protocol", "udp");
                break;
            }
            itemObject.insert("destination", QString::fromStdString(socket.outputDst));
            itemObject.insert("message", QString::fromStdString(socket.socketMsg));
            if (socket.protocol == Platform::SocketProtocol::Http)
            {
                itemObject.insert("httpMethod", QString::fromStdString(socket.httpMethod));
            }
            break;
        }
        default:
            itemObject.insert("type", "unsupported");
            break;
        }
        return itemObject;
    }
}

QString MenuConfigLoader::defaultConfigPath()
{
    for (const QString &candidate : defaultConfigCandidates())
    {
        if (QFile::exists(candidate))
        {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }

    return QFileInfo(QDir::current().filePath("config/default_menu.json")).absoluteFilePath();
}

bool MenuConfigLoader::loadConfig(const QString &filepath, AppConfig &appConfig, std::vector<Action> &actions, std::vector<Menu *> &menus)
{
    QJsonDocument document;
    if (!readJsonFile(filepath, document))
    {
        return false;
    }

    const QJsonObject rootObject = document.object();
    actions.clear();

    if (rootObject.contains("globalHotkey"))
    {
        QJsonObject hk = rootObject.value("globalHotkey").toObject();
        appConfig.globalHotkeyMod = hk.value("mod").toInt(0x0001);
        appConfig.globalHotkeyVk = hk.value("vk").toInt(0x20);
    }

    // Newer configs include a top-level action library. If that section is
    // absent, treat the file as the older menu-owned schema for compatibility.
    if (rootObject.contains("actions"))
    {
        return loadNewSchema(rootObject, actions, menus);
    }

    return loadLegacySchema(rootObject, actions, menus);
}

bool MenuConfigLoader::saveConfig(const QString &filepath, const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu *> &menus)
{
    QJsonArray actionsArray;
    for (const Action &action : actions)
    {
        QJsonObject actionObject;
        actionObject.insert("id", QString::fromStdString(action.getId()));
        actionObject.insert("name", QString::fromStdString(action.getName()));
        if (!action.getIconFilepath().empty())
        {
            actionObject.insert("icon", QString::fromStdString(action.getIconFilepath()));
        }
        actionObject.insert("channel", static_cast<int>(action.channel()));

        QJsonArray itemsArray;
        for (const auto &itemPtr : action.getItems())
        {
            if (itemPtr)
            {
                itemsArray.append(serializeItem(*itemPtr));
            }
        }
        actionObject.insert("items", itemsArray);
        actionsArray.append(actionObject);
    }

    QJsonArray menusArray;
    for (const Menu *menu : menus)
    {
        if (menu == nullptr)
        {
            continue;
        }

        QJsonObject menuObject;
        menuObject.insert("id", QString::fromStdString(menu->getId()));
        menuObject.insert("name", QString::fromStdString(menu->getName()));
        menuObject.insert("executeOnRelease", menu->executeOnRelease);
        menuObject.insert("exitOnAction", menu->exitOnAction);

        QJsonArray actionIdsArray;
        for (const std::string &actionId : menu->getActionIds())
        {
            actionIdsArray.append(QString::fromStdString(actionId));
        }
        menuObject.insert("actionIds", actionIdsArray);
        menusArray.append(menuObject);
    }

    QJsonObject rootObject;
    rootObject.insert("actions", actionsArray);
    rootObject.insert("menus", menusArray);

    QJsonObject hkObject;
    hkObject.insert("mod", appConfig.globalHotkeyMod);
    hkObject.insert("vk", appConfig.globalHotkeyVk);
    rootObject.insert("globalHotkey", hkObject);

    QFileInfo fileInfo(filepath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "Could not write menu config:" << filepath;
        return false;
    }

    file.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
    return true;
}

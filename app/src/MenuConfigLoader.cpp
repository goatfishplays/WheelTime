#include "App/MenuConfigLoader.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QDebug>

#include <algorithm>
#include <memory>

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
            else if (type == "hotkey")
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
        return Action(parseItems(actionObject.value("items").toArray()), name, "", id);
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

bool MenuConfigLoader::loadConfig(const QString &filepath, std::vector<Action> &actions, std::vector<Menu *> &menus)
{
    QJsonDocument document;
    if (!readJsonFile(filepath, document))
    {
        return false;
    }

    const QJsonObject rootObject = document.object();
    actions.clear();

    if (rootObject.contains("actions"))
    {
        return loadNewSchema(rootObject, actions, menus);
    }

    return loadLegacySchema(rootObject, actions, menus);
}

bool MenuConfigLoader::saveConfig(const QString &filepath, const std::vector<Action> &actions, const std::vector<Menu *> &menus)
{
    QJsonArray actionsArray;
    for (const Action &action : actions)
    {
        QJsonObject actionObject;
        actionObject.insert("id", QString::fromStdString(action.getId()));
        actionObject.insert("name", QString::fromStdString(action.getName()));

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

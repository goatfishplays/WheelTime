#include "App/MenuConfigLoader.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QDebug>

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

    Action parseAction(const QJsonObject &actionObject)
    {
        const std::string name = actionObject.value("name").toString("Unnamed Action").toStdString();
        const QString type = actionObject.value("type").toString();

        std::vector<std::unique_ptr<ActionItem>> sequence;
        if (type == "script")
        {
            const QString path = actionObject.value("path").toString();
            if (!path.isEmpty())
            {
                sequence.push_back(std::make_unique<AI_Script>(path.toStdString()));
            }
            else
            {
                qWarning() << "Script action is missing path:" << QString::fromStdString(name);
            }
        }
        else
        {
            qWarning() << "Unsupported action type:" << type << "for action:" << QString::fromStdString(name);
        }

        return Action(std::move(sequence), name);
    }

    QJsonObject serializeAction(const Action &action)
    {
        QJsonObject actionObject;
        actionObject.insert("name", QString::fromStdString(action.getName()));

        if (action.isScriptAction())
        {
            actionObject.insert("type", "script");
            actionObject.insert("path", QString::fromStdString(action.getScriptPath()));
        }

        return actionObject;
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

bool MenuConfigLoader::loadMenus(const QString &filepath, std::vector<Menu *> &menus)
{
    QJsonDocument document;
    if (!readJsonFile(filepath, document))
    {
        return false;
    }

    const QJsonArray menuArray = document.object().value("menus").toArray();
    if (menuArray.isEmpty())
    {
        qWarning() << "Menu config has no menus:" << filepath;
        return false;
    }

    for (const QJsonValue &menuValue : menuArray)
    {
        const QJsonObject menuObject = menuValue.toObject();
        const std::string name = menuObject.value("name").toString("Unnamed Menu").toStdString();
        const bool executeOnRelease = menuObject.value("executeOnRelease").toBool(false);
        const bool exitOnAction = menuObject.value("exitOnAction").toBool(false);

        std::vector<Action> actions;
        const QJsonArray actionArray = menuObject.value("actions").toArray();
        for (const QJsonValue &actionValue : actionArray)
        {
            actions.push_back(parseAction(actionValue.toObject()));
        }

        menus.push_back(new Menu(nullptr, executeOnRelease, exitOnAction, name, actions));
    }

    return !menus.empty();
}

bool MenuConfigLoader::loadDefaultMenus(std::vector<Menu *> &menus)
{
    const QString filepath = defaultConfigPath();
    const bool loaded = loadMenus(filepath, menus);
    if (!loaded)
    {
        qWarning() << "Failed to load config/default_menu.json from known locations";
    }
    return loaded;
}

bool MenuConfigLoader::saveMenus(const QString &filepath, const std::vector<Menu *> &menus)
{
    QJsonArray menuArray;
    for (const Menu *menu : menus)
    {
        if (menu == nullptr)
        {
            continue;
        }

        QJsonObject menuObject;
        menuObject.insert("name", QString::fromStdString(menu->getName()));
        menuObject.insert("executeOnRelease", menu->executeOnRelease);
        menuObject.insert("exitOnAction", menu->exitOnAction);

        QJsonArray actionArray;
        for (const Action &action : menu->actions)
        {
            actionArray.append(serializeAction(action));
        }

        menuObject.insert("actions", actionArray);
        menuArray.append(menuObject);
    }

    QJsonObject rootObject;
    rootObject.insert("menus", menuArray);

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

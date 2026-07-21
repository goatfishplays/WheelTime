/**
 * @file action_history_smoke.cpp
 * @brief Smoke / integration tests for ActionHistory and nth ActionItems.
 *
 * Pure ActionHistory tests are isolated. App integration tests install their own
 * fixture actions/menu slots so they do not depend on the user-editable
 * default_menu.json contents.
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionHistory.hpp"
#include "App/ActionItems.hpp"
#include "App/App.hpp"
#include "App/ExecuteResult.hpp"
#include "App/MenuConfigLoader.hpp"
#include "App/Scheduler.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace Application;

namespace
{

constexpr const char *kSmokeAlpha = "action-smoke-alpha";
constexpr const char *kSmokeBeta = "action-smoke-beta";
constexpr const char *kSmokeGamma = "action-smoke-gamma";
constexpr const char *kSmokeNthRecent = "action-smoke-nth-recent";
constexpr const char *kSmokeNthFrequent = "action-smoke-nth-frequent";
constexpr const char *kSmokeCancel = "action-smoke-cancel";

QString historyPathBesideConfig()
{
    return QFileInfo(MenuConfigLoader::defaultConfigPath()).dir().filePath("action_history.json");
}

bool writeHistoryFile(const QString &path, const QJsonArray &recent, const QJsonObject &frequent)
{
    QJsonObject root;
    root.insert("recent", recent);
    root.insert("frequent", frequent);
    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

Action makeDelayAction(const char *id, const char *name)
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_Delay>(1));
    return Action(std::move(items), name, "", id, 0);
}

void upsertAction(App &app, Action action)
{
    for (Action &existing : app.actionLibrary)
    {
        if (existing.getId() == action.getId())
        {
            existing = std::move(action);
            return;
        }
    }
    app.actionLibrary.push_back(std::move(action));
}

/// @brief Installs deterministic library + menu slots without saving user config.
bool installSmokeFixtures(App &app)
{
    upsertAction(app, makeDelayAction(kSmokeAlpha, "Smoke Alpha"));
    upsertAction(app, makeDelayAction(kSmokeBeta, "Smoke Beta"));
    upsertAction(app, makeDelayAction(kSmokeGamma, "Smoke Gamma"));

    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_NthRecent>(1));
        upsertAction(app, Action(std::move(items), "Smoke Nth Recent", "", kSmokeNthRecent, 0));
    }
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_NthFrequent>(1));
        upsertAction(app, Action(std::move(items), "Smoke Nth Frequent", "", kSmokeNthFrequent, 0));
    }
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Cancel>(CancelLevel::MostRecent, 0));
        upsertAction(app, Action(std::move(items), "Smoke Cancel", "", kSmokeCancel, 0));
    }

    if (app.loadedMenus.empty())
    {
        app.loadedMenus.push_back(
            new Menu(0, 0, false, false, true, false, "Smoke Menu", {}, "menu-smoke"));
    }

    Menu *menu = app.loadedMenus.front();
    if (menu == nullptr)
    {
        std::cerr << "smoke fixtures: front menu null\n";
        return false;
    }

    while (menu->actionCount() > 0)
    {
        menu->removeAction(0);
    }

    // Slots: 0 alpha, 1 beta, 2 gamma, 3 cancel, 4 nth-recent, 5 nth-frequent
    menu->addActionId(-1, kSmokeAlpha);
    menu->addActionId(-1, kSmokeBeta);
    menu->addActionId(-1, kSmokeGamma);
    menu->addActionId(-1, kSmokeCancel);
    menu->addActionId(-1, kSmokeNthRecent);
    menu->addActionId(-1, kSmokeNthFrequent);

    if (app.getActiveMenu() != menu)
    {
        // showGui sets activeMenu; keep the overlay dormant via hide afterward.
        app.showGui(menu);
        app.hideGui();
    }

    if (app.findActionById(kSmokeAlpha) == nullptr || app.getActiveMenu() == nullptr
        || app.getActiveMenu()->actionCount() < 6)
    {
        std::cerr << "smoke fixtures: install incomplete\n";
        return false;
    }

    return true;
}

void drainScheduler(App &app)
{
    app.scheduler().cancelAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

bool testRecentAndFrequentRanking()
{
    ActionHistory history{8};
    history.recordUse("a");
    history.recordUse("b");
    history.recordUse("a");
    history.recordUse("c");
    history.recordUse("a");

    // Recent: a, c, b
    if (history.nthRecentId(1) != "a" || history.nthRecentId(2) != "c" || history.nthRecentId(3) != "b")
    {
        std::cerr << "recent ranking wrong\n";
        return false;
    }
    if (!history.nthRecentId(0).empty() || !history.nthRecentId(4).empty())
    {
        std::cerr << "recent bounds wrong\n";
        return false;
    }

    // Frequent: a(3), then c(1)/b(1) with c more recent than b
    if (history.nthFrequentId(1) != "a")
    {
        std::cerr << "frequent #1 wrong\n";
        return false;
    }
    if (history.nthFrequentId(2) != "c" || history.nthFrequentId(3) != "b")
    {
        std::cerr << "frequent tie-break wrong\n";
        return false;
    }
    return true;
}

bool testPersistRoundTrip()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "temp dir failed\n";
        return false;
    }

    const QString path = dir.filePath("action_history.json");
    {
        ActionHistory history;
        history.recordUse("alpha");
        history.recordUse("beta");
        history.recordUse("alpha");
        if (!history.save(path))
        {
            std::cerr << "save failed\n";
            return false;
        }
    }

    ActionHistory loaded;
    if (!loaded.load(path))
    {
        std::cerr << "load failed\n";
        return false;
    }
    if (loaded.nthRecentId(1) != "alpha" || loaded.nthRecentId(2) != "beta")
    {
        std::cerr << "loaded recent wrong\n";
        return false;
    }
    if (loaded.nthFrequentId(1) != "alpha")
    {
        std::cerr << "loaded frequent wrong\n";
        return false;
    }
    return true;
}

bool testPrune()
{
    ActionHistory history;
    history.recordUse("keep");
    history.recordUse("drop");
    history.pruneToLibrary({"keep"});
    if (history.nthRecentId(1) != "keep" || history.recentCount() != 1)
    {
        std::cerr << "prune recent failed\n";
        return false;
    }
    if (history.nthFrequentId(1) != "keep" || history.frequentEntryCount() != 1)
    {
        std::cerr << "prune frequent failed\n";
        return false;
    }
    return true;
}

bool testMaxRecentCapAndEmptyIgnored()
{
    ActionHistory history{2};
    history.recordUse("");
    history.recordUse("a");
    history.recordUse("b");
    history.recordUse("c");
    if (history.recentCount() != 2 || history.nthRecentId(1) != "c" || history.nthRecentId(2) != "b")
    {
        std::cerr << "maxRecent cap failed\n";
        return false;
    }
    // Counts for a should still exist even if pruned from MRU list.
    if (history.nthFrequentId(1).empty())
    {
        std::cerr << "frequent should still track capped ids\n";
        return false;
    }
    return true;
}

bool testAppLookupAndActionItemSchedule()
{
    App &app = App::getInstance();
    if (!installSmokeFixtures(app))
    {
        return false;
    }

    // Seed history the same way wheel launches do: executeAction records use.
    // Menu order: alpha(0), beta(1), gamma(2), ...
    app.executeAction(1); // beta
    app.executeAction(2); // gamma
    app.executeAction(0); // alpha (most recent)
    app.executeAction(0); // alpha again (most frequent)
    drainScheduler(app);

    Action *r1 = app.nthRecentAction(1);
    Action *r2 = app.nthRecentAction(2);
    Action *r3 = app.nthRecentAction(3);
    if (r1 == nullptr || r2 == nullptr || r3 == nullptr)
    {
        std::cerr << "app_lookup: recent pointers null\n";
        return false;
    }
    if (r1->getId() != kSmokeAlpha || r2->getId() != kSmokeGamma || r3->getId() != kSmokeBeta)
    {
        std::cerr << "app_lookup: recent order wrong: " << r1->getId() << ", " << r2->getId()
                  << ", " << r3->getId() << '\n';
        return false;
    }

    Action *f1 = app.nthFrequentAction(1);
    if (f1 == nullptr || f1->getId() != kSmokeAlpha)
    {
        std::cerr << "app_lookup: frequent #1 should be alpha\n";
        return false;
    }

    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "host", "", "host", 0)};

    AI_NthRecent recentItem{1};
    if (recentItem.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "nth_recent_item: expected Continue\n";
        return false;
    }
    {
        auto scheduled = ctx.takeScheduledActions();
        if (scheduled.size() != 1 || !scheduled[0].action
            || scheduled[0].action->getId() != kSmokeAlpha)
        {
            std::cerr << "nth_recent_item: did not schedule alpha copy\n";
            return false;
        }
    }

    AI_NthFrequent frequentItem{1};
    if (frequentItem.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "nth_frequent_item: expected Continue\n";
        return false;
    }
    {
        auto scheduled = ctx.takeScheduledActions();
        if (scheduled.size() != 1 || !scheduled[0].action
            || scheduled[0].action->getId() != kSmokeAlpha)
        {
            std::cerr << "nth_frequent_item: did not schedule alpha copy\n";
            return false;
        }
    }

    AI_NthRecent miss{99};
    if (miss.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        return false;
    }
    if (!ctx.takeScheduledActions().empty())
    {
        std::cerr << "nth_recent_item: miss should not schedule\n";
        return false;
    }

    AI_NthRecent bad{0};
    if (bad.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        return false;
    }
    if (!ctx.takeScheduledActions().empty())
    {
        std::cerr << "nth_recent_item: n=0 should not schedule\n";
        return false;
    }

    return true;
}

bool testSchedulerRunsScheduledNthTarget()
{
    App &app = App::getInstance();
    if (!installSmokeFixtures(app))
    {
        return false;
    }

    app.executeAction(0); // ensure a recent non-meta Action exists
    drainScheduler(app);

    if (app.nthRecentAction(1) == nullptr)
    {
        std::cerr << "scheduler_nth: failed to seed recent history\n";
        return false;
    }

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_NthRecent>(1));
    auto host = std::make_unique<Action>(std::move(items), "nth-host", "", "nth-host", 0);

    Scheduler &scheduler = app.scheduler();
    scheduler.submit(std::move(host));
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    drainScheduler(app);
    return true;
}

bool testHistoryMetaActionsDoNotLoop()
{
    App &app = App::getInstance();
    if (!installSmokeFixtures(app))
    {
        return false;
    }

    const Action *recentHelper = app.findActionById(kSmokeNthRecent);
    const Action *frequentHelper = app.findActionById(kSmokeNthFrequent);
    const Action *cancelHelper = app.findActionById(kSmokeCancel);
    const Action *alpha = app.findActionById(kSmokeAlpha);
    if (recentHelper == nullptr || frequentHelper == nullptr || cancelHelper == nullptr
        || alpha == nullptr)
    {
        std::cerr << "meta_loop: smoke helper/library actions missing\n";
        return false;
    }

    // Seed a normal most-recent, then launch helpers. Helpers must not enter history.
    app.executeAction(0); // alpha
    drainScheduler(app);

    // Slots: cancel(3), nth-recent(4), nth-frequent(5)
    app.executeAction(4);
    app.executeAction(5);
    app.executeAction(3);
    drainScheduler(app);

    Action *r1 = app.nthRecentAction(1);
    if (r1 == nullptr || r1->getId() != kSmokeAlpha)
    {
        std::cerr << "meta_loop: helpers were recorded as most recent\n";
        return false;
    }
    if (r1->getId() == recentHelper->getId() || r1->getId() == frequentHelper->getId()
        || r1->getId() == cancelHelper->getId())
    {
        std::cerr << "meta_loop: lookup returned a helper action\n";
        return false;
    }

    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "host", "", kSmokeNthRecent, 0)};
    AI_NthRecent item{1};
    if (item.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "meta_loop: nth_recent execute failed\n";
        return false;
    }
    auto scheduled = ctx.takeScheduledActions();
    if (scheduled.size() != 1 || !scheduled[0].action
        || scheduled[0].action->getId() == kSmokeNthRecent)
    {
        std::cerr << "meta_loop: nth_recent scheduled itself\n";
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv)
{
    // Clear history beside config before App loads it so stale user history
    // cannot steal nth-recent ranks from the fixture ids.
    writeHistoryFile(historyPathBesideConfig(), QJsonArray{}, QJsonObject{});

    QApplication app(argc, argv);

    const std::pair<const char *, bool (*)()> tests[] = {
        {"ranking", testRecentAndFrequentRanking},
        {"persist_round_trip", testPersistRoundTrip},
        {"prune", testPrune},
        {"max_recent_cap", testMaxRecentCapAndEmptyIgnored},
        {"app_lookup_and_items", testAppLookupAndActionItemSchedule},
        {"scheduler_nth_host", testSchedulerRunsScheduledNthTarget},
        {"meta_no_loop", testHistoryMetaActionsDoNotLoop},
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
        std::cerr << failed << " action history smoke test(s) failed\n";
        return 1;
    }

    std::cout << "ActionHistory / nth ActionItem smoke tests passed\n";
    return 0;
}

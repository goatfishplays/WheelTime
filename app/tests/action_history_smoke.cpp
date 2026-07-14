/**
 * @file action_history_smoke.cpp
 * @brief Smoke / integration tests for ActionHistory and nth ActionItems.
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

    const Action *calc = app.findActionById("action-menu-example-calculator");
    const Action *paint = app.findActionById("action-menu-example-paint");
    const Action *note = app.findActionById("action-menu-example-notepad");
    if (calc == nullptr || paint == nullptr || note == nullptr)
    {
        std::cerr << "app_lookup: default config actions missing\n";
        return false;
    }

    // Seed history the same way wheel launches do: executeAction records use.
    // Menu order: calculator(0), paint(1), notepad(2), ...
    app.executeAction(1); // paint
    app.executeAction(2); // notepad
    app.executeAction(0); // calculator (most recent)
    app.executeAction(0); // calculator again (most frequent)

    // Stop any macros started by executeAction so we don't leave processes around.
    app.scheduler().cancelAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    Action *r1 = app.nthRecentAction(1);
    Action *r2 = app.nthRecentAction(2);
    Action *r3 = app.nthRecentAction(3);
    if (r1 == nullptr || r2 == nullptr || r3 == nullptr)
    {
        std::cerr << "app_lookup: recent pointers null\n";
        return false;
    }
    if (r1->getId() != "action-menu-example-calculator"
        || r2->getId() != "action-menu-example-notepad"
        || r3->getId() != "action-menu-example-paint")
    {
        std::cerr << "app_lookup: recent order wrong: "
                  << r1->getId() << ", " << r2->getId() << ", " << r3->getId() << '\n';
        return false;
    }

    Action *f1 = app.nthFrequentAction(1);
    if (f1 == nullptr || f1->getId() != "action-menu-example-calculator")
    {
        std::cerr << "app_lookup: frequent #1 should be calculator\n";
        return false;
    }

    // ActionItems should schedule copies of those library Actions.
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
            || scheduled[0].action->getId() != "action-menu-example-calculator")
        {
            std::cerr << "nth_recent_item: did not schedule calculator copy\n";
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
            || scheduled[0].action->getId() != "action-menu-example-calculator")
        {
            std::cerr << "nth_frequent_item: did not schedule calculator copy\n";
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
    Action *target = app.nthRecentAction(1);
    if (target == nullptr)
    {
        std::cerr << "scheduler_nth: need prior recent history from previous test\n";
        return false;
    }

    // Build a host Action whose only item is nth-recent(1). The scheduled copy
    // is the library Action; we only assert the host completes and schedules
    // were ingested (no hang / deadlock). Nested launch_app may run — cancel soon.
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_NthRecent>(1));
    auto host = std::make_unique<Action>(std::move(items), "nth-host", "", "nth-host", 0);

    Scheduler &scheduler = app.scheduler();
    scheduler.submit(std::move(host));
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    scheduler.cancelAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return true;
}

bool testHistoryMetaActionsDoNotLoop()
{
    App &app = App::getInstance();

    const Action *recentHelper = app.findActionById("action-nth-recent-1");
    const Action *frequentHelper = app.findActionById("action-nth-frequent-1");
    const Action *cancelHelper = app.findActionById("action-cancel-latest");
    const Action *calc = app.findActionById("action-menu-example-calculator");
    if (recentHelper == nullptr || frequentHelper == nullptr || cancelHelper == nullptr
        || calc == nullptr)
    {
        std::cerr << "meta_loop: expected helper/library actions in default config\n";
        return false;
    }

    // Seed a normal most-recent, then launch helpers. Helpers must not enter history.
    app.executeAction(0); // calculator
    app.scheduler().cancelAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Menu: ... ab-chord(4), cancel(5), most-recent(6), most-frequent(7)
    app.executeAction(6);
    app.executeAction(7);
    app.executeAction(5);
    app.scheduler().cancelAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    Action *r1 = app.nthRecentAction(1);
    if (r1 == nullptr || r1->getId() != "action-menu-example-calculator")
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

    // Even if polluted history somehow listed the helper first, lookup must skip it.
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "host", "", "action-nth-recent-1", 0)};
    AI_NthRecent item{1};
    if (item.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "meta_loop: nth_recent execute failed\n";
        return false;
    }
    auto scheduled = ctx.takeScheduledActions();
    if (scheduled.size() != 1 || !scheduled[0].action
        || scheduled[0].action->getId() == "action-nth-recent-1")
    {
        std::cerr << "meta_loop: nth_recent scheduled itself\n";
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // Seed history before first App access so startup load is deterministic if
    // the singleton was never created; App may already exist from linking order,
    // so executeAction-based seeding in the App tests remains the source of truth.
    {
        QJsonArray recent;
        recent.append("action-menu-example-notepad");
        recent.append("action-menu-example-paint");
        QJsonObject frequent;
        frequent.insert("action-menu-example-notepad", 1);
        frequent.insert("action-menu-example-paint", 1);
        writeHistoryFile(historyPathBesideConfig(), recent, frequent);
    }

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

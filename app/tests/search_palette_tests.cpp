/**
 * @file search_palette_tests.cpp
 * @brief Automated tests for the search palette feature.
 *
 * Covers the fuzzy scorer, websearch URL building, AI_Search defaults /
 * clone / JSON round-trip, history-meta classification, ProgramIndex
 * invariants, and SearchPaletteWidget result assembly (filters, category
 * priority, websearch fallback, selection). Widget tests install fixture
 * actions/menus on the App singleton, mirroring action_history_smoke.
 */

#include "App/Action.hpp"
#include "App/ActionItems.hpp"
#include "App/App.hpp"
#include "App/FuzzyMatch.hpp"
#include "App/MenuConfigLoader.hpp"
#include "App/ProgramIndex.hpp"
#include "App/SearchConfig.hpp"
#include "App/SearchPaletteWidget.hpp"

#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

constexpr const char *kFixtureActionA = "action-searchtest-alpha";
constexpr const char *kFixtureActionB = "action-searchtest-beta";
constexpr const char *kFixtureCancel = "action-searchtest-cancel";
constexpr const char *kFixtureMenuId = "menu-searchtest";

// ---------------------------------------------------------------- fuzzy

bool testFuzzyBasics()
{
    if (!fuzzyMatch("", "anything").matched)
    {
        std::cerr << "fuzzy: empty pattern should match\n";
        return false;
    }
    if (fuzzyMatch("a", "").matched)
    {
        std::cerr << "fuzzy: empty text should not match\n";
        return false;
    }
    if (!fuzzyMatch("frfx", "Firefox").matched)
    {
        std::cerr << "fuzzy: subsequence should match\n";
        return false;
    }
    if (!fuzzyMatch("FIREFOX", "firefox").matched)
    {
        std::cerr << "fuzzy: match should be case-insensitive\n";
        return false;
    }
    if (fuzzyMatch("xf", "Firefox").matched)
    {
        std::cerr << "fuzzy: out-of-order pattern should not match\n";
        return false;
    }
    if (fuzzyMatch("firefoxx", "Firefox").matched)
    {
        std::cerr << "fuzzy: pattern longer than matchable text should fail\n";
        return false;
    }
    return true;
}

bool testFuzzyRankingHeuristics()
{
    // Prefix match beats the same match mid-string.
    const int prefix = fuzzyMatch("fire", "Firefox").score;
    const int mid = fuzzyMatch("fire", "Cease Firefox").score;
    if (prefix <= mid)
    {
        std::cerr << "fuzzy: prefix should outscore mid-string (" << prefix << " vs " << mid << ")\n";
        return false;
    }

    // Consecutive run beats the same letters scattered.
    const int consecutive = fuzzyMatch("fox", "foxhound").score;
    const int scattered = fuzzyMatch("fox", "f o x hound").score;
    if (consecutive <= scattered)
    {
        std::cerr << "fuzzy: consecutive should outscore scattered\n";
        return false;
    }

    // Tighter target wins on otherwise-equal matches (trailing penalty).
    const int exact = fuzzyMatch("note", "Note").score;
    const int longer = fuzzyMatch("note", "Notebook Configuration Manager").score;
    if (exact <= longer)
    {
        std::cerr << "fuzzy: shorter target should rank higher\n";
        return false;
    }

    // Word-boundary hits (initials) should still match and score decently.
    if (!fuzzyMatch("vsc", "Visual Studio Code").matched)
    {
        std::cerr << "fuzzy: initials should match\n";
        return false;
    }
    return true;
}

// ------------------------------------------------------------- web url

bool testBuildWebSearchUrl()
{
    const QString url = SearchPaletteWidget::buildWebSearchUrl(
        "https://www.google.com/search?q={query}", "hello world & more");
    if (url != "https://www.google.com/search?q=hello%20world%20%26%20more")
    {
        std::cerr << "weburl: placeholder substitution wrong: " << url.toStdString() << '\n';
        return false;
    }

    const QString appended = SearchPaletteWidget::buildWebSearchUrl(
        "https://duckduckgo.com/?q=", "cats");
    if (appended != "https://duckduckgo.com/?q=cats")
    {
        std::cerr << "weburl: append-without-placeholder wrong: " << appended.toStdString() << '\n';
        return false;
    }

    const QString multi = SearchPaletteWidget::buildWebSearchUrl(
        "https://x.test/{query}/{query}", "a");
    if (multi != "https://x.test/a/a")
    {
        std::cerr << "weburl: all placeholders should be replaced\n";
        return false;
    }
    return true;
}

// ------------------------------------------------------------ AI_Search

bool testSearchItemDefaultsAndClone()
{
    AI_Search item;
    if (item.kind() != ActionItemKind::Search)
    {
        std::cerr << "item: kind wrong\n";
        return false;
    }
    if (std::string(actionItemKindName(ActionItemKind::Search)) != "search")
    {
        std::cerr << "item: kind name wrong\n";
        return false;
    }
    if (!item.config.searchActions || !item.config.searchPrograms || !item.config.searchMenus
        || !item.config.webSearch)
    {
        std::cerr << "item: filters should default to true\n";
        return false;
    }
    if (item.config.webSearchUrl != "https://www.google.com/search?q={query}")
    {
        std::cerr << "item: default url wrong\n";
        return false;
    }

    item.config.searchPrograms = false;
    item.config.webSearchUrl = "https://example.test/?q={query}";
    auto cloned = item.clone();
    auto *clonedSearch = dynamic_cast<AI_Search *>(cloned.get());
    if (clonedSearch == nullptr || clonedSearch->config.searchPrograms
        || clonedSearch->config.webSearchUrl != "https://example.test/?q={query}")
    {
        std::cerr << "item: clone did not preserve config\n";
        return false;
    }
    return true;
}

bool testIsHistoryMetaAction()
{
    auto makeSingleItemAction = [](std::unique_ptr<ActionItem> item, const char *id)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::move(item));
        return Action(std::move(items), id, "", id, 0);
    };

    if (App::isHistoryMetaAction(makeSingleItemAction(std::make_unique<AI_Search>(), "s")))
    {
        std::cerr << "meta: search should not be a history meta action\n";
        return false;
    }
    if (App::isHistoryMetaAction(makeSingleItemAction(std::make_unique<AI_Delay>(1), "d")))
    {
        std::cerr << "meta: delay should not be a history meta action\n";
        return false;
    }
    if (!App::isHistoryMetaAction(
            makeSingleItemAction(std::make_unique<AI_Cancel>(CancelLevel::All), "c")))
    {
        std::cerr << "meta: cancel should be a history meta action\n";
        return false;
    }
    if (!App::isHistoryMetaAction(makeSingleItemAction(std::make_unique<AI_NthRecent>(1), "n")))
    {
        std::cerr << "meta: nth-recent should be a history meta action\n";
        return false;
    }
    return true;
}

// --------------------------------------------------------- json config

bool writeJson(const QString &path, const QJsonObject &root)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

const AI_Search *findSearchItem(const std::vector<Action> &actions, const std::string &actionId)
{
    for (const Action &action : actions)
    {
        if (action.getId() != actionId)
        {
            continue;
        }
        for (const auto &item : action.getItems())
        {
            if (item && item->kind() == ActionItemKind::Search)
            {
                return static_cast<const AI_Search *>(item.get());
            }
        }
    }
    return nullptr;
}

bool testConfigParseDefaultsAndValues()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "config: temp dir failed\n";
        return false;
    }

    // One search item with explicit values, one with everything omitted.
    QJsonObject explicitItem;
    explicitItem.insert("type", "search");
    explicitItem.insert("searchActions", false);
    explicitItem.insert("searchPrograms", true);
    explicitItem.insert("searchMenus", false);
    explicitItem.insert("webSearch", true);
    explicitItem.insert("webSearchUrl", "https://example.test/?q={query}");

    QJsonObject bareItem;
    bareItem.insert("type", "search");

    QJsonObject explicitAction;
    explicitAction.insert("id", "act-explicit");
    explicitAction.insert("name", "Explicit");
    explicitAction.insert("items", QJsonArray{explicitItem});
    QJsonObject bareAction;
    bareAction.insert("id", "act-bare");
    bareAction.insert("name", "Bare");
    bareAction.insert("items", QJsonArray{bareItem});

    QJsonObject menu;
    menu.insert("id", "menu-1");
    menu.insert("name", "Menu");
    menu.insert("actionIds", QJsonArray{"act-explicit", "act-bare"});

    QJsonObject root;
    root.insert("actions", QJsonArray{explicitAction, bareAction});
    root.insert("menus", QJsonArray{menu});

    const QString path = dir.filePath("config.json");
    if (!writeJson(path, root))
    {
        std::cerr << "config: write failed\n";
        return false;
    }

    AppConfig appConfig;
    std::vector<Action> actions;
    std::vector<Menu *> menus;
    const bool loaded = MenuConfigLoader::loadConfig(path, appConfig, actions, menus);
    const auto cleanup = [&menus]()
    {
        for (Menu *m : menus)
        {
            delete m;
        }
        menus.clear();
    };

    if (!loaded)
    {
        std::cerr << "config: load failed\n";
        cleanup();
        return false;
    }

    const AI_Search *explicitSearch = findSearchItem(actions, "act-explicit");
    if (explicitSearch == nullptr || explicitSearch->config.searchActions
        || !explicitSearch->config.searchPrograms || explicitSearch->config.searchMenus
        || !explicitSearch->config.webSearch
        || explicitSearch->config.webSearchUrl != "https://example.test/?q={query}")
    {
        std::cerr << "config: explicit search item parsed wrong\n";
        cleanup();
        return false;
    }

    const AI_Search *bareSearch = findSearchItem(actions, "act-bare");
    if (bareSearch == nullptr || !bareSearch->config.searchActions
        || !bareSearch->config.searchPrograms || !bareSearch->config.searchMenus
        || !bareSearch->config.webSearch
        || bareSearch->config.webSearchUrl != "https://www.google.com/search?q={query}")
    {
        std::cerr << "config: bare search item should get defaults\n";
        cleanup();
        return false;
    }

    cleanup();
    return true;
}

bool testConfigSaveLoadRoundTrip()
{
    QTemporaryDir dir;
    if (!dir.isValid())
    {
        std::cerr << "roundtrip: temp dir failed\n";
        return false;
    }

    SearchConfig config;
    config.searchActions = true;
    config.searchPrograms = false;
    config.searchMenus = true;
    config.webSearch = false;
    config.webSearchUrl = "https://rt.test/?q={query}";

    std::vector<Action> actions;
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Search>(config));
        actions.emplace_back(std::move(items), "RT Search", "", "act-rt", 0);
    }
    std::vector<Menu *> menus;
    menus.push_back(new Menu(0, 0, false, false, true, false, "RT Menu", {"act-rt"}, "menu-rt"));

    const QString path = dir.filePath("roundtrip.json");
    AppConfig appConfig;
    const bool saved = MenuConfigLoader::saveConfig(path, appConfig, actions, menus);
    for (Menu *m : menus)
    {
        delete m;
    }
    menus.clear();
    if (!saved)
    {
        std::cerr << "roundtrip: save failed\n";
        return false;
    }

    std::vector<Action> reloadedActions;
    std::vector<Menu *> reloadedMenus;
    if (!MenuConfigLoader::loadConfig(path, appConfig, reloadedActions, reloadedMenus))
    {
        std::cerr << "roundtrip: reload failed\n";
        return false;
    }
    const auto cleanup = [&reloadedMenus]()
    {
        for (Menu *m : reloadedMenus)
        {
            delete m;
        }
        reloadedMenus.clear();
    };

    const AI_Search *reloaded = findSearchItem(reloadedActions, "act-rt");
    if (reloaded == nullptr || reloaded->config.searchActions != config.searchActions
        || reloaded->config.searchPrograms != config.searchPrograms
        || reloaded->config.searchMenus != config.searchMenus
        || reloaded->config.webSearch != config.webSearch
        || reloaded->config.webSearchUrl != config.webSearchUrl)
    {
        std::cerr << "roundtrip: search config not preserved\n";
        cleanup();
        return false;
    }

    cleanup();
    return true;
}

// --------------------------------------------------------- ProgramIndex

bool testProgramIndexInvariants()
{
    ProgramIndex index;
    const QVector<ProgramEntry> first = index.entries();

    // Optional local debugging: SEARCHTEST_DUMP=1 prints the indexed programs.
    if (qEnvironmentVariableIsSet("SEARCHTEST_DUMP"))
    {
        std::cout << "programs indexed: " << first.size() << '\n';
        for (const ProgramEntry &entry : first)
        {
            std::cout << "  " << entry.name.toStdString() << " -> "
                      << entry.path.toStdString() << '\n';
        }
    }

    // Sorted and deduplicated by case-insensitive display name.
    for (int i = 1; i < first.size(); ++i)
    {
        if (first[i - 1].name.localeAwareCompare(first[i].name) > 0)
        {
            std::cerr << "programs: entries not sorted at index " << i << '\n';
            return false;
        }
        if (first[i - 1].name.toLower() == first[i].name.toLower())
        {
            std::cerr << "programs: duplicate name '" << first[i].name.toStdString() << "'\n";
            return false;
        }
    }
    for (const ProgramEntry &entry : first)
    {
        const bool knownShortcutType = entry.path.endsWith(".lnk", Qt::CaseInsensitive)
                                       || entry.path.endsWith(".url", Qt::CaseInsensitive);
        if (entry.name.isEmpty() || !knownShortcutType)
        {
            std::cerr << "programs: bad entry '" << entry.path.toStdString() << "'\n";
            return false;
        }
    }

    // Later calls serve the cache without rescanning.
    const QVector<ProgramEntry> second = index.entries();
    if (second.size() != first.size())
    {
        std::cerr << "programs: cached call should match first scan size\n";
        return false;
    }

    // Explicit background refresh must not deadlock (destructor joins it) and
    // repeated requests while one is running must be safe.
    index.refreshAsync();
    index.refreshAsync();
    const QVector<ProgramEntry> third = index.entries();
    (void)third;

    // Startup preload path: refreshAsync before any entries() call fills the
    // cache in the background; entries() waits for it instead of rescanning.
    {
        ProgramIndex preloaded;
        preloaded.refreshAsync();
        const QVector<ProgramEntry> viaPreload = preloaded.entries();
        if (viaPreload.size() != first.size())
        {
            std::cerr << "programs: preloaded scan should match direct scan size\n";
            return false;
        }
    }

    // Icon cache: null until extracted, cached afterwards.
    if (!first.isEmpty())
    {
        const QString path = first.front().path;
        if (!index.cachedIconFor(path).isNull())
        {
            std::cerr << "programs: icon cache should start empty\n";
            return false;
        }
        const QIcon extracted = index.iconFor(path);
        if (extracted.isNull() || index.cachedIconFor(path).isNull())
        {
            std::cerr << "programs: iconFor should populate the cache\n";
            return false;
        }
    }
    return true;
}

// ----------------------------------------------------- palette + App

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

/// @brief Installs uniquely named fixture actions/menu (never saved to disk).
void installSearchFixtures(App &app)
{
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Delay>(1));
        upsertAction(app, Action(std::move(items), "Zzqx Fixture Alpha", "", kFixtureActionA, 0));
    }
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Delay>(1));
        upsertAction(app, Action(std::move(items), "Zzqx Fixture Beta", "", kFixtureActionB, 0));
    }
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Cancel>(CancelLevel::All));
        upsertAction(app, Action(std::move(items), "Zzqx Fixture Cancel", "", kFixtureCancel, 0));
    }

    if (app.findMenuById(kFixtureMenuId) == nullptr)
    {
        app.loadedMenus.push_back(new Menu(
            0, 0, false, false, true, false, "Zzqx Fixture Menu", {kFixtureActionA}, kFixtureMenuId));
    }
}

int categoryRank(const QString &category)
{
    if (category == "Action")
        return 0;
    if (category == "Program")
        return 1;
    if (category == "Menu")
        return 2;
    if (category == "Web")
        return 3;
    return 99;
}

bool testPaletteFiltersAndPriority()
{
    App &app = App::getInstance();
    installSearchFixtures(app);

    SearchPaletteWidget palette;

    // Programs disabled for determinism (machine-independent expectations).
    SearchConfig config;
    config.searchPrograms = false;
    palette.openWithConfig(config);
    palette.setQueryText("zzqx fixture");

    // Expect: alpha + beta actions first, then the fixture menu, then web.
    const int count = palette.resultCount();
    if (count < 4)
    {
        std::cerr << "palette: expected >= 4 rows, got " << count << '\n';
        return false;
    }
    if (palette.selectedRow() != 0)
    {
        std::cerr << "palette: first row should be selected after refresh\n";
        return false;
    }

    bool sawAlpha = false;
    bool sawMenu = false;
    int lastRank = 0;
    for (int row = 0; row < count; ++row)
    {
        const int rank = categoryRank(palette.resultCategoryAt(row));
        if (rank < lastRank)
        {
            std::cerr << "palette: category priority violated at row " << row << '\n';
            return false;
        }
        lastRank = rank;
        if (palette.resultLabelAt(row) == "Zzqx Fixture Alpha"
            && palette.resultCategoryAt(row) == "Action")
        {
            sawAlpha = true;
        }
        if (palette.resultLabelAt(row) == "Zzqx Fixture Menu"
            && palette.resultCategoryAt(row) == "Menu")
        {
            sawMenu = true;
        }
        if (palette.resultLabelAt(row) == "Zzqx Fixture Cancel")
        {
            std::cerr << "palette: history meta action should be excluded\n";
            return false;
        }
    }
    if (!sawAlpha || !sawMenu)
    {
        std::cerr << "palette: fixture action/menu missing from results\n";
        return false;
    }

    // Web fallback is pinned last and quotes the query.
    if (palette.resultCategoryAt(count - 1) != "Web"
        || !palette.resultLabelAt(count - 1).contains("zzqx fixture"))
    {
        std::cerr << "palette: web fallback row missing or not last\n";
        return false;
    }
    return true;
}

bool testPaletteFilterTogglesAndFallback()
{
    App &app = App::getInstance();
    installSearchFixtures(app);

    SearchPaletteWidget palette;

    // Actions filter off: fixture action must not appear even though it matches.
    SearchConfig noActions;
    noActions.searchActions = false;
    noActions.searchPrograms = false;
    palette.openWithConfig(noActions);
    palette.setQueryText("zzqx fixture alpha");
    for (int row = 0; row < palette.resultCount(); ++row)
    {
        if (palette.resultCategoryAt(row) == "Action")
        {
            std::cerr << "toggles: action row shown with actions filter off\n";
            return false;
        }
    }

    // Nothing matches + web on: exactly the single fallback row.
    SearchConfig webOnlyMatch;
    webOnlyMatch.searchPrograms = false;
    palette.openWithConfig(webOnlyMatch);
    palette.setQueryText("qqqqzzzz-no-such-thing");
    if (palette.resultCount() != 1 || palette.resultCategoryAt(0) != "Web")
    {
        std::cerr << "toggles: web fallback should be the only row\n";
        return false;
    }

    // Nothing matches + web off: no rows at all.
    SearchConfig noWeb;
    noWeb.searchPrograms = false;
    noWeb.webSearch = false;
    palette.openWithConfig(noWeb);
    palette.setQueryText("qqqqzzzz-no-such-thing");
    if (palette.resultCount() != 0)
    {
        std::cerr << "toggles: expected zero rows with everything filtered out\n";
        return false;
    }

    // Empty query: rows shown (rofi-style initial list), but no web row.
    SearchConfig initial;
    initial.searchPrograms = false;
    palette.openWithConfig(initial);
    if (palette.resultCount() == 0)
    {
        std::cerr << "toggles: empty query should list enabled items\n";
        return false;
    }
    for (int row = 0; row < palette.resultCount(); ++row)
    {
        if (palette.resultCategoryAt(row) == "Web")
        {
            std::cerr << "toggles: empty query should not offer websearch\n";
            return false;
        }
    }
    return true;
}

bool testExecuteActionByIdRecordsHistory()
{
    App &app = App::getInstance();
    installSearchFixtures(app);

    app.executeActionById(kFixtureActionB);
    app.scheduler().cancelAll();

    Action *mostRecent = app.nthRecentAction(1);
    if (mostRecent == nullptr || mostRecent->getId() != kFixtureActionB)
    {
        std::cerr << "history: executeActionById should record usage\n";
        return false;
    }

    // Unknown ids must be a safe no-op.
    app.executeActionById("no-such-action-id");

    // Meta helpers must not enter history even via the search path.
    app.executeActionById(kFixtureCancel);
    app.scheduler().cancelAll();
    mostRecent = app.nthRecentAction(1);
    if (mostRecent == nullptr || mostRecent->getId() == kFixtureCancel)
    {
        std::cerr << "history: meta action leaked into history\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    const std::pair<const char *, bool (*)()> tests[] = {
        {"fuzzy_basics", testFuzzyBasics},
        {"fuzzy_ranking", testFuzzyRankingHeuristics},
        {"web_search_url", testBuildWebSearchUrl},
        {"search_item_defaults_clone", testSearchItemDefaultsAndClone},
        {"history_meta_classification", testIsHistoryMetaAction},
        {"config_parse_defaults", testConfigParseDefaultsAndValues},
        {"config_save_load_round_trip", testConfigSaveLoadRoundTrip},
        {"program_index_invariants", testProgramIndexInvariants},
        {"palette_filters_and_priority", testPaletteFiltersAndPriority},
        {"palette_toggles_and_fallback", testPaletteFilterTogglesAndFallback},
        {"execute_action_by_id_history", testExecuteActionByIdRecordsHistory},
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
        std::cerr << failed << " search palette test(s) failed\n";
        return 1;
    }

    std::cout << "Search palette tests passed\n";
    return 0;
}

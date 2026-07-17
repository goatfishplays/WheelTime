/**
 * @file SearchPaletteWidget.cpp
 * @brief Search palette definitions.
 */

#include "App/SearchPaletteWidget.hpp"

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

#include "App/App.hpp"
#include "App/FuzzyMatch.hpp"

using namespace Application;

namespace
{

constexpr int kMaxResultRows = 40;
/// @brief Shortcut icons extracted per icon-fill timer tick (keeps each tick
/// well under a frame; extraction is the slow SHGetFileInfo path).
constexpr int kIconsPerFillChunk = 6;
constexpr int kIconFillIntervalMs = 15;

QToolButton *makeFilterChip(const QString &text, const QString &shortcutHint, QWidget *parent)
{
    auto *chip = new QToolButton(parent);
    chip->setObjectName("searchFilterChip");
    chip->setText(text);
    chip->setToolTip(QString("%1 (%2)").arg(text, shortcutHint));
    chip->setCheckable(true);
    chip->setChecked(true);
    // Chips must never take keyboard focus away from the search field.
    chip->setFocusPolicy(Qt::NoFocus);
    chip->setCursor(Qt::PointingHandCursor);
    return chip;
}

} // namespace

SearchPaletteWidget::SearchPaletteWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("searchPanel");
    // QWidget subclasses need this for QSS background/border painting.
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(640);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    m_input = new QLineEdit(this);
    m_input->setObjectName("searchInput");
    m_input->setPlaceholderText("Search…");
    m_input->setClearButtonEnabled(true);
    m_input->installEventFilter(this);
    layout->addWidget(m_input);

    auto *chipRow = new QHBoxLayout();
    chipRow->setSpacing(6);
    m_filterActions = makeFilterChip("Actions", "Ctrl+1", this);
    m_filterPrograms = makeFilterChip("Programs", "Ctrl+2", this);
    m_filterMenus = makeFilterChip("Menus", "Ctrl+3", this);
    m_filterWeb = makeFilterChip("Web", "Ctrl+4", this);
    chipRow->addWidget(m_filterActions);
    chipRow->addWidget(m_filterPrograms);
    chipRow->addWidget(m_filterMenus);
    chipRow->addWidget(m_filterWeb);
    chipRow->addStretch();
    layout->addLayout(chipRow);

    m_results = new QListWidget(this);
    m_results->setObjectName("searchResults");
    m_results->setFocusPolicy(Qt::NoFocus);
    m_results->setMouseTracking(true);
    m_results->setUniformItemSizes(true);
    m_results->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_results->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_results->setFixedHeight(360);
    m_results->setIconSize(QSize(20, 20));
    layout->addWidget(m_results);

    // Shortcut icon extraction is too slow for the per-keystroke path; rows
    // appear immediately and icons stream in over a few timer ticks.
    m_iconFillTimer = new QTimer(this);
    m_iconFillTimer->setInterval(kIconFillIntervalMs);
    connect(m_iconFillTimer, &QTimer::timeout, this, [this]() { fillPendingIconsChunk(); });

    connect(m_input, &QLineEdit::textChanged, this, [this]() { refreshResults(); });
    for (QToolButton *chip : {m_filterActions, m_filterPrograms, m_filterMenus, m_filterWeb})
    {
        connect(chip, &QToolButton::toggled, this, [this]() { refreshResults(); });
    }
    connect(m_results, &QListWidget::itemEntered, this, [this](QListWidgetItem *item)
            { m_results->setCurrentItem(item); });
    connect(m_results, &QListWidget::itemClicked, this, [this](QListWidgetItem *item)
            { activateRow(m_results->row(item)); });
}

void SearchPaletteWidget::openWithConfig(const SearchConfig &config)
{
    m_config = config;

    // Block chip signals so the reset does not run four intermediate queries.
    const QSignalBlocker blockActions(m_filterActions);
    const QSignalBlocker blockPrograms(m_filterPrograms);
    const QSignalBlocker blockMenus(m_filterMenus);
    const QSignalBlocker blockWeb(m_filterWeb);
    m_filterActions->setChecked(config.searchActions);
    m_filterPrograms->setChecked(config.searchPrograms);
    m_filterMenus->setChecked(config.searchMenus);
    m_filterWeb->setChecked(config.webSearch);

    const QSignalBlocker blockInput(m_input);
    m_input->clear();

    // Pick up newly installed programs without rescanning per keystroke.
    m_programIndex.refreshAsync();

    refreshResults();
}

void SearchPaletteWidget::focusInput()
{
    m_input->setFocus(Qt::OtherFocusReason);
}

void SearchPaletteWidget::preloadIndex()
{
    m_programIndex.refreshAsync();
}

void SearchPaletteWidget::setQueryText(const QString &text)
{
    m_input->setText(text);
}

int SearchPaletteWidget::resultCount() const
{
    // The pooled QListWidget keeps hidden spare rows; only live entries count.
    return static_cast<int>(m_entries.size());
}

int SearchPaletteWidget::selectedRow() const
{
    return m_results->currentRow();
}

QString SearchPaletteWidget::resultLabelAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_entries.size()))
    {
        return {};
    }
    return m_entries[static_cast<std::size_t>(row)].label;
}

QString SearchPaletteWidget::resultCategoryAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_entries.size()))
    {
        return {};
    }
    return categoryName(m_entries[static_cast<std::size_t>(row)].kind);
}

QString SearchPaletteWidget::buildWebSearchUrl(const QString &urlTemplate, const QString &query)
{
    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(query));
    QString url = urlTemplate;
    if (url.contains("{query}"))
    {
        url.replace("{query}", encoded);
    }
    else
    {
        url += encoded;
    }
    return url;
}

bool SearchPaletteWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (handlePaletteKey(keyEvent))
        {
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SearchPaletteWidget::keyPressEvent(QKeyEvent *event)
{
    if (handlePaletteKey(event))
    {
        return;
    }
    QWidget::keyPressEvent(event);
}

bool SearchPaletteWidget::handlePaletteKey(QKeyEvent *keyEvent)
{
    switch (keyEvent->key())
    {
    case Qt::Key_Escape:
        App::getInstance().hideSearchOverlay();
        return true;
    case Qt::Key_Up:
        moveSelection(-1);
        return true;
    case Qt::Key_Down:
        moveSelection(1);
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        activateRow(m_results->currentRow());
        return true;
    default:
        break;
    }

    if (keyEvent->modifiers().testFlag(Qt::ControlModifier))
    {
        QToolButton *chip = nullptr;
        switch (keyEvent->key())
        {
        case Qt::Key_1:
            chip = m_filterActions;
            break;
        case Qt::Key_2:
            chip = m_filterPrograms;
            break;
        case Qt::Key_3:
            chip = m_filterMenus;
            break;
        case Qt::Key_4:
            chip = m_filterWeb;
            break;
        default:
            break;
        }
        if (chip != nullptr)
        {
            chip->toggle();
            return true;
        }
    }

    return false;
}

void SearchPaletteWidget::moveSelection(int delta)
{
    // Pooled rows past the current results are hidden; wrap over live rows only.
    const int count = static_cast<int>(m_entries.size());
    if (count == 0)
    {
        return;
    }
    int row = m_results->currentRow();
    row = row < 0 ? 0 : (row + delta + count) % count;
    m_results->setCurrentRow(row);
}

QString SearchPaletteWidget::categoryName(ResultKind kind) const
{
    switch (kind)
    {
    case ResultKind::Action:
        return "Action";
    case ResultKind::Program:
        return "Program";
    case ResultKind::Menu:
        return "Menu";
    case ResultKind::Web:
        return "Web";
    }
    return {};
}

QIcon SearchPaletteWidget::resolveActionIcon(const std::string &iconFilepath)
{
    if (iconFilepath.empty())
    {
        return {};
    }
    QString iconPath = QString::fromStdString(iconFilepath);
    if (!QFileInfo(iconPath).isAbsolute())
    {
        const QDir configDir = QFileInfo(App::getInstance().getConfigPath()).absoluteDir();
        iconPath = configDir.filePath(iconPath);
    }
    auto it = m_actionIconCache.find(iconPath);
    if (it != m_actionIconCache.end())
    {
        return it.value();
    }
    QIcon icon(iconPath);
    m_actionIconCache.insert(iconPath, icon);
    return icon;
}

void SearchPaletteWidget::collectActionResults(const QString &query, std::vector<ResultEntry> &out)
{
    const App &app = App::getInstance();
    std::vector<ResultEntry> matches;
    for (const Action &action : app.getActionLibrary())
    {
        // History/cancel helpers make no sense to launch from search.
        if (App::isHistoryMetaAction(action))
        {
            continue;
        }
        const QString name = QString::fromStdString(action.getName());
        const FuzzyResult match = fuzzyMatch(query, name);
        if (!match.matched)
        {
            continue;
        }
        matches.push_back(ResultEntry{
            ResultKind::Action, name, resolveActionIcon(action.getIconFilepath()),
            action.getId(), match.score});
    }
    std::stable_sort(matches.begin(), matches.end(), [](const ResultEntry &a, const ResultEntry &b)
                     { return a.score != b.score ? a.score > b.score
                                                 : a.label.compare(b.label, Qt::CaseInsensitive) < 0; });
    out.insert(out.end(), matches.begin(), matches.end());
}

void SearchPaletteWidget::collectProgramResults(const QString &query, std::vector<ResultEntry> &out)
{
    std::vector<ResultEntry> matches;
    const QVector<ProgramEntry> programs = m_programIndex.entries();
    for (const ProgramEntry &program : programs)
    {
        const FuzzyResult match = fuzzyMatch(query, program.name);
        if (!match.matched)
        {
            continue;
        }
        // Never extract icons here (slow); use whatever is already cached and
        // let the icon-fill timer stream in the rest after the rows render.
        matches.push_back(ResultEntry{
            ResultKind::Program, program.name, m_programIndex.cachedIconFor(program.path),
            program.path.toStdString(), match.score});
    }
    std::stable_sort(matches.begin(), matches.end(), [](const ResultEntry &a, const ResultEntry &b)
                     { return a.score != b.score ? a.score > b.score
                                                 : a.label.compare(b.label, Qt::CaseInsensitive) < 0; });
    out.insert(out.end(), matches.begin(), matches.end());
}

void SearchPaletteWidget::collectMenuResults(const QString &query, std::vector<ResultEntry> &out) const
{
    const App &app = App::getInstance();
    std::vector<ResultEntry> matches;
    for (const Menu *menu : app.loadedMenus)
    {
        if (menu == nullptr)
        {
            continue;
        }
        const QString name = QString::fromStdString(menu->getName());
        const FuzzyResult match = fuzzyMatch(query, name);
        if (!match.matched)
        {
            continue;
        }
        matches.push_back(ResultEntry{
            ResultKind::Menu, name, QIcon(), menu->getId(), match.score});
    }
    std::stable_sort(matches.begin(), matches.end(), [](const ResultEntry &a, const ResultEntry &b)
                     { return a.score != b.score ? a.score > b.score
                                                 : a.label.compare(b.label, Qt::CaseInsensitive) < 0; });
    out.insert(out.end(), matches.begin(), matches.end());
}

void SearchPaletteWidget::refreshResults()
{
    const QString query = m_input->text().trimmed();

    m_entries.clear();
    // Strict category priority: actions, then programs, then menus.
    if (m_filterActions->isChecked())
    {
        collectActionResults(query, m_entries);
    }
    if (m_filterPrograms->isChecked())
    {
        collectProgramResults(query, m_entries);
    }
    if (m_filterMenus->isChecked())
    {
        collectMenuResults(query, m_entries);
    }
    if (m_entries.size() > kMaxResultRows)
    {
        m_entries.resize(kMaxResultRows);
    }

    // Websearch is the pinned fallback row (and the only row when nothing
    // else matches). Meaningless for an empty query.
    if (m_filterWeb->isChecked() && !query.isEmpty())
    {
        const QString url =
            buildWebSearchUrl(QString::fromStdString(m_config.webSearchUrl), query);
        m_entries.push_back(ResultEntry{
            ResultKind::Web, QString("Search web for \"%1\"").arg(query),
            QIcon(), url.toStdString(), 0});
    }

    // Update the pooled rows in place; creating/destroying 40 row widgets
    // per keystroke is what made typing feel sluggish.
    const int rowCount = static_cast<int>(m_entries.size());
    ensureRowPool(rowCount);
    for (int row = 0; row < rowCount; ++row)
    {
        applyEntryToRow(row);
    }
    for (int row = rowCount; row < static_cast<int>(m_rows.size()); ++row)
    {
        m_rows[static_cast<std::size_t>(row)].item->setHidden(true);
    }

    if (rowCount > 0)
    {
        m_results->setCurrentRow(0);
    }
    else
    {
        m_results->setCurrentRow(-1);
    }

    startPendingIconFill();
}

void SearchPaletteWidget::ensureRowPool(int rowCount)
{
    while (static_cast<int>(m_rows.size()) < rowCount)
    {
        ResultRow pooled;
        pooled.item = new QListWidgetItem(m_results);
        pooled.item->setSizeHint(QSize(0, 36));

        auto *row = new QWidget(m_results);
        row->setObjectName("searchResultRow");
        // Let the viewport keep hover/click handling for the whole row.
        row->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 0, 8, 0);
        rowLayout->setSpacing(8);

        pooled.iconLabel = new QLabel(row);
        pooled.iconLabel->setFixedSize(20, 20);
        rowLayout->addWidget(pooled.iconLabel);

        pooled.nameLabel = new QLabel(row);
        pooled.nameLabel->setObjectName("searchResultName");
        rowLayout->addWidget(pooled.nameLabel, 1);

        pooled.categoryLabel = new QLabel(row);
        pooled.categoryLabel->setObjectName("searchResultCategory");
        rowLayout->addWidget(pooled.categoryLabel);

        m_results->setItemWidget(pooled.item, row);
        m_rows.push_back(pooled);
    }
}

void SearchPaletteWidget::applyEntryToRow(int row)
{
    const ResultEntry &entry = m_entries[static_cast<std::size_t>(row)];
    ResultRow &pooled = m_rows[static_cast<std::size_t>(row)];
    if (entry.icon.isNull())
    {
        pooled.iconLabel->clear();
    }
    else
    {
        pooled.iconLabel->setPixmap(entry.icon.pixmap(20, 20));
    }
    pooled.nameLabel->setText(entry.label);
    pooled.categoryLabel->setText(categoryName(entry.kind));
    pooled.item->setHidden(false);
}

void SearchPaletteWidget::startPendingIconFill()
{
    m_pendingIconRows.clear();
    for (int row = 0; row < static_cast<int>(m_entries.size()); ++row)
    {
        const ResultEntry &entry = m_entries[static_cast<std::size_t>(row)];
        if (entry.kind == ResultKind::Program && entry.icon.isNull())
        {
            m_pendingIconRows.push_back(row);
        }
    }
    if (m_pendingIconRows.empty())
    {
        m_iconFillTimer->stop();
    }
    else
    {
        m_iconFillTimer->start();
    }
}

void SearchPaletteWidget::fillPendingIconsChunk()
{
    int processed = 0;
    while (!m_pendingIconRows.empty() && processed < kIconsPerFillChunk)
    {
        const int row = m_pendingIconRows.front();
        m_pendingIconRows.erase(m_pendingIconRows.begin());
        if (row >= static_cast<int>(m_entries.size()))
        {
            continue;
        }
        ResultEntry &entry = m_entries[static_cast<std::size_t>(row)];
        if (entry.kind != ResultKind::Program || !entry.icon.isNull())
        {
            continue;
        }
        entry.icon = m_programIndex.iconFor(QString::fromStdString(entry.target));
        if (!entry.icon.isNull())
        {
            m_rows[static_cast<std::size_t>(row)].iconLabel->setPixmap(entry.icon.pixmap(20, 20));
        }
        ++processed;
    }
    if (m_pendingIconRows.empty())
    {
        m_iconFillTimer->stop();
    }
}

void SearchPaletteWidget::activateRow(int row)
{
    if (row < 0 || row >= static_cast<int>(m_entries.size()))
    {
        return;
    }
    // Copy: hideSearchOverlay tears down state while we still need the entry.
    const ResultEntry entry = m_entries[static_cast<std::size_t>(row)];
    activateEntry(entry);
}

void SearchPaletteWidget::activateEntry(const ResultEntry &entry)
{
    App &app = App::getInstance();
    switch (entry.kind)
    {
    case ResultKind::Action:
        // Restore focus to the prior window first so keystroke/mouse macros
        // land where the user expects, then run the action.
        app.hideSearchOverlay();
        app.executeActionById(entry.target);
        break;
    case ResultKind::Program:
        app.hideSearchOverlay();
        app.executor.executeScript(entry.target);
        break;
    case ResultKind::Menu:
    {
        Menu *menu = app.findMenuById(entry.target);
        // Restore prior focus before entering wheel mode: the wheel is
        // non-activating and expects the user's app to own the keyboard.
        app.hideSearchOverlay();
        if (menu != nullptr)
        {
            app.showGui(menu);
        }
        break;
    }
    case ResultKind::Web:
        app.hideSearchOverlay();
        app.executor.executeScript(entry.target);
        break;
    }
}

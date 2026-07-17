/**
 * @file SearchPaletteWidget.hpp
 * @brief Rofi-style search palette shown inside the overlay shell.
 */

#pragma once

#include <QHash>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QToolButton>
#include <QWidget>

#include <string>
#include <vector>

#include "App/ProgramIndex.hpp"
#include "App/SearchConfig.hpp"

class QTimer;

namespace Application
{

/**
 * @brief Search input + filter chips + ranked results list.
 *
 * Result categories are ranked strictly actions > programs > menus, with a
 * websearch fallback row pinned last. The widget queries App directly for
 * actions/menus and owns the Start Menu program index.
 */
class SearchPaletteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchPaletteWidget(QWidget *parent = nullptr);

    /// @brief Resets text/filters from @p config and rebuilds results.
    void openWithConfig(const SearchConfig &config);
    /// @brief Puts keyboard focus into the search field.
    void focusInput();
    /// @brief Starts the shortcut scan on a background thread so the first
    /// palette open does not block on it. Call once at app startup.
    void preloadIndex();

    /// @brief Substitutes the percent-encoded @p query into @p urlTemplate.
    /// "{query}" is replaced; templates without the placeholder get the
    /// encoded query appended.
    [[nodiscard]] static QString buildWebSearchUrl(const QString &urlTemplate, const QString &query);

    // Introspection for tests/automation.
    /// @brief Sets the query text programmatically (rebuilds results).
    void setQueryText(const QString &text);
    /// @brief Number of rows currently in the results list.
    [[nodiscard]] int resultCount() const;
    /// @brief Currently selected results row, or -1.
    [[nodiscard]] int selectedRow() const;
    /// @brief Display label of a results row (empty when out of range).
    [[nodiscard]] QString resultLabelAt(int row) const;
    /// @brief Category tag ("Action"/"Program"/"Menu"/"Web") of a results row.
    [[nodiscard]] QString resultCategoryAt(int row) const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum class ResultKind
    {
        Action,
        Program,
        Menu,
        Web
    };

    struct ResultEntry
    {
        ResultKind kind{ResultKind::Action};
        QString label;
        QIcon icon;
        /// @brief Action id, menu id, .lnk path, or websearch URL.
        std::string target;
        int score{0};
    };

    /// @brief One pooled results row (created once, updated in place).
    struct ResultRow
    {
        QListWidgetItem *item{nullptr};
        QLabel *iconLabel{nullptr};
        QLabel *nameLabel{nullptr};
        QLabel *categoryLabel{nullptr};
    };

    void refreshResults();
    void collectActionResults(const QString &query, std::vector<ResultEntry> &out);
    void collectProgramResults(const QString &query, std::vector<ResultEntry> &out);
    void collectMenuResults(const QString &query, std::vector<ResultEntry> &out) const;
    void ensureRowPool(int rowCount);
    void applyEntryToRow(int row);
    void startPendingIconFill();
    void fillPendingIconsChunk();
    void moveSelection(int delta);
    void activateRow(int row);
    void activateEntry(const ResultEntry &entry);
    bool handlePaletteKey(QKeyEvent *keyEvent);
    QString categoryName(ResultKind kind) const;
    QIcon resolveActionIcon(const std::string &iconFilepath);

    QLineEdit *m_input{nullptr};
    QToolButton *m_filterActions{nullptr};
    QToolButton *m_filterPrograms{nullptr};
    QToolButton *m_filterMenus{nullptr};
    QToolButton *m_filterWeb{nullptr};
    QListWidget *m_results{nullptr};
    QTimer *m_iconFillTimer{nullptr};

    SearchConfig m_config;
    ProgramIndex m_programIndex;
    std::vector<ResultEntry> m_entries;
    std::vector<ResultRow> m_rows;
    /// @brief Rows still waiting for (slow) shortcut icon extraction.
    std::vector<int> m_pendingIconRows;
    /// @brief Caches action icons so QIcon files are not re-read per keystroke.
    QHash<QString, QIcon> m_actionIconCache;
};

} // namespace Application

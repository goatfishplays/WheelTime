#include "App/Gui.hpp"

#include <cmath>
#include <QCursor>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include "App/App.hpp"

using namespace Application;

namespace
{
    class MacroEditorDialog : public QDialog
    {
    public:
        explicit MacroEditorDialog(Menu *menu, QWidget *parent = nullptr)
            : QDialog(parent), m_menu(menu)
        {
            setWindowTitle("Edit Macros");
            resize(620, 360);

            auto *layout = new QVBoxLayout(this);
            m_table = new QTableWidget(this);
            m_table->setColumnCount(2);
            m_table->setHorizontalHeaderLabels({"Button Name", "App or Script Path"});
            m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            layout->addWidget(m_table);

            auto *controls = new QHBoxLayout();
            auto *addButton = new QPushButton("Add Slot", this);
            auto *removeButton = new QPushButton("Remove Slot", this);
            controls->addWidget(addButton);
            controls->addWidget(removeButton);
            controls->addStretch();
            layout->addLayout(controls);

            auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
            layout->addWidget(buttonBox);

            connect(addButton, &QPushButton::clicked, this, [this]()
                    { appendRow("New Action", ""); });
            connect(removeButton, &QPushButton::clicked, this, [this]()
                    {
                        const int row = m_table->currentRow();
                        if (row >= 0)
                        {
                            m_table->removeRow(row);
                        } });
            connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

            populate();
        }

        void apply()
        {
            if (m_menu == nullptr)
            {
                return;
            }

            std::vector<Action> updatedActions;
            for (int row = 0; row < m_table->rowCount(); ++row)
            {
                const QString name = itemText(row, 0).trimmed();
                const QString path = itemText(row, 1).trimmed();
                if (name.isEmpty() && path.isEmpty())
                {
                    continue;
                }

                Action action({}, name.isEmpty() ? "Unnamed Action" : name.toStdString());
                if (!path.isEmpty())
                {
                    action.setScriptAction(path.toStdString());
                }
                updatedActions.push_back(std::move(action));
            }

            m_menu->actions = std::move(updatedActions);
        }

    private:
        QString itemText(int row, int column) const
        {
            QTableWidgetItem *item = m_table->item(row, column);
            return item == nullptr ? QString() : item->text();
        }

        void appendRow(const QString &name, const QString &path)
        {
            const int row = m_table->rowCount();
            m_table->insertRow(row);
            m_table->setItem(row, 0, new QTableWidgetItem(name));
            m_table->setItem(row, 1, new QTableWidgetItem(path));
        }

        void populate()
        {
            if (m_menu == nullptr)
            {
                return;
            }

            for (const Action &action : m_menu->actions)
            {
                appendRow(QString::fromStdString(action.getName()), QString::fromStdString(action.getScriptPath()));
            }
        }

        Menu *m_menu{nullptr};
        QTableWidget *m_table{nullptr};
    };
}

// ---------------- Gui ----------------

Gui::Gui(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(600, 400);
    // showFullScreen();

    // setAttribute(Qt::WA_TranslucentBackground);
    // setWindowFlags(Qt::FramelessWindowHint);

    // setStyleSheet(R"(
    //     QWidget {
    //         background: transparent;
    //     }
    // )");

    qApp->installEventFilter(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // Top title
    m_titleLabel = new QLabel("Title", this);
    m_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    root->addWidget(m_titleLabel);

    // Middle area that holds the radial widget and can expand freely.
    auto *middle = new QWidget(this);
    auto *middleLayout = new QVBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);

    m_radialMenu = new RadialMenuWidget(middle);
    m_radialMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    middleLayout->addWidget(m_radialMenu);

    root->addWidget(middle, 1);

    // Bottom row: spacer pushes buttons to the right.
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();

    m_settingsButton = new QPushButton("Settings", this);
    m_editButton = new QPushButton("Edit", this);

    bottomRow->addWidget(m_settingsButton);
    bottomRow->addWidget(m_editButton);

    root->addLayout(bottomRow);

    // example = "Example Menu";
    // example.actions = {
    //     {"One"},
    //     {"Two"},
    //     {"Three"},
    //     {"Four"}};

    // App::App::getInstance().loadedMenus.push_back(example);
    // App::App::getInstance().showGui(&App::App::getInstance().loadedMenus[0]);
    // m_radialMenu->setMenu(example);
    m_radialMenu->setButtonRadius(100);
    m_radialMenu->setActivationMode(RadialMenuWidget::ActivationMode::Distance);

    connect(m_radialMenu, &RadialMenuWidget::selectedIndexChanged, this, &Gui::onSelectChange);
    connect(m_settingsButton, &QPushButton::clicked, this, &Gui::openMacroEditor);
    connect(m_editButton, &QPushButton::clicked, this, &Gui::openMacroEditor);

    connect(m_radialMenu, &RadialMenuWidget::buttonTriggered, this, [](int index)
            { qDebug() << "Button clicked:" << index;
            App::App::getInstance().executeAction(index); });
}

void Gui::onSelectChange(int index)
{
    qDebug() << "Selected index changed:" << index;
    // m_radialMenu->highlightButton(m_radialMenu->getSelectedIndex());
};

bool Gui::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        auto *mouseMoveEvent = static_cast<QMouseEvent *>(event);
        m_radialMenu->updateSelectionFromGlobalMousePosition(mouseMoveEvent->globalPosition().toPoint());
    }
    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());
        QWidget *clickedChild = childAt(localPos);

        if (mouseEvent->button() == Qt::LeftButton)
        {
            if (qobject_cast<QPushButton *>(watched) != nullptr || qobject_cast<QPushButton *>(clickedChild) != nullptr)
            {
                return QWidget::eventFilter(watched, event);
            }

            if (m_radialMenu->geometry().contains(localPos))
            {
                App::App::getInstance().executeAction(m_radialMenu->getSelectedIndex());
                return true;
            }

            return QWidget::eventFilter(watched, event);
        }
        if (mouseEvent->button() == Qt::RightButton && m_radialMenu->geometry().contains(localPos))
        {
            App::App::getInstance().hideGui();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void Gui::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit escapePressed();
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

void Gui::setMenu(const Menu &menu) // TODO: might be better to just make the radial menu public
{
    m_radialMenu->setMenu(menu);
}

void Gui::openMacroEditor()
{
    Menu *menu = App::App::getInstance().getActiveMenu();
    if (menu == nullptr)
    {
        return;
    }

    MacroEditorDialog dialog(menu, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    dialog.apply();
    App::App::getInstance().refreshActiveMenu();

    if (!App::App::getInstance().saveMenus())
    {
        QMessageBox::warning(this, "Save Failed", "Could not save the macro configuration file.");
    }
}

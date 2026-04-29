#include "mainwindow.h"
#include "addtransactiondialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QFrame>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QApplication>

#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

MainWindow::MainWindow(DataManager *dm, const QString &username, QWidget *parent)
    : QMainWindow(parent), m_dm(dm), m_username(username)
{
    setWindowTitle("FinTrack — " + username);
    setMinimumSize(960, 660);
    resize(1100, 720);

    setupUi();
    applyStyle();
    refreshAll();
}

// ─────────────────────────────────────────────
//  Построение UI
// ─────────────────────────────────────────────
void MainWindow::setupUi() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_rootLayout = new QHBoxLayout(m_centralWidget);
    m_rootLayout->setContentsMargins(0, 0, 0, 0);
    m_rootLayout->setSpacing(0);

    setupSidebar();

    // Правая часть: стек страниц
    m_pages = new QStackedWidget();
    setupDashboard();
    setupHistory();

    m_pages->addWidget(m_dashPage);
    m_pages->addWidget(m_histPage);

    m_rootLayout->addWidget(m_sidebar);
    m_rootLayout->addWidget(m_pages, 1);
}

// ──── Левая панель навигации ────
void MainWindow::setupSidebar() {
    m_sidebar = new QFrame();
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(220);

    auto *lay = new QVBoxLayout(m_sidebar);
    lay->setContentsMargins(16, 32, 16, 24);
    lay->setSpacing(4);

    // Лого
    auto *logo = new QLabel("💰 FinTrack");
    logo->setObjectName("sidebarLogo");
    lay->addWidget(logo);
    lay->addSpacing(32);

    // Навигация
    m_navDashboard = new QPushButton("  📊  Дашборд");
    m_navDashboard->setObjectName("navBtn");
    m_navDashboard->setCheckable(true);
    m_navDashboard->setChecked(true);

    m_navHistory = new QPushButton("  📋  История");
    m_navHistory->setObjectName("navBtn");
    m_navHistory->setCheckable(true);

    connect(m_navDashboard, &QPushButton::clicked, this, [this](){
        m_pages->setCurrentIndex(0);
        m_navDashboard->setChecked(true);
        m_navHistory->setChecked(false);
    });
    connect(m_navHistory, &QPushButton::clicked, this, [this](){
        m_pages->setCurrentIndex(1);
        m_navDashboard->setChecked(false);
        m_navHistory->setChecked(true);
    });

    lay->addWidget(m_navDashboard);
    lay->addWidget(m_navHistory);
    lay->addStretch();

    // Информация о пользователе внизу
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("sidebarSep");
    lay->addWidget(sep);
    lay->addSpacing(12);

    m_userLabel = new QLabel("👤 " + m_username);
    m_userLabel->setObjectName("userLabel");
    lay->addWidget(m_userLabel);
}

// ──── Страница Дашборд ────
void MainWindow::setupDashboard() {
    m_dashPage = new QWidget();
    auto *lay = new QVBoxLayout(m_dashPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(20);

    // Заголовок
    auto *header = new QLabel("Обзор финансов");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    // ── Карточки сверху: Баланс / Доходы / Расходы
    auto *cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(16);

    auto makeCard = [](const QString &title, QLabel **valueLabel, const QString &objName) -> QFrame* {
        auto *card = new QFrame();
        card->setObjectName(objName);
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(20, 18, 20, 18);
        auto *t = new QLabel(title);
        t->setObjectName("cardTitle");
        *valueLabel = new QLabel("0 ₽");
        (*valueLabel)->setObjectName("cardValue");
        cl->addWidget(t);
        cl->addWidget(*valueLabel);
        return card;
    };

    auto *balCard     = makeCard("Баланс",   &m_balanceLabel,  "balanceCard");
    auto *incCard     = makeCard("Доходы",   &m_incomeLabel,   "incomeCard");
    auto *expCard     = makeCard("Расходы",  &m_expenseLabel,  "expenseCard");

    cardsRow->addWidget(balCard, 1);
    cardsRow->addWidget(incCard, 1);
    cardsRow->addWidget(expCard, 1);
    lay->addLayout(cardsRow);

    // ── Нижняя секция: диаграмма + список категорий
    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(20);

    // Левый блок: переключатель + диаграмма
    auto *chartCard = new QFrame();
    chartCard->setObjectName("chartCard");
    auto *chartLay = new QVBoxLayout(chartCard);
    chartLay->setContentsMargins(24, 20, 24, 20);
    chartLay->setSpacing(16);

    // Вкладки Расходы / Доходы
    auto *tabRow = new QHBoxLayout();
    tabRow->setSpacing(0);

    // Вкладки переключения Расходы / Доходы
    auto *tabExpBtn2 = new QPushButton("РАСХОДЫ");
    tabExpBtn2->setObjectName("tabBtnActive");
    tabExpBtn2->setCheckable(true);
    tabExpBtn2->setChecked(true);

    auto *tabIncBtn2 = new QPushButton("ДОХОДЫ");
    tabIncBtn2->setObjectName("tabBtnInactive");
    tabIncBtn2->setCheckable(true);

    connect(tabExpBtn2, &QPushButton::clicked, this, [=](){
        m_currentTab = 0;
        tabExpBtn2->setChecked(true);
        tabIncBtn2->setChecked(false);
        tabExpBtn2->setObjectName("tabBtnActive");
        tabIncBtn2->setObjectName("tabBtnInactive");

        // Принудительно обновляем стили
        tabExpBtn2->style()->unpolish(tabExpBtn2);
        tabExpBtn2->style()->polish(tabExpBtn2);
        tabIncBtn2->style()->unpolish(tabIncBtn2);
        tabIncBtn2->style()->polish(tabIncBtn2);

        refreshChart();
        refreshCategories();
    });
    connect(tabIncBtn2, &QPushButton::clicked, this, [=](){
        m_currentTab = 1;
        tabExpBtn2->setChecked(false);
        tabIncBtn2->setChecked(true);
        tabExpBtn2->setObjectName("tabBtnInactive");
        tabIncBtn2->setObjectName("tabBtnActive");

        // Принудительно обновляем стили
        tabExpBtn2->style()->unpolish(tabExpBtn2);
        tabExpBtn2->style()->polish(tabExpBtn2);
        tabIncBtn2->style()->unpolish(tabIncBtn2);
        tabIncBtn2->style()->polish(tabIncBtn2);

        refreshChart();
        refreshCategories();
    });

    tabRow->addWidget(tabExpBtn2);
    tabRow->addWidget(tabIncBtn2);
    chartLay->addLayout(tabRow);

    // Диаграмма (donut)
    m_pieSeries = new QPieSeries();
    m_pieSeries->setHoleSize(0.55);
    m_pieSeries->setPieSize(0.85);

    auto *chart = new QChart();
    chart->addSeries(m_pieSeries);
    chart->setBackgroundBrush(Qt::transparent);
    chart->setBackgroundRoundness(0);
    chart->setMargins(QMargins(0,0,0,0));
    chart->legend()->hide();
    chart->setTitle("");

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);
    m_chartView->setMinimumHeight(260);
    m_chartView->setObjectName("chartView");

    chartLay->addWidget(m_chartView);
    bottomRow->addWidget(chartCard, 1);

    // Правый блок: список категорий
    auto *catCard = new QFrame();
    catCard->setObjectName("catCard");
    auto *catOuterLay = new QVBoxLayout(catCard);
    catOuterLay->setContentsMargins(20, 20, 20, 20);
    catOuterLay->setSpacing(12);

    auto *catTitle = new QLabel("По категориям");
    catTitle->setObjectName("catTitle");
    catOuterLay->addWidget(catTitle);

    // Скролл-область для списка категорий
    auto *scroll = new QScrollArea();
    scroll->setObjectName("catScroll");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *catInner = new QWidget();
    m_catListLayout = new QVBoxLayout(catInner);
    m_catListLayout->setContentsMargins(0,0,0,0);
    m_catListLayout->setSpacing(8);
    m_catListLayout->addStretch();

    scroll->setWidget(catInner);
    catOuterLay->addWidget(scroll, 1);

    bottomRow->addWidget(catCard, 1);

    lay->addLayout(bottomRow, 1);

    // FAB кнопка + (поверх всего)
    m_fabBtn = new QPushButton("+", m_dashPage);
    m_fabBtn->setObjectName("fabBtn");
    m_fabBtn->setFixedSize(56, 56);
    connect(m_fabBtn, &QPushButton::clicked, this, &MainWindow::onAddTransaction);
}

// ──── Страница История ────
void MainWindow::setupHistory() {
    m_histPage = new QWidget();
    auto *lay = new QVBoxLayout(m_histPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(16);

    auto *header = new QLabel("История транзакций");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    // Кнопки управления
    auto *btnRow = new QHBoxLayout();
    auto *addBtn = new QPushButton("+ Добавить");
    addBtn->setObjectName("primaryBtn");
    addBtn->setFixedHeight(40);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddTransaction);

    m_deleteBtn = new QPushButton("🗑  Удалить");
    m_deleteBtn->setObjectName("dangerBtn");
    m_deleteBtn->setFixedHeight(40);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteTransaction);

    btnRow->addWidget(addBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addStretch();
    lay->addLayout(btnRow);

    // Таблица транзакций
    m_table = new QTableWidget(0, 5);
    m_table->setObjectName("transTable");
    m_table->setHorizontalHeaderLabels({"Тип", "Категория", "Сумма", "Комментарий", "Дата"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);

    lay->addWidget(m_table, 1);
}

// ─────────────────────────────────────────────
//  Обновление данных
// ─────────────────────────────────────────────

void MainWindow::refreshAll() {
    refreshBalance();
    refreshChart();
    refreshTable();
    refreshCategories();
}

// Обновляем карточки с балансом
void MainWindow::refreshBalance() {
    double bal = m_dm->balance();
    double inc = m_dm->totalIncome();
    double exp = m_dm->totalExpense();

    m_balanceLabel->setText(QString("%1 ₽").arg(bal, 0, 'f', 2));
    m_incomeLabel->setText(QString("%1 ₽").arg(inc, 0, 'f', 2));
    m_expenseLabel->setText(QString("%1 ₽").arg(exp, 0, 'f', 2));

    // Цвет баланса: зелёный если > 0, красный если < 0
    m_balanceLabel->setStyleSheet(
        bal >= 0 ? "color: #10b981; font-size: 24px; font-weight: 800;"
                 : "color: #ef4444; font-size: 24px; font-weight: 800;");
}

// Обновляем круговую диаграмму (donut)
void MainWindow::refreshChart() {
    // Убираем старые срезы
    m_pieSeries->clear();

    QMap<QString, double> data = (m_currentTab == 0)
        ? m_dm->expensesByCategory()
        : m_dm->incomeByCategory();

    double total = 0;
    for (auto v : data) total += v;

    if (total <= 0) {
        // Пустой срез-заглушка
        auto *slice = m_pieSeries->append("Нет данных", 1);
        slice->setColor(QColor("#252535"));
        slice->setLabelVisible(false);
        slice->setBorderColor(Qt::transparent);
        return;
    }

    // Добавляем срез для каждой категории
    for (auto it = data.begin(); it != data.end(); ++it) {
        auto *slice = m_pieSeries->append(it.key(), it.value());
        slice->setColor(DataManager::colorForCategory(it.key()));
        slice->setBorderColor(QColor("#0a0a0f"));
        slice->setBorderWidth(2);
        slice->setLabelVisible(false);
    }
}

// Список категорий под диаграммой
void MainWindow::refreshCategories() {
    // Чистим старые виджеты из списка
    QLayoutItem *child;
    while ((child = m_catListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    QMap<QString, double> data = (m_currentTab == 0)
        ? m_dm->expensesByCategory()
        : m_dm->incomeByCategory();

    double total = 0;
    for (auto v : data) total += v;

    if (total <= 0) {
        auto *empty = new QLabel("Нет данных");
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: #4a4a60; font-size: 13px;");
        m_catListLayout->addWidget(empty);
        m_catListLayout->addStretch();
        return;
    }

    // Сортируем по убыванию суммы
    QList<QPair<QString, double>> sorted;
    for (auto it = data.begin(); it != data.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(),
              [](auto &a, auto &b){ return a.second > b.second; });

    // Создаём строку для каждой категории
    for (auto &pair : sorted) {
        QString cat   = pair.first;
        double  val   = pair.second;
        double  pct   = (total > 0) ? (val / total * 100.0) : 0;

        auto *row = new QFrame();
        row->setObjectName("catRow");
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(12, 10, 12, 10);

        // Цветная точка
        auto *dot = new QLabel("●");
        dot->setStyleSheet(QString("color: %1; font-size: 16px;")
                           .arg(DataManager::colorForCategory(cat).name()));
        dot->setFixedWidth(20);

        // Иконка + название
        QString icon = "📦";
        auto cats = (m_currentTab == 0) ? DataManager::expenseCategories() : DataManager::incomeCategories();
        for (auto &c : cats)
            if (c.name == cat) { icon = c.icon; break; }

        auto *nameLabel = new QLabel(icon + "  " + cat);
        nameLabel->setStyleSheet("color: #c8c8e0; font-size: 13px;");

        auto *pctLabel = new QLabel(QString("%1%").arg(pct, 0, 'f', 0));
        pctLabel->setStyleSheet("color: #6b6b85; font-size: 13px;");

        auto *valLabel = new QLabel(QString("%1 ₽").arg(val, 0, 'f', 2));
        valLabel->setStyleSheet("color: #f0f0f8; font-size: 13px; font-weight: 600;");
        valLabel->setAlignment(Qt::AlignRight);

        rl->addWidget(dot);
        rl->addWidget(nameLabel, 1);
        rl->addWidget(pctLabel);
        rl->addSpacing(8);
        rl->addWidget(valLabel);

        m_catListLayout->addWidget(row);
    }
    m_catListLayout->addStretch();
}

// Обновляем таблицу истории
void MainWindow::refreshTable() {
    const auto &list = m_dm->transactions();
    m_table->setRowCount(list.size());

    for (int i = 0; i < list.size(); ++i) {
        const Transaction &t = list[i];

        // Иконка типа
        auto *typeItem = new QTableWidgetItem(t.type == "expense" ? "🔴 Расход" : "🟢 Доход");
        typeItem->setData(Qt::UserRole, t.id); // храним id для удаления

        auto *catItem  = new QTableWidgetItem(t.category);
        auto *amtItem  = new QTableWidgetItem(QString("₽ %1").arg(t.amount, 0, 'f', 2));
        auto *comItem  = new QTableWidgetItem(t.comment);
        auto *dateItem = new QTableWidgetItem(t.date.toString("dd.MM.yyyy hh:mm"));

        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        // Цвет суммы
        amtItem->setForeground(t.type == "expense" ? QColor("#ff6b8a") : QColor("#2dd4a0"));

        m_table->setItem(i, 0, typeItem);
        m_table->setItem(i, 1, catItem);
        m_table->setItem(i, 2, amtItem);
        m_table->setItem(i, 3, comItem);
        m_table->setItem(i, 4, dateItem);
    }
}

// ─────────────────────────────────────────────
//  Слоты действий
// ─────────────────────────────────────────────

void MainWindow::onAddTransaction() {
    AddTransactionDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        m_dm->addTransaction(dlg.result());
        refreshAll(); // обновляем всё после добавления
    }
}

void MainWindow::onDeleteTransaction() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Удаление", "Выберите строку для удаления");
        return;
    }
    auto *item = m_table->item(row, 0);
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    auto ret = QMessageBox::question(this, "Удалить?",
        "Удалить выбранную транзакцию?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_dm->removeTransaction(id);
        refreshAll();
    }
}

// ─────────────────────────────────────────────
//  Тёмная тема (CSS)
// ─────────────────────────────────────────────
void MainWindow::applyStyle() {
    QApplication::setStyle("Fusion");

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #0f111a;
            color: #f8fafc;
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
        }

        /* ── Sidebar ── */
        #sidebar {
            background: #151722;
            border-right: 1px solid rgba(255,255,255,0.04);
        }
        #sidebarLogo {
            font-size: 22px;
            font-weight: 800;
            color: #6366f1;
            padding: 10px 12px;
            letter-spacing: -0.5px;
        }
        #sidebarSep { color: rgba(255,255,255,0.05); }
        #userLabel  { color: #94a3b8; font-size: 13px; padding: 8px 12px; font-weight: 500; }

        /* Nav buttons */
        #navBtn {
            background: transparent;
            border: none;
            border-radius: 12px;
            padding: 14px 18px;
            text-align: left;
            font-size: 14px;
            color: #94a3b8;
            font-weight: 600;
            margin: 2px 0;
        }
        #navBtn:hover   { background: rgba(99, 102, 241, 0.08); color: #c7d2fe; }
        #navBtn:checked { background: #6366f1; color: white; }

        /* ── Page title ── */
        #pageTitle {
            font-size: 28px;
            font-weight: 800;
            color: #f8fafc;
            padding-bottom: 8px;
            letter-spacing: -0.5px;
        }

        /* ── Cards ── */
        #balanceCard, #incomeCard, #expenseCard {
            background: #1a1c29;
            border-radius: 20px;
            border: 1px solid rgba(255,255,255,0.05);
            min-height: 100px;
        }
        #cardTitle { font-size: 13px; color: #94a3b8; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; }
        #cardValue { font-size: 24px; font-weight: 800; color: #f8fafc; }
        #incomeCard  #cardValue { color: #10b981; }
        #expenseCard #cardValue { color: #ef4444; }

        /* ── Chart card ── */
        #chartCard {
            background: #1a1c29;
            border-radius: 24px;
            border: 1px solid rgba(255,255,255,0.05);
        }
        #chartView { background: transparent; border: none; }

        /* Tab buttons */
        #tabBtnActive {
            background: transparent;
            border: none;
            border-bottom: 3px solid #6366f1;
            border-radius: 0;
            color: #6366f1;
            font-size: 13px;
            font-weight: 800;
            padding: 12px 24px;
            letter-spacing: 1.2px;
        }
        #tabBtnInactive {
            background: transparent;
            border: none;
            border-bottom: 3px solid transparent;
            border-radius: 0;
            color: #475569;
            font-size: 13px;
            font-weight: 700;
            padding: 12px 24px;
            letter-spacing: 1.2px;
        }
        #tabBtnInactive:hover { color: #94a3b8; border-bottom-color: rgba(99, 102, 241, 0.2); }

        /* ── Category card ── */
        #catCard {
            background: #1a1c29;
            border-radius: 24px;
            border: 1px solid rgba(255,255,255,0.05);
        }
        #catTitle  { font-size: 18px; font-weight: 700; color: #f8fafc; margin-bottom: 4px; }
        #catScroll { background: transparent; border: none; }
        #catRow {
            background: #232533;
            border-radius: 14px;
            border: 1px solid rgba(255,255,255,0.03);
        }
        #catRow:hover { background: #2a2c3d; border-color: rgba(99, 102, 241, 0.3); }

        /* ── Table ── */
        #transTable {
            background: #1a1c29;
            alternate-background-color: #1d1f2e;
            border: 1px solid rgba(255,255,255,0.05);
            border-radius: 18px;
            gridline-color: transparent;
            color: #e2e8f0;
            font-size: 13px;
        }
        #transTable::item { padding: 12px 16px; border: none; }
        #transTable::item:selected {
            background: rgba(99, 102, 241, 0.15);
            color: #f8fafc;
        }
        QHeaderView::section {
            background: #151722;
            color: #64748b;
            font-size: 12px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.8px;
            padding: 14px 16px;
            border: none;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        QScrollBar:vertical {
            background: transparent; width: 8px; margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #334155; border-radius: 4px; min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #475569; }

        /* ── Buttons ── */
        #primaryBtn {
            background: #6366f1;
            color: white; border: none; border-radius: 12px;
            padding: 10px 24px; font-size: 14px; font-weight: 700;
        }
        #primaryBtn:hover { background: #4f46e5; }
        #primaryBtn:pressed { background: #4338ca; }

        #dangerBtn {
            background: rgba(239, 68, 68, 0.1);
            color: #f87171; border: 1px solid rgba(239, 68, 68, 0.2);
            border-radius: 12px; padding: 10px 24px;
            font-size: 14px; font-weight: 700;
        }
        #dangerBtn:hover { background: rgba(239, 68, 68, 0.2); }

        /* ── FAB ── */
        #fabBtn {
            background: #6366f1;
            color: white;
            font-size: 24px;
            font-weight: 400;
            border: none;
            border-radius: 28px;
        }
        #fabBtn:hover {
            background: #4f46e5;
            transform: scale(1.1);
        }

        QMessageBox {
            background: #1a1c29;
            border: 1px solid rgba(255,255,255,0.1);
        }
        QMessageBox QLabel { color: #f8fafc; font-size: 14px; }
        QMessageBox QPushButton {
            background: #232533;
            color: white;
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 10px;
            padding: 8px 24px;
            min-width: 80px;
            font-weight: 600;
        }
        QMessageBox QPushButton:hover { background: #2d3045; }
    )");
}

// Позиционируем FAB при изменении размера окна
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_fabBtn && m_dashPage) {
        int x = m_dashPage->width() - m_fabBtn->width() - 32;
        int y = m_dashPage->height() - m_fabBtn->height() - 32;
        m_fabBtn->move(x, y);
        m_fabBtn->raise();
    }
}

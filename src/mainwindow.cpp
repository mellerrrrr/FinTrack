#include "mainwindow.h"
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
#include <QInputDialog>
#include <QProgressBar>
#include <QTextEdit>
#include <QCalendarWidget>
#include <QNetworkRequest>
#include <QUrl>
#include <algorithm>

#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

MainWindow::MainWindow(DataManager *dm, const QString &username, QWidget *parent)
    : QMainWindow(parent), m_dm(dm), m_username(username)
{
    m_netManager = new QNetworkAccessManager(this);
    connect(m_netManager, &QNetworkAccessManager::finished, this, &MainWindow::onRatesReceived);
    setWindowTitle("FinTrack — " + username);
    setMinimumSize(960, 660);
    resize(1100, 720);

    setupUi();
    applyStyle();
    refreshAll();
}

void MainWindow::setupUi() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_rootLayout = new QHBoxLayout(m_centralWidget);
    m_rootLayout->setContentsMargins(0, 0, 0, 0);
    m_rootLayout->setSpacing(0);

    setupSidebar();

    m_pages = new QStackedWidget();
    setupDashboard();
    setupHistory();
    setupRates();
    setupCharts();
    setupTips();

    m_pages->addWidget(m_dashPage);
    m_pages->addWidget(m_histPage);
    m_pages->addWidget(m_ratesPage);
    m_pages->addWidget(m_chartsPage);
    m_pages->addWidget(m_tipsPage);

    m_rootLayout->addWidget(m_sidebar);
    m_rootLayout->addWidget(m_pages, 1);
}

void MainWindow::setupSidebar() {
    m_sidebar = new QFrame();
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(220);

    auto *lay = new QVBoxLayout(m_sidebar);
    lay->setContentsMargins(16, 32, 16, 24);
    lay->setSpacing(4);

    auto *logo = new QLabel("💰 FinTrack");
    logo->setObjectName("sidebarLogo");
    lay->addWidget(logo);
    lay->addSpacing(32);

    m_navDashboard = new QPushButton("  📊  Дашборд");
    m_navDashboard->setObjectName("navBtn");
    m_navDashboard->setCheckable(true);
    m_navDashboard->setChecked(true);

    m_navHistory = new QPushButton("  📋  История");
    m_navHistory->setObjectName("navBtn");
    m_navHistory->setCheckable(true);

    m_navRates = new QPushButton("  💵  Курсы валют");
    m_navRates->setObjectName("navBtn");
    m_navRates->setCheckable(true);

    m_navCharts = new QPushButton("  📈  Графики");
    m_navCharts->setObjectName("navBtn");
    m_navCharts->setCheckable(true);

    m_navTips = new QPushButton("  💡  Советы");
    m_navTips->setObjectName("navBtn");
    m_navTips->setCheckable(true);

    auto resetNav = [this](){
        m_navDashboard->setChecked(false);
        m_navHistory->setChecked(false);
        m_navRates->setChecked(false);
        m_navCharts->setChecked(false);
        m_navTips->setChecked(false);
    };

    connect(m_navDashboard, &QPushButton::clicked, this, [=](){
        resetNav();
        m_pages->setCurrentIndex(0);
        m_navDashboard->setChecked(true);
        refreshAll();
    });
    connect(m_navHistory, &QPushButton::clicked, this, [=](){
        resetNav();
        m_pages->setCurrentIndex(1);
        m_navHistory->setChecked(true);
        refreshAll();
    });
    connect(m_navRates, &QPushButton::clicked, this, [=](){
        resetNav();
        m_pages->setCurrentIndex(2);
        m_navRates->setChecked(true);
        refreshAll();
    });
    connect(m_navCharts, &QPushButton::clicked, this, [=](){
        resetNav();
        m_pages->setCurrentIndex(3);
        m_navCharts->setChecked(true);
        refreshAll();
    });
    connect(m_navTips, &QPushButton::clicked, this, [=](){
        resetNav();
        m_pages->setCurrentIndex(4);
        m_navTips->setChecked(true);
        refreshAll();
    });

    auto *navContainer = new QWidget();
    auto *navLay = new QVBoxLayout(navContainer);
    navLay->setContentsMargins(0, 0, 0, 0);
    navLay->setSpacing(2);
    
    navLay->addWidget(m_navDashboard);
    navLay->addWidget(m_navHistory);
    navLay->addWidget(m_navRates);
    navLay->addWidget(m_navCharts);
    navLay->addWidget(m_navTips);
    navLay->addStretch();
    
    lay->addWidget(navContainer, 1);

    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("sidebarSep");
    lay->addWidget(sep);
    lay->addSpacing(12);

    m_userLabel = new QLabel("👤 " + m_username);
    m_userLabel->setObjectName("userLabel");
    lay->addWidget(m_userLabel);

    m_logoutBtn = new QPushButton("  🚪  Выйти");
    m_logoutBtn->setObjectName("logoutBtn");
    connect(m_logoutBtn, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    lay->addWidget(m_logoutBtn);
}

void MainWindow::setupDashboard() {
    m_dashPage = new QWidget();
    auto *lay = new QVBoxLayout(m_dashPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(20);

    auto *headerRow = new QHBoxLayout();
    auto *header = new QLabel("Обзор финансов");
    header->setObjectName("pageTitle");
    headerRow->addWidget(header);
    headerRow->addStretch();

    m_globalRangeCombo = new QComboBox();
    m_globalRangeCombo->addItems({"Все время", "Год", "Месяц", "Неделя"});
    m_globalRangeCombo->setObjectName("timeCombo");
    connect(m_globalRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
        if(idx == 0) m_currentRange = "all";
        else if(idx == 1) m_currentRange = "year";
        else if(idx == 2) m_currentRange = "month";
        else if(idx == 3) m_currentRange = "week";
        refreshAll();
    });
    headerRow->addWidget(m_globalRangeCombo);
    lay->addLayout(headerRow);

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

    auto *balCard = makeCard("Баланс", &m_balanceLabel, "balanceCard");
    auto *incCard = makeCard("Доходы", &m_incomeLabel, "incomeCard");
    auto *expCard = makeCard("Расходы", &m_expenseLabel, "expenseCard");

    cardsRow->addWidget(balCard, 1);
    cardsRow->addWidget(incCard, 1);
    cardsRow->addWidget(expCard, 1);
    lay->addLayout(cardsRow);

    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(20);

    auto *chartCard = new QFrame();
    chartCard->setObjectName("chartCard");
    auto *chartLay = new QVBoxLayout(chartCard);
    chartLay->setContentsMargins(24, 20, 24, 20);
    chartLay->setSpacing(16);

    auto *tabRow = new QHBoxLayout();
    tabRow->setSpacing(0);
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

    m_pieSeries = new QPieSeries();
    m_pieSeries->setHoleSize(0.6);
    m_pieSeries->setPieSize(0.75);

    auto *chart = new QChart();
    chart->addSeries(m_pieSeries);
    chart->setBackgroundBrush(Qt::transparent);
    chart->setMargins(QMargins(0,0,0,0));
    chart->legend()->hide();

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);
    m_chartView->setMinimumHeight(260);

    chartLay->addWidget(m_chartView);
    bottomRow->addWidget(chartCard, 1);

    auto *catCard = new QFrame();
    catCard->setObjectName("catCard");
    auto *catOuterLay = new QVBoxLayout(catCard);
    catOuterLay->setContentsMargins(20, 20, 20, 20);
    catOuterLay->setSpacing(12);

    auto *catTitle = new QLabel("По категориям");
    catTitle->setObjectName("catTitle");
    catOuterLay->addWidget(catTitle);

    auto *scroll = new QScrollArea();
    scroll->setObjectName("catScroll");
    scroll->setWidgetResizable(true);
    auto *catInner = new QWidget();
    m_catListLayout = new QVBoxLayout(catInner);
    m_catListLayout->setContentsMargins(0,0,0,0);
    scroll->setWidget(catInner);
    catOuterLay->addWidget(scroll, 1);

    bottomRow->addWidget(catCard, 1);
    lay->addLayout(bottomRow, 1);
}

void MainWindow::setupHistory() {
    m_histPage = new QWidget();
    auto *lay = new QVBoxLayout(m_histPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(16);

    auto *header = new QLabel("История транзакций");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("🔍 Поиск по категориям или комментариям...");
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setFixedHeight(45);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearch);
    lay->addWidget(m_searchEdit);

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

    m_table = new QTableWidget(0, 5);
    m_table->setObjectName("transTable");
    m_table->setHorizontalHeaderLabels({"Тип", "Категория", "Сумма", "Комментарий", "Дата"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);

    lay->addWidget(m_table, 1);
}

void MainWindow::setupRates() {
    m_ratesPage = new QWidget();
    auto *lay = new QVBoxLayout(m_ratesPage);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(24);

    auto *header = new QLabel("Курсы валют");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    auto *convCard = new QFrame();
    convCard->setObjectName("chartCard");
    auto *convLay = new QVBoxLayout(convCard);
    
    auto *convTitle = new QLabel("Калькулятор");
    convTitle->setStyleSheet("color: #f8fafc; font-size: 18px; font-weight: 700;");
    convLay->addWidget(convTitle);

    auto *inputRow = new QHBoxLayout();
    m_convAmount = new QLineEdit("100");
    m_convAmount->setObjectName("timeCombo"); 
    m_convFrom = new QComboBox();
    m_convFrom->addItems({"BYN", "USD", "EUR", "RUB", "CNY"});
    m_convFrom->setObjectName("timeCombo");
    m_convTo = new QComboBox();
    m_convTo->addItems({"USD", "BYN", "EUR", "RUB", "CNY"});
    m_convTo->setObjectName("timeCombo");

    inputRow->addWidget(m_convAmount, 2);
    inputRow->addWidget(m_convFrom, 1);
    inputRow->addWidget(new QLabel(" -> "));
    inputRow->addWidget(m_convTo, 1);
    convLay->addLayout(inputRow);

    m_convResult = new QLabel("Результат: ---");
    m_convResult->setStyleSheet("color: #10b981; font-size: 20px; font-weight: 800;");
    convLay->addWidget(m_convResult);

    connect(m_convAmount, &QLineEdit::textChanged, this, &MainWindow::onConvert);
    connect(m_convFrom, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onConvert);
    connect(m_convTo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onConvert);

    lay->addWidget(convCard);

    m_ratesTable = new QTableWidget(0, 3);
    m_ratesTable->setObjectName("transTable");
    m_ratesTable->setHorizontalHeaderLabels({"Валюта", "Покупка", "Продажа"});
    m_ratesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    lay->addWidget(m_ratesTable, 1);
}

void MainWindow::setupCharts() {
    m_chartsPage = new QWidget();
    auto *lay = new QVBoxLayout(m_chartsPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(24);

    auto *header = new QLabel("Аналитика");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("background: transparent; border: none;");
    auto *scrollContent = new QWidget();
    auto *contentLay = new QVBoxLayout(scrollContent);
    contentLay->setSpacing(30);

    auto makeChartCard = [&](const QString &title, QChartView **view) {
        auto *card = new QFrame();
        card->setObjectName("chartCard");
        card->setMinimumHeight(400);
        auto *cl = new QVBoxLayout(card);
        auto *tl = new QLabel(title);
        tl->setStyleSheet("color: #f8fafc; font-size: 18px; font-weight: 700;");
        cl->addWidget(tl);

        auto *chart = new QChart();
        chart->setBackgroundBrush(Qt::transparent);
        chart->setMargins(QMargins(0,0,0,0));
        *view = new QChartView(chart);
        (*view)->setRenderHint(QPainter::Antialiasing);
        (*view)->setBackgroundBrush(Qt::transparent);
        cl->addWidget(*view, 1);
        return card;
    };

    m_expCatBarsWidget = makeChartCard("Расходы по категориям", &m_barChartView);
    m_incCatBarsWidget = makeChartCard("Доходы по категориям", &m_incomeBarChartView);
    contentLay->addWidget(m_expCatBarsWidget);
    contentLay->addWidget(m_incCatBarsWidget);
    contentLay->addWidget(makeChartCard("Динамика баланса", &m_lineChartView));

    scroll->setWidget(scrollContent);
    lay->addWidget(scroll);
}

void MainWindow::setupTips() {
    m_tipsPage = new QWidget();
    auto *lay = new QVBoxLayout(m_tipsPage);
    lay->setContentsMargins(32, 32, 32, 32);
    
    auto *header = new QLabel("Финансовые советы");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    auto *tipsArea = new QTextEdit();
    tipsArea->setReadOnly(true);
    tipsArea->setObjectName("chartCard");
    tipsArea->setHtml(R"(
        <h2 style='color: #7c6fff;'>5 золотых правил экономии</h2>
        <ol>
            <li><b>Правило 50/30/20:</b> Тратьте 50% на нужды, 30% на желания и 20% откладывайте.</li>
            <li><b>Сначала заплати себе:</b> Как только получили зарплату, сразу переведите 10% на сберегательный счет.</li>
            <li><b>Избегайте импульсивных покупок:</b> Подождите 24 часа перед тем, как купить дорогую вещь.</li>
            <li><b>Ведите учет:</b> Используя FinTrack, вы уже на шаг впереди!</li>
        </ol>
    )");
    lay->addWidget(tipsArea);
}

void MainWindow::refreshAll() {
    refreshBalance();
    refreshChart();
    refreshCategories();
    refreshTable();
    refreshRates();
    refreshCharts();
}

void MainWindow::refreshBalance() {
    double bal = m_dm->balance();
    double inc = m_dm->totalIncome();
    double exp = m_dm->totalExpense();

    m_balanceLabel->setText(QString("%1 ₽").arg(bal, 0, 'f', 2));
    m_incomeLabel->setText(QString("%1 ₽").arg(inc, 0, 'f', 2));
    m_expenseLabel->setText(QString("%1 ₽").arg(exp, 0, 'f', 2));

    m_balanceLabel->setStyleSheet(bal >= 0 ? "color: #10b981; font-size: 24px; font-weight: 800;" : "color: #ef4444; font-size: 24px; font-weight: 800;");
}

void MainWindow::refreshChart() {
    m_pieSeries->clear();
    QMap<QString, double> data = (m_currentTab == 0) ? m_dm->expensesByCategory(m_currentRange) : m_dm->incomeByCategory(m_currentRange);
    double total = 0;
    for (auto v : data) total += v;

    if (total <= 0) {
        auto *slice = m_pieSeries->append("Нет данных", 1);
        slice->setColor(QColor("#232533"));
        slice->setLabelVisible(false);
        slice->setBorderColor(Qt::transparent);
        return;
    }

    for (auto it = data.begin(); it != data.end(); ++it) {
        double val = it.value();
        double pct = (val / total) * 100.0;
        QString cat = it.key();

        auto *slice = m_pieSeries->append(cat, val);
        slice->setColor(DataManager::colorForCategory(cat));
        slice->setBorderColor(QColor("#1a1c29"));
        slice->setBorderWidth(2);
        
        slice->setLabel(QString("%1 %2%").arg(DataManager::iconForCategory(cat)).arg(pct, 0, 'f', 0));
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelColor(QColor("#94a3b8"));
    }
}

void MainWindow::refreshCategories() {
    QLayoutItem *child;
    while ((child = m_catListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    QMap<QString, double> data = (m_currentTab == 0) ? m_dm->expensesByCategory(m_currentRange) : m_dm->incomeByCategory(m_currentRange);
    double total = 0;
    for (auto v : data) total += v;

    if (total <= 0) {
        auto *empty = new QLabel("Нет данных");
        empty->setStyleSheet("color: #4a4a60; font-size: 13px;");
        m_catListLayout->addWidget(empty);
        m_catListLayout->addStretch();
        return;
    }

    QList<QPair<QString, double>> sorted;
    for (auto it = data.begin(); it != data.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](auto &a, auto &b){ return a.second > b.second; });

    for (auto &pair : sorted) {
        QString cat = pair.first;
        double  val = pair.second;
        double  pct = (total > 0) ? (val / total * 100.0) : 0;

        auto *row = new QFrame();
        row->setObjectName("catRow");
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(12, 10, 12, 10);

        auto *dot = new QLabel("●");
        dot->setStyleSheet(QString("color: %1; font-size: 16px;").arg(DataManager::colorForCategory(cat).name()));

        auto *nameLabel = new QLabel(DataManager::iconForCategory(cat) + "  " + cat);
        nameLabel->setStyleSheet("color: #c8c8e0; font-size: 13px;");

        auto *pctLabel = new QLabel(QString("%1%").arg(pct, 0, 'f', 0));
        pctLabel->setStyleSheet("color: #6b6b85; font-size: 13px;");

        auto *valLabel = new QLabel(QString("%1 ₽").arg(val, 0, 'f', 2));
        valLabel->setStyleSheet("color: #f0f0f8; font-size: 13px; font-weight: 600;");

        rl->addWidget(dot);
        rl->addWidget(nameLabel, 1);
        rl->addWidget(pctLabel);
        rl->addSpacing(8);
        rl->addWidget(valLabel);
        m_catListLayout->addWidget(row);
    }
    m_catListLayout->addStretch();
}

void MainWindow::refreshTable() {
    const auto &list = m_dm->transactions();
    m_table->setRowCount(list.size());

    for (int i = 0; i < list.size(); ++i) {
        const Transaction &t = list[i];
        auto *typeItem = new QTableWidgetItem(t.type == "expense" ? "🔴 Расход" : "🟢 Доход");
        typeItem->setData(Qt::UserRole, t.id);

        auto *catItem = new QTableWidgetItem(t.category);
        auto *amtItem = new QTableWidgetItem(QString("₽ %1").arg(t.amount, 0, 'f', 2));
        auto *comItem = new QTableWidgetItem(t.comment);
        auto *dateItem = new QTableWidgetItem(t.date.toString("dd.MM.yyyy hh:mm"));

        amtItem->setForeground(t.type == "expense" ? QColor("#ff6b8a") : QColor("#2dd4a0"));
        m_table->setItem(i, 0, typeItem);
        m_table->setItem(i, 1, catItem);
        m_table->setItem(i, 2, amtItem);
        m_table->setItem(i, 3, comItem);
        m_table->setItem(i, 4, dateItem);
    }
}

void MainWindow::refreshRates() {
    if (m_pages->currentIndex() == 2) {
        m_netManager->get(QNetworkRequest(QUrl("https://belarusbank.by/api/kursExchange?city=Минск")));
    }
}

void MainWindow::refreshCharts() {
    if (m_pages->currentIndex() != 3) return;

    auto updateBar = [&](QChartView *view, bool isExpense) {
        auto *chart = view->chart();
        chart->removeAllSeries();
        for(auto *axis : chart->axes()) chart->removeAxis(axis);

        auto *barSeries = new QBarSeries();
        auto *set = new QBarSet(isExpense ? "Расходы" : "Доходы");
        set->setColor(isExpense ? QColor("#6366f1") : QColor("#10b981"));

        auto data = isExpense ? m_dm->expensesByCategory(m_currentRange) : m_dm->incomeByCategory(m_currentRange);
        QStringList categories;
        for (auto it = data.begin(); it != data.end(); ++it) {
            categories << it.key();
            *set << it.value();
        }
        barSeries->append(set);
        chart->addSeries(barSeries);

        auto *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setLabelsColor(QColor("#94a3b8"));
        chart->addAxis(axisX, Qt::AlignBottom);
        barSeries->attachAxis(axisX);

        auto *axisY = new QValueAxis();
        axisY->setLabelsColor(QColor("#94a3b8"));
        chart->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);
        chart->legend()->hide();
    };

    updateBar(m_barChartView, true);
    updateBar(m_incomeBarChartView, false);

    auto *lineChart = m_lineChartView->chart();
    lineChart->removeAllSeries();
    for(auto *axis : lineChart->axes()) lineChart->removeAxis(axis);

    auto *incSeries = new QLineSeries();
    incSeries->setName("Доходы");
    incSeries->setColor(QColor("#10b981"));
    auto *expSeries = new QLineSeries();
    expSeries->setName("Расходы");
    expSeries->setColor(QColor("#ef4444"));

    QDate today = QDate::currentDate();
    QStringList months;
    for (int i = 5; i >= 0; --i) {
        QDate d = today.addMonths(-i);
        months << d.toString("MMM");
        double incSum = 0, expSum = 0;
        for (const auto &t : m_dm->transactions()) {
            if (t.date.date().month() == d.month() && t.date.date().year() == d.year()) {
                if (t.type == "income") incSum += t.amount;
                else expSum += t.amount;
            }
        }
        incSeries->append(5 - i, incSum);
        expSeries->append(5 - i, expSum);
    }
    lineChart->addSeries(incSeries);
    lineChart->addSeries(expSeries);

    auto *lAxisX = new QBarCategoryAxis();
    lAxisX->append(months);
    lAxisX->setLabelsColor(QColor("#94a3b8"));
    lineChart->addAxis(lAxisX, Qt::AlignBottom);
    incSeries->attachAxis(lAxisX);
    expSeries->attachAxis(lAxisX);

    auto *lAxisY = new QValueAxis();
    lAxisY->setLabelsColor(QColor("#94a3b8"));
    lineChart->addAxis(lAxisY, Qt::AlignLeft);
    incSeries->attachAxis(lAxisY);
    expSeries->attachAxis(lAxisY);
    lineChart->legend()->setVisible(true);
    lineChart->legend()->setLabelColor(QColor("#f8fafc"));
}

void MainWindow::refreshCatBars(QWidget *container, bool isExpense) {} // Dummy for compat

void MainWindow::onRatesReceived(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isArray() && !doc.array().isEmpty()) {
            QJsonObject obj = doc.array().first().toObject();
            m_ratesTable->setRowCount(0);
            m_currentRates.clear();
            m_currentRates["BYN"] = 1.0;

            auto addRate = [&](const QString &code, const QString &inK, const QString &outK, double mult = 1.0) {
                int r = m_ratesTable->rowCount();
                m_ratesTable->insertRow(r);
                QString valStr = obj.value(inK).toString();
                m_currentRates[code] = valStr.toDouble() / mult;
                m_ratesTable->setItem(r, 0, new QTableWidgetItem(code));
                m_ratesTable->setItem(r, 1, new QTableWidgetItem(valStr + " BYN"));
                m_ratesTable->setItem(r, 2, new QTableWidgetItem(obj.value(outK).toString() + " BYN"));
            };
            addRate("USD", "USD_in", "USD_out");
            addRate("EUR", "EUR_in", "EUR_out");
            addRate("RUB", "RUB_in", "RUB_out", 100.0);
            addRate("CNY", "CNY_in", "CNY_out", 10.0);
            onConvert();
        }
    }
    reply->deleteLater();
}

void MainWindow::onConvert() {
    if (m_currentRates.isEmpty()) return;
    double amount = m_convAmount->text().replace(",", ".").toDouble();
    QString from = m_convFrom->currentText();
    QString to = m_convTo->currentText();
    if (m_currentRates.contains(from) && m_currentRates.contains(to)) {
        double result = (amount * m_currentRates[from]) / m_currentRates[to];
        m_convResult->setText(QString("Результат: %1 %2").arg(result, 0, 'f', 2).arg(to));
    }
}

void MainWindow::onSearch(const QString &text) {
    QString query = text.toLower();
    for (int i = 0; i < m_table->rowCount(); ++i) {
        bool match = m_table->item(i, 1)->text().toLower().contains(query) || m_table->item(i, 3)->text().toLower().contains(query);
        m_table->setRowHidden(i, !match);
    }
}

void MainWindow::onAddTransaction() {
    bool ok;
    double amount = QInputDialog::getDouble(this, "Транзакция", "Сумма:", 100, 0, 1000000, 2, &ok);
    if (ok) {
        QStringList cats;
        for(auto& c : DataManager::expenseCategories()) cats << c.name;
        QString cat = QInputDialog::getItem(this, "Категория", "Выберите категорию:", cats, 0, false, &ok);
        if (ok) {
            Transaction t;
            t.type = "expense";
            t.category = cat;
            t.amount = amount;
            t.comment = "Добавлено вручную";
            t.date = QDateTime::currentDateTime();
            m_dm->addTransaction(t);
            refreshAll();
        }
    }
}

void MainWindow::onDeleteTransaction() {
    int row = m_table->currentRow();
    if (row >= 0 && QMessageBox::question(this, "Удалить?", "Удалить транзакцию?") == QMessageBox::Yes) {
        m_dm->removeTransaction(m_table->item(row, 0)->data(Qt::UserRole).toInt());
        refreshAll();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::applyStyle() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #0f111a; color: #f8fafc; font-family: 'Inter'; }
        #sidebar { background: #151722; border-right: 1px solid rgba(255,255,255,0.04); }
        #sidebarLogo { font-size: 22px; font-weight: 800; color: #6366f1; padding: 10px; }
        #userLabel { color: #94a3b8; font-size: 13px; padding: 8px; font-weight: 500; }
        #navBtn { background: transparent; border: none; border-radius: 12px; padding: 14px; text-align: left; font-size: 14px; color: #94a3b8; font-weight: 600; }
        #navBtn:hover { background: rgba(99, 102, 241, 0.08); color: #c7d2fe; }
        #navBtn:checked { background: #6366f1; color: white; }
        #logoutBtn { background: transparent; border: 1px solid rgba(239, 68, 68, 0.2); border-radius: 10px; padding: 10px; text-align: left; font-size: 13px; color: #f87171; font-weight: 600; }
        #logoutBtn:hover { background: rgba(239, 68, 68, 0.1); }
        #pageTitle { font-size: 28px; font-weight: 800; }
        #balanceCard, #incomeCard, #expenseCard, #chartCard, #catCard { background: #1a1c29; border-radius: 20px; border: 1px solid rgba(255,255,255,0.05); }
        #cardTitle { font-size: 13px; color: #94a3b8; font-weight: 600; text-transform: uppercase; }
        #incomeCard #cardValue { color: #10b981; }
        #expenseCard #cardValue { color: #ef4444; }
        #tabBtnActive { background: transparent; border: none; border-bottom: 3px solid #6366f1; color: #6366f1; font-size: 13px; font-weight: 800; padding: 12px; }
        #tabBtnInactive { background: transparent; border: none; border-bottom: 3px solid transparent; color: #475569; font-size: 13px; font-weight: 700; padding: 12px; }
        #timeCombo, #searchEdit { background: #232533; border: 1px solid rgba(255,255,255,0.05); border-radius: 10px; padding: 6px 12px; color: #f8fafc; font-size: 13px; }
        #catRow { background: #232533; border-radius: 14px; border: 1px solid rgba(255,255,255,0.03); }
        #catTitle { font-size: 18px; font-weight: 700; color: #f8fafc; }
        #transTable { background: #1a1c29; alternate-background-color: #1d1f2e; border: 1px solid rgba(255,255,255,0.05); border-radius: 18px; gridline-color: transparent; font-size: 13px; }
        #transTable::item { padding: 12px 16px; border: none; }
        #primaryBtn { background: #6366f1; color: white; border: none; border-radius: 12px; padding: 10px 24px; font-size: 14px; font-weight: 700; }
        #dangerBtn { background: rgba(239, 68, 68, 0.1); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.2); border-radius: 12px; padding: 10px 24px; font-size: 14px; font-weight: 700; }
        QHeaderView::section { background: #151722; color: #64748b; font-size: 12px; font-weight: 700; text-transform: uppercase; padding: 14px; border: none; border-bottom: 1px solid rgba(255,255,255,0.05); }
    )");
}

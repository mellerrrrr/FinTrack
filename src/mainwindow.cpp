#include "mainwindow.h"
#include "addtransactiondialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
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
    setupLimits();

    m_pages->addWidget(m_dashPage);
    m_pages->addWidget(m_histPage);
    m_pages->addWidget(m_ratesPage);
    m_pages->addWidget(m_chartsPage);
    m_pages->addWidget(m_tipsPage);
    m_pages->addWidget(m_limitsPage);

    if (m_dm->isAdmin()) {
        setupAdmin();
        m_pages->addWidget(m_adminPage);
    }

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
    lay->addSpacing(24);

    m_addTransBtnSidebar = new QPushButton("➕ Новая транзакция");
    m_addTransBtnSidebar->setObjectName("primaryBtn");
    connect(m_addTransBtnSidebar, &QPushButton::clicked, this, &MainWindow::onAddTransaction);
    lay->addWidget(m_addTransBtnSidebar);
    lay->addSpacing(24);

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

    m_navLimits = new QPushButton("  🎯  Лимиты");
    m_navLimits->setObjectName("navBtn");
    m_navLimits->setCheckable(true);

    if (m_dm->isAdmin()) {
        m_navAdmin = new QPushButton("  ⚙️  Админ-панель");
        m_navAdmin->setObjectName("navBtn");
        m_navAdmin->setCheckable(true);
    }

    auto resetNav = [this](){
        m_navDashboard->setChecked(false);
        m_navHistory->setChecked(false);
        m_navRates->setChecked(false);
        m_navCharts->setChecked(false);
        m_navTips->setChecked(false);
        m_navLimits->setChecked(false);
        if (m_navAdmin) m_navAdmin->setChecked(false);
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
    connect(m_navLimits, &QPushButton::clicked, this, [=](){
        resetNav();
        m_pages->setCurrentIndex(5);
        m_navLimits->setChecked(true);
        refreshAll();
    });
    if (m_navAdmin) {
        connect(m_navAdmin, &QPushButton::clicked, this, [=](){
            resetNav();
            m_pages->setCurrentWidget(m_adminPage);
            m_navAdmin->setChecked(true);
            refreshAll();
        });
    }

    auto *navContainer = new QWidget();
    auto *navLay = new QVBoxLayout(navContainer);
    navLay->setContentsMargins(0, 0, 0, 0);
    navLay->setSpacing(2);
    
    navLay->addWidget(m_navDashboard);
    navLay->addWidget(m_navHistory);
    navLay->addWidget(m_navRates);
    navLay->addWidget(m_navCharts);
    navLay->addWidget(m_navTips);
    navLay->addWidget(m_navLimits);
    if (m_navAdmin) navLay->addWidget(m_navAdmin);
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
    connect(m_globalRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateRange);
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

    auto *limitCard = new QFrame();
    limitCard->setObjectName("chartCard");
    auto *limitLay = new QVBoxLayout(limitCard);
    auto *limitTitle = new QLabel("ЛИМИТ");
    limitTitle->setObjectName("cardTitle");
    m_dashLimitLabel = new QLabel("Нет лимита");
    m_dashLimitLabel->setStyleSheet("color: #f8fafc; font-size: 18px; font-weight: 800;");
    m_dashLimitProgress = new QProgressBar();
    m_dashLimitProgress->setFixedHeight(8);
    m_dashLimitProgress->setTextVisible(false);
    m_dashLimitProgress->setStyleSheet(R"(
        QProgressBar { background: rgba(255,255,255,0.05); border: none; border-radius: 4px; }
        QProgressBar::chunk { background: #6366f1; border-radius: 4px; }
    )");
    limitLay->addWidget(limitTitle);
    limitLay->addWidget(m_dashLimitLabel);
    limitLay->addWidget(m_dashLimitProgress);

    cardsRow->addWidget(balCard, 1);
    cardsRow->addWidget(incCard, 1);
    cardsRow->addWidget(expCard, 1);
    cardsRow->addWidget(limitCard, 1);
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

    auto *headerRow = new QHBoxLayout();
    auto *header = new QLabel("История транзакций");
    header->setObjectName("pageTitle");
    headerRow->addWidget(header);
    headerRow->addStretch();
    m_histRangeCombo = new QComboBox();
    m_histRangeCombo->addItems({"Все время", "Год", "Месяц", "Неделя"});
    m_histRangeCombo->setObjectName("timeCombo");
    connect(m_histRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateRange);
    headerRow->addWidget(m_histRangeCombo);
    lay->addLayout(headerRow);

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

    m_exportExcelBtn = new QPushButton("💾 Скачать в Excel");
    m_exportExcelBtn->setObjectName("primaryBtn");
    m_exportExcelBtn->setFixedHeight(40);
    connect(m_exportExcelBtn, &QPushButton::clicked, this, &MainWindow::onExportExcel);

    btnRow->addWidget(addBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addWidget(m_exportExcelBtn);
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
    m_table->setFocusPolicy(Qt::NoFocus);

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

    auto *headerRow = new QHBoxLayout();
    auto *header = new QLabel("Аналитика");
    header->setObjectName("pageTitle");
    headerRow->addWidget(header);
    headerRow->addStretch();
    m_chartsRangeCombo = new QComboBox();
    m_chartsRangeCombo->addItems({"Все время", "Год", "Месяц", "Неделя"});
    m_chartsRangeCombo->setObjectName("timeCombo");
    connect(m_chartsRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateRange);
    headerRow->addWidget(m_chartsRangeCombo);
    lay->addLayout(headerRow);

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

    auto makeCatBarsCard = [&](const QString &title, QWidget **container) {
        auto *card = new QFrame();
        card->setObjectName("chartCard");
        card->setMinimumHeight(300);
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(24, 24, 24, 24);
        
        auto *tl = new QLabel(title);
        tl->setStyleSheet("color: #f8fafc; font-size: 18px; font-weight: 700; margin-bottom: 15px;");
        cl->addWidget(tl);

        *container = new QWidget();
        auto *innerLay = new QVBoxLayout(*container);
        innerLay->setContentsMargins(0, 0, 0, 0);
        innerLay->setSpacing(12);
        innerLay->addStretch();
        
        cl->addWidget(*container, 1);
        return card;
    };

    m_expCatBarsWidget = makeCatBarsCard("Расходы по категориям", &m_expCatBarsWidget);
    m_incCatBarsWidget = makeCatBarsCard("Доходы по категориям", &m_incCatBarsWidget);
    contentLay->addWidget(m_expCatBarsWidget);
    contentLay->addWidget(m_incCatBarsWidget);
    contentLay->addWidget(makeChartCard("Ежедневная статистика", &m_dailyChartView));
    contentLay->addWidget(makeChartCard("Динамика баланса", &m_lineChartView));

    scroll->setWidget(scrollContent);
    lay->addWidget(scroll);
}

void MainWindow::setupTips() {
    m_tipsPage = new QWidget();
    auto *lay = new QVBoxLayout(m_tipsPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(16);
    
    auto *header = new QLabel("Финансовые советы");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    // Scroll Area for Tips
    m_tipsScrollArea = new QScrollArea();
    m_tipsScrollArea->setObjectName("catScroll");
    m_tipsScrollArea->setWidgetResizable(true);
    m_tipsScrollArea->setStyleSheet("background: transparent; border: none;");

    m_tipsScrollContainer = new QWidget();
    m_tipsScrollContainer->setStyleSheet("background: transparent;");
    m_tipsListLayout = new QVBoxLayout(m_tipsScrollContainer);
    m_tipsListLayout->setContentsMargins(0, 0, 0, 0);
    m_tipsListLayout->setSpacing(16);
    m_tipsScrollArea->setWidget(m_tipsScrollContainer);
    lay->addWidget(m_tipsScrollArea, 1);

    // Add Tip Form
    auto *addTipCard = new QFrame();
    addTipCard->setObjectName("chartCard");
    auto *addTipLay = new QVBoxLayout(addTipCard);
    addTipLay->setContentsMargins(20, 16, 20, 16);
    addTipLay->setSpacing(12);

    auto *addTipTitle = new QLabel("Поделиться советом");
    addTipTitle->setStyleSheet("color: #f8fafc; font-size: 14px; font-weight: 700; text-transform: uppercase;");
    addTipLay->addWidget(addTipTitle);

    m_newTipEdit = new QTextEdit();
    m_newTipEdit->setPlaceholderText("Напишите ваш финансовый совет здесь...");
    m_newTipEdit->setObjectName("tipInput");
    m_newTipEdit->setFixedHeight(80);
    m_newTipEdit->setStyleSheet(R"(
        #tipInput {
            background: #232533;
            border: 1px solid rgba(255,255,255,0.05);
            border-radius: 12px;
            padding: 12px;
            color: #f8fafc;
            font-size: 13px;
        }
        #tipInput:focus {
            border: 1px solid #6366f1;
        }
    )");
    addTipLay->addWidget(m_newTipEdit);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    m_addTipBtn = new QPushButton("Опубликовать совет");
    m_addTipBtn->setObjectName("primaryBtn");
    connect(m_addTipBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_newTipEdit->toPlainText().trimmed();
        if (text.isEmpty()) {
            QMessageBox::warning(this, "Внимание", "Текст совета не может быть пустым!");
            return;
        }
        m_dm->addTip(text);
        m_newTipEdit->clear();
        refreshAll();
    });
    btnRow->addWidget(m_addTipBtn);
    addTipLay->addLayout(btnRow);

    lay->addWidget(addTipCard);
}

void MainWindow::refreshTips() {
    if (!m_tipsListLayout) return;

    // Clear list
    QLayoutItem *child;
    while ((child = m_tipsListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    QList<Tip> tips = m_dm->getTips();

    if (tips.isEmpty()) {
        auto *emptyLabel = new QLabel("Пока нет советов. Будьте первыми, кто поделится полезным опытом!");
        emptyLabel->setStyleSheet("color: #64748b; font-size: 14px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_tipsListLayout->addWidget(emptyLabel);
    } else {
        for (const auto &tip : tips) {
            auto *card = new QFrame();
            card->setObjectName("chartCard");
            auto *vlay = new QVBoxLayout(card);
            vlay->setContentsMargins(20, 18, 20, 18);
            vlay->setSpacing(12);

            auto *textLabel = new QLabel(tip.text);
            textLabel->setWordWrap(true);
            textLabel->setStyleSheet("color: #f8fafc; font-size: 14px; font-weight: 500; line-height: 1.45;");
            vlay->addWidget(textLabel);

            auto *infoRow = new QHBoxLayout();
            auto *authorLabel = new QLabel(QString("👤 Автор: %1").arg(tip.username));
            authorLabel->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: 600;");

            QString ratingStr;
            if (tip.ratingCount > 0) {
                ratingStr = QString("⭐ %1 (%2)").arg(tip.averageRating, 0, 'f', 1).arg(tip.ratingCount);
            } else {
                ratingStr = "⭐ Нет оценок";
            }
            auto *ratingLabel = new QLabel(ratingStr);
            ratingLabel->setStyleSheet("color: #f59e0b; font-size: 12px; font-weight: 600;");

            infoRow->addWidget(authorLabel);
            infoRow->addWidget(ratingLabel);
            infoRow->addStretch();

            // Vote combo
            auto *ratePrompt = new QLabel("Ваша оценка:");
            ratePrompt->setStyleSheet("color: #64748b; font-size: 12px; font-weight: 500;");
            auto *rateCombo = new QComboBox();
            rateCombo->setObjectName("timeCombo");
            rateCombo->addItems({"Не оценено", "1 ⭐", "2 ⭐", "3 ⭐", "4 ⭐", "5 ⭐"});
            rateCombo->setFixedWidth(120);
            
            // Set current rating
            rateCombo->setCurrentIndex(tip.myRating);

            // Connect using activated to avoid recursive events
            connect(rateCombo, QOverload<int>::of(&QComboBox::activated), this, [this, tip](int index) {
                m_dm->rateTip(tip.id, index);
                refreshAll();
            });

            infoRow->addWidget(ratePrompt);
            infoRow->addWidget(rateCombo);

            vlay->addLayout(infoRow);
            m_tipsListLayout->addWidget(card);
        }
    }
    m_tipsListLayout->addStretch();
}

void MainWindow::refreshAll() {
    refreshBalance();
    refreshChart();
    refreshCategories();
    refreshTable();
    refreshRates();
    refreshCharts();
    refreshAdmin();
    refreshLimits();
    refreshTips();
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
        slice->setColor(m_dm->colorForCategory(cat));
        slice->setBorderColor(QColor("#1a1c29"));
        slice->setBorderWidth(2);
        
        slice->setLabel(QString("%1 %2%").arg(m_dm->iconForCategory(cat)).arg(pct, 0, 'f', 0));
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
        dot->setStyleSheet(QString("color: %1; font-size: 16px;").arg(m_dm->colorForCategory(cat).name()));

        auto *nameLabel = new QLabel(m_dm->iconForCategory(cat) + "  " + cat);
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
    QDateTime start = QDateTime::currentDateTime();
    if (m_currentRange == "day") start = start.addDays(-1);
    else if (m_currentRange == "week") start = start.addDays(-7);
    else if (m_currentRange == "month") start = start.addMonths(-1);
    else if (m_currentRange == "year") start = start.addYears(-1);
    else start = QDateTime::fromMSecsSinceEpoch(0);

    const auto &all_list = m_dm->transactions();
    QList<Transaction> list;
    for (const auto &t : all_list) {
        if (t.date >= start) list.append(t);
    }

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

void MainWindow::onExportExcel() {
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить историю в Excel (CSV)", "history.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для записи.");
        return;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out.setGenerateByteOrderMark(true);
    out << "Тип;Категория;Сумма;Комментарий;Дата\n";

    for (int i = 0; i < m_table->rowCount(); ++i) {
        QString type = m_table->item(i, 0)->text().remove("🔴 ").remove("🟢 ");
        QString cat = m_table->item(i, 1)->text();
        QString amt = m_table->item(i, 2)->text().remove("₽ ");
        QString com = m_table->item(i, 3)->text();
        QString date = m_table->item(i, 4)->text();
        out << type << ";" << cat << ";" << amt << ";" << com << ";" << date << "\n";
    }
    file.close();
    QMessageBox::information(this, "Успех", "История успешно сохранена в " + fileName);
}

void MainWindow::refreshRates() {
    if (m_pages->currentIndex() == 2) {
        m_netManager->get(QNetworkRequest(QUrl("https://belarusbank.by/api/kursExchange?city=Минск")));
    }
}

void MainWindow::refreshCharts() {
    if (m_pages->currentIndex() != 3) return;

    refreshCatBars(m_expCatBarsWidget, true);
    refreshCatBars(m_incCatBarsWidget, false);

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
    double maxVal = 0;
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
        maxVal = std::max({maxVal, incSum, expSum});
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
    lAxisY->setRange(0, maxVal * 1.2 + 1);
    lAxisY->setLabelsColor(QColor("#94a3b8"));
    lAxisY->setGridLineColor(QColor("#232533"));
    lineChart->addAxis(lAxisY, Qt::AlignLeft);
    incSeries->attachAxis(lAxisY);
    expSeries->attachAxis(lAxisY);
    lineChart->legend()->setVisible(true);
    lineChart->legend()->setLabelColor(QColor("#f8fafc"));
    lineChart->legend()->setAlignment(Qt::AlignBottom);

    // Refresh daily charts
    auto *dailyChart = m_dailyChartView->chart();
    dailyChart->removeAllSeries();
    for (auto *axis : dailyChart->axes()) dailyChart->removeAxis(axis);

    auto *dailyIncSeries = new QLineSeries();
    dailyIncSeries->setName("Доходы");
    dailyIncSeries->setColor(QColor("#10b981"));
    auto *dailyExpSeries = new QLineSeries();
    dailyExpSeries->setName("Расходы");
    dailyExpSeries->setColor(QColor("#ef4444"));

    auto incData = m_dm->getDailyData("income");
    auto expData = m_dm->getDailyData("expense");

    QStringList days;
    double maxDaily = 0;
    for (int i = 13; i >= 0; --i) {
        QDate d = today.addDays(-i);
        days << d.toString("dd.MM");
        double dInc = incData.value(d, 0.0);
        double dExp = expData.value(d, 0.0);
        dailyIncSeries->append(13 - i, dInc);
        dailyExpSeries->append(13 - i, dExp);
        maxDaily = std::max({maxDaily, dInc, dExp});
    }
    dailyChart->addSeries(dailyIncSeries);
    dailyChart->addSeries(dailyExpSeries);

    auto *dAxisX = new QBarCategoryAxis();
    dAxisX->append(days);
    dAxisX->setLabelsColor(QColor("#94a3b8"));
    dailyChart->addAxis(dAxisX, Qt::AlignBottom);
    dailyIncSeries->attachAxis(dAxisX);
    dailyExpSeries->attachAxis(dAxisX);

    auto *dAxisY = new QValueAxis();
    dAxisY->setRange(0, maxDaily * 1.2 + 1);
    dAxisY->setLabelsColor(QColor("#94a3b8"));
    dAxisY->setGridLineColor(QColor("#232533"));
    dailyChart->addAxis(dAxisY, Qt::AlignLeft);
    dailyIncSeries->attachAxis(dAxisY);
    dailyExpSeries->attachAxis(dAxisY);
    dailyChart->legend()->setVisible(true);
    dailyChart->legend()->setLabelColor(QColor("#f8fafc"));
    dailyChart->legend()->setAlignment(Qt::AlignBottom);
}

void MainWindow::refreshCatBars(QWidget *container, bool isExpense) {
    if (!container) return;
    auto *lay = qobject_cast<QVBoxLayout*>(container->layout());
    if (!lay) return;

    QLayoutItem *child;
    while ((child = lay->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    auto data = isExpense ? m_dm->expensesByCategory(m_currentRange) : m_dm->incomeByCategory(m_currentRange);
    double total = 0;
    for (auto v : data) total += v;

    if (total <= 0) {
        auto *empty = new QLabel("Нет данных за этот период");
        empty->setStyleSheet("color: #64748b; font-size: 14px;");
        empty->setAlignment(Qt::AlignCenter);
        lay->addWidget(empty);
        lay->addStretch();
        return;
    }

    QList<QPair<QString, double>> sorted;
    for (auto it = data.begin(); it != data.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](auto &a, auto &b){ return a.second > b.second; });

    for (const auto &pair : sorted) {
        QString cat = pair.first;
        double val = pair.second;
        double pct = (val / total) * 100.0;
        QColor col = m_dm->colorForCategory(cat);

        auto *row = new QWidget();
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 4, 0, 4);

        auto *nameLabel = new QLabel(m_dm->iconForCategory(cat) + " " + cat);
        nameLabel->setFixedWidth(160);
        nameLabel->setStyleSheet("color: #e2e8f0; font-size: 13px; font-weight: 600;");

        auto *barBg = new QFrame();
        barBg->setFixedHeight(12);
        barBg->setStyleSheet("background: #232533; border-radius: 6px;");
        barBg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        
        auto *barBgLay = new QHBoxLayout(barBg);
        barBgLay->setContentsMargins(0, 0, 0, 0);
        barBgLay->setSpacing(0);
        
        auto *barFill = new QFrame();
        barFill->setFixedHeight(12);
        barFill->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(col.name()));
        barFill->setFixedWidth(qMax(6, static_cast<int>(pct * 3.0))); // Rough scaling
        
        barBgLay->addWidget(barFill);
        barBgLay->addStretch();

        auto *pctLabel = new QLabel(QString("%1%").arg(pct, 0, 'f', 1));
        pctLabel->setFixedWidth(50);
        pctLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pctLabel->setStyleSheet(QString("color: %1; font-weight: 800; font-size: 12px;").arg(col.name()));

        auto *valLabel = new QLabel(QString("%1 ₽").arg(val, 0, 'f', 0));
        valLabel->setFixedWidth(90);
        valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valLabel->setStyleSheet("color: #f8fafc; font-weight: 700; font-size: 13px;");

        rl->addWidget(nameLabel);
        rl->addWidget(barBg, 1);
        rl->addWidget(pctLabel);
        rl->addWidget(valLabel);

        lay->addWidget(row);
    }
    lay->addStretch();
}

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
    AddTransactionDialog dlg(m_dm, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_dm->addTransaction(dlg.result());
        refreshAll();
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

void MainWindow::updateRange(int idx) {
    if(idx == 0) m_currentRange = "all";
    else if(idx == 1) m_currentRange = "year";
    else if(idx == 2) m_currentRange = "month";
    else if(idx == 3) m_currentRange = "week";
    
    // Temporarily block signals to avoid recursive refresh calls
    if (m_globalRangeCombo) { m_globalRangeCombo->blockSignals(true); m_globalRangeCombo->setCurrentIndex(idx); m_globalRangeCombo->blockSignals(false); }
    if (m_histRangeCombo) { m_histRangeCombo->blockSignals(true); m_histRangeCombo->setCurrentIndex(idx); m_histRangeCombo->blockSignals(false); }
    if (m_chartsRangeCombo) { m_chartsRangeCombo->blockSignals(true); m_chartsRangeCombo->setCurrentIndex(idx); m_chartsRangeCombo->blockSignals(false); }
    
    refreshAll();
}

void MainWindow::setupAdmin() {
    m_adminPage = new QWidget();
    auto *rootLay = new QVBoxLayout(m_adminPage);
    rootLay->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("background: transparent; border: none;");
    auto *scrollContent = new QWidget();
    auto *lay = new QVBoxLayout(scrollContent);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(24);

    auto *header = new QLabel("Панель администратора");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    // --- Статистика ---
    auto *statsRow = new QHBoxLayout();
    auto makeStat = [&](const QString &title, QLabel **lbl) {
        auto *f = new QFrame();
        f->setObjectName("chartCard");
        auto *vl = new QVBoxLayout(f);
        auto *tl = new QLabel(title);
        tl->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 700;");
        *lbl = new QLabel("0");
        (*lbl)->setStyleSheet("color: #f8fafc; font-size: 20px; font-weight: 800;");
        vl->addWidget(tl);
        vl->addWidget(*lbl);
        statsRow->addWidget(f, 1);
    };
    makeStat("ПОЛЬЗОВАТЕЛИ", &m_adminStatsUsers);
    makeStat("АДМИНИСТРАТОРЫ", &m_adminStatsAdmins);
    makeStat("ТРАНЗАКЦИИ", &m_adminStatsTrans);
    makeStat("ОБОРОТ", &m_adminStatsTurnover);
    lay->addLayout(statsRow);

    // --- Категории ---
    auto *catCard = new QFrame();
    catCard->setObjectName("chartCard");
    auto *catLay = new QVBoxLayout(catCard);
    auto *catTitle = new QLabel("Создать глобальную категорию");
    catTitle->setStyleSheet("color: #f8fafc; font-size: 16px; font-weight: 700;");
    catLay->addWidget(catTitle);

    auto *catForm = new QHBoxLayout();
    
    auto makeInput = [&](const QString &lbl, QWidget *w) {
        auto *v = new QVBoxLayout();
        v->setSpacing(4);
        auto *l = new QLabel(lbl);
        l->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 600;");
        v->addWidget(l);
        v->addWidget(w);
        catForm->addLayout(v);
    };

    m_newCatName = new QLineEdit(); m_newCatName->setPlaceholderText("Напр. Продукты"); m_newCatName->setObjectName("authInput");
    m_newCatType = new QComboBox(); m_newCatType->addItems({"expense", "income"}); m_newCatType->setObjectName("timeCombo");
    m_newCatColor = new QLineEdit(); m_newCatColor->setPlaceholderText("#10b981"); m_newCatColor->setObjectName("authInput");
    m_newCatIcon = new QLineEdit(); m_newCatIcon->setPlaceholderText("🍎"); m_newCatIcon->setObjectName("authInput");
    auto *addCatBtn = new QPushButton("Добавить"); addCatBtn->setObjectName("primaryBtn");
    addCatBtn->setFixedWidth(120);
    
    makeInput("Название", m_newCatName);
    makeInput("Тип", m_newCatType);
    makeInput("Цвет", m_newCatColor);
    makeInput("Иконка", m_newCatIcon);
    
    auto *btnV = new QVBoxLayout();
    btnV->addSpacing(18); // Align with inputs
    btnV->addWidget(addCatBtn);
    catForm->addLayout(btnV);

    catLay->addLayout(catForm);
    lay->addWidget(catCard);

    connect(addCatBtn, &QPushButton::clicked, this, [this](){
        if (m_newCatName->text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Введите название категории");
            return;
        }
        m_dm->addGlobalCategory(m_newCatName->text(), m_newCatType->currentText(), m_newCatColor->text(), m_newCatIcon->text());
        m_newCatName->clear();
        m_newCatColor->clear();
        m_newCatIcon->clear();
        refreshAll();
    });

    // --- Таблица пользователей ---
    auto *userTitle = new QLabel("Управление пользователями");
    userTitle->setStyleSheet("color: #f8fafc; font-size: 16px; font-weight: 700;");
    lay->addWidget(userTitle);

    m_adminTable = new QTableWidget(0, 4);
    m_adminTable->setObjectName("transTable");
    m_adminTable->setHorizontalHeaderLabels({"ID", "Пользователь", "Роль", "Действия"});
    m_adminTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_adminTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_adminTable->setColumnWidth(3, 300); // Increased width
    m_adminTable->verticalHeader()->setDefaultSectionSize(60); // Set taller row height
    m_adminTable->verticalHeader()->setVisible(false);
    m_adminTable->setMinimumHeight(400);
    lay->addWidget(m_adminTable, 1);

    scroll->setWidget(scrollContent);
    rootLay->addWidget(scroll);
}

void MainWindow::refreshAdmin() {
    if (!m_dm->isAdmin() || !m_adminPage) return;

    // Обновляем статистику
    auto s = m_dm->getGlobalStats();
    m_adminStatsUsers->setText(QString::number(s.totalUsers));
    m_adminStatsAdmins->setText(QString::number(s.totalAdmins));
    m_adminStatsTrans->setText(QString::number(s.totalTransactions));
    m_adminStatsTurnover->setText(QString("%1 ₽").arg(s.totalTurnover, 0, 'f', 2));

    // Обновляем таблицу
    auto users = m_dm->getAllUsers();
    m_adminTable->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        auto u = users[i];
        m_adminTable->setItem(i, 0, new QTableWidgetItem(QString::number(u.id)));
        m_adminTable->setItem(i, 1, new QTableWidgetItem(u.username));
        m_adminTable->setItem(i, 2, new QTableWidgetItem(u.role == 1 ? "Админ" : "Пользователь"));
        
        auto *btnContainer = new QWidget();
        auto *btnLay = new QHBoxLayout(btnContainer);
        btnLay->setContentsMargins(10, 8, 10, 8); // More margins
        btnLay->setSpacing(8);

        auto *resetBtn = new QPushButton("Сброс");
        resetBtn->setCursor(Qt::PointingHandCursor);
        resetBtn->setFixedHeight(36); // Fixed height for button
        resetBtn->setStyleSheet("background: #f59e0b; color: white; border: none; border-radius: 8px; padding: 4px 12px; font-size: 12px; font-weight: 700;");
        connect(resetBtn, &QPushButton::clicked, this, [this, u](){
            if (QMessageBox::question(this, "Сброс", "Сбросить данные пользователя " + u.username + "?") == QMessageBox::Yes) {
                m_dm->resetUserData(u.id);
                refreshAll();
            }
        });

        auto *roleBtn = new QPushButton(u.role == 1 ? "Юзер" : "Админ");
        roleBtn->setCursor(Qt::PointingHandCursor);
        roleBtn->setFixedHeight(36);
        roleBtn->setStyleSheet("background: #6366f1; color: white; border: none; border-radius: 8px; padding: 4px 12px; font-size: 12px; font-weight: 700;");
        connect(roleBtn, &QPushButton::clicked, this, [this, u](){
            m_dm->setUserRole(u.id, u.role == 1 ? 0 : 1);
            refreshAll();
        });

        auto *delBtn = new QPushButton("Удалить");
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFixedHeight(36);
        delBtn->setStyleSheet("background: #ef4444; color: white; border: none; border-radius: 8px; padding: 4px 12px; font-size: 12px; font-weight: 700;");
        connect(delBtn, &QPushButton::clicked, this, [this, u](){
            if (QMessageBox::question(this, "Удалить?", "Удалить пользователя " + u.username + "?") == QMessageBox::Yes) {
                if (m_dm->deleteUser(u.id)) refreshAll();
            }
        });

        btnLay->addWidget(resetBtn);
        btnLay->addWidget(roleBtn);
        btnLay->addWidget(delBtn);
        m_adminTable->setCellWidget(i, 3, btnContainer);
    }
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
        #timeCombo QAbstractItemView { background: #232533; border: 1px solid rgba(255,255,255,0.05); selection-background-color: #6366f1; color: #f8fafc; outline: none; }
        #catRow { background: #232533; border-radius: 14px; border: 1px solid rgba(255,255,255,0.03); }
        #catTitle { font-size: 18px; font-weight: 700; color: #f8fafc; }
        #transTable { background: #1a1c29; alternate-background-color: #1d1f2e; border: 1px solid rgba(255,255,255,0.05); border-radius: 18px; gridline-color: transparent; font-size: 13px; outline: none; }
        #transTable::item { padding: 12px 16px; border: none; }
        #primaryBtn { background: #6366f1; color: white; border: none; border-radius: 12px; padding: 10px 24px; font-size: 14px; font-weight: 700; }
        #dangerBtn { background: rgba(239, 68, 68, 0.1); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.2); border-radius: 12px; padding: 10px 24px; font-size: 14px; font-weight: 700; }
        QHeaderView::section { background: #151722; color: #64748b; font-size: 12px; font-weight: 700; text-transform: uppercase; padding: 14px; border: none; border-bottom: 1px solid rgba(255,255,255,0.05); }
    )");
}
void MainWindow::setupLimits() {
    m_limitsPage = new QWidget();
    auto *lay = new QVBoxLayout(m_limitsPage);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(24);

    auto *header = new QLabel("Управление лимитами");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    auto *card = new QFrame();
    card->setObjectName("chartCard");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(24, 24, 24, 24);
    cl->setSpacing(20);

    auto *formLay = new QHBoxLayout();
    m_limitAmountEdit = new QLineEdit();
    m_limitAmountEdit->setPlaceholderText("Сумма лимита");
    m_limitAmountEdit->setObjectName("authInput");
    
    m_limitPeriodCombo = new QComboBox();
    m_limitPeriodCombo->addItems({"Месяц", "Год"});
    m_limitPeriodCombo->setObjectName("timeCombo");
    
    auto *saveBtn = new QPushButton("Установить лимит");
    saveBtn->setObjectName("primaryBtn");
    
    formLay->addWidget(m_limitAmountEdit, 2);
    formLay->addWidget(m_limitPeriodCombo, 1);
    formLay->addWidget(saveBtn, 1);
    cl->addLayout(formLay);

    m_limitStatusLabel = new QLabel("Лимит не установлен");
    m_limitStatusLabel->setStyleSheet("color: #94a3b8; font-size: 14px; font-weight: 600;");
    cl->addWidget(m_limitStatusLabel);

    m_limitProgressBar = new QProgressBar();
    m_limitProgressBar->setFixedHeight(12);
    m_limitProgressBar->setTextVisible(false);
    m_limitProgressBar->setStyleSheet(R"(
        QProgressBar { background: rgba(255,255,255,0.05); border: none; border-radius: 6px; }
        QProgressBar::chunk { background: #6366f1; border-radius: 6px; }
    )");
    cl->addWidget(m_limitProgressBar);

    lay->addWidget(card);
    lay->addStretch();

    connect(saveBtn, &QPushButton::clicked, this, [this](){
        qDebug() << "Limit save button clicked";
        QString text = m_limitAmountEdit->text().replace(",", ".");
        bool ok;
        double val = text.toDouble(&ok);
        if (ok && val > 0) {
            m_dm->setLimit(val, m_limitPeriodCombo->currentText() == "Месяц" ? "month" : "year");
            m_lastLimitPercent = 100.0;
            refreshAll();
            QMessageBox::information(this, "Успех", "Лимит успешно установлен!");
            m_limitAmountEdit->clear();
        } else {
            QMessageBox::warning(this, "Ошибка", "Введите корректную сумму лимита (больше 0)");
        }
    });
}

void MainWindow::refreshLimits() {
    UserLimit l = m_dm->getLimit();
    if (l.amount <= 0) {
        if (m_limitStatusLabel) m_limitStatusLabel->setText("Лимит не установлен");
        if (m_limitProgressBar) m_limitProgressBar->setValue(0);
        if (m_dashLimitLabel) m_dashLimitLabel->setText("Нет лимита");
        if (m_dashLimitProgress) m_dashLimitProgress->setValue(0);
        return;
    }

    double spent = m_dm->getSpentInCurrentLimit();
    double remaining = l.amount - spent;
    int percent = (l.amount > 0) ? qMax(0, qMin(100, int((remaining / l.amount) * 100))) : 0;

    QString status = QString("Остаток: %1 ₽ из %2 ₽ (%3%)")
                        .arg(qMax(0.0, remaining), 0, 'f', 2)
                        .arg(l.amount, 0, 'f', 2)
                        .arg(percent);

    if (m_limitStatusLabel) m_limitStatusLabel->setText(status);
    if (m_limitProgressBar) m_limitProgressBar->setValue(percent);
    if (m_dashLimitLabel) m_dashLimitLabel->setText(QString("%1 ₽").arg(qMax(0.0, remaining), 0, 'f', 0));
    if (m_dashLimitProgress) m_dashLimitProgress->setValue(percent);

    // Notification logic
    double currentPercent = (remaining / l.amount) * 100.0;
    
    auto notify = [this](const QString &msg) {
        QMessageBox *box = new QMessageBox(QMessageBox::Warning, "Внимание", msg, QMessageBox::Ok, this);
        box->setStyleSheet("QLabel { color: #f8fafc; } QPushButton { background: #6366f1; color: white; padding: 8px 16px; border-radius: 8px; }");
        box->show();
    };

    if (m_lastLimitPercent > 50 && currentPercent <= 50 && currentPercent > 10) {
        notify("Осталось менее 50% лимита!");
    } else if (m_lastLimitPercent > 10 && currentPercent <= 10 && currentPercent > 0) {
        notify("Осталось менее 10% лимита! Будьте осторожны.");
    } else if (m_lastLimitPercent > 0 && currentPercent <= 0) {
        notify("Лимит исчерпан! Вы превысили свои бюджетные ожидания.");
    }

    m_lastLimitPercent = currentPercent;
}

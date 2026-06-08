#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollArea>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProgressBar>
#include <QTextEdit>

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

#include "datamanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DataManager *dm, const QString &username, QWidget *parent = nullptr);
signals:
    void logoutRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAddTransaction();
    void onDeleteTransaction();
    void onRatesReceived(QNetworkReply *reply);
    void onConvert();
    void onSearch(const QString &text);
    void updateRange(int idx);

private:
    // UI setup
    void setupUi();
    void setupSidebar();
    void setupDashboard();
    void setupHistory();
    void setupRates();
    void setupCharts();
    void setupTips();
    void setupLimits();
    void setupAdmin();

    // Data refresh
    void refreshAll();
    void refreshBalance();
    void refreshChart();
    void refreshCategories();
    void refreshTable();
    void refreshRates();
    void refreshCharts();
    void refreshCatBars(QWidget *container, bool isExpense);
    void refreshAdmin();
    void refreshLimits();
    void refreshTips();

    void applyStyle();

    DataManager *m_dm;
    QString      m_username;
    int          m_currentTab = 0;     // 0=расходы, 1=доходы на дашборде

    // Root
    QWidget         *m_centralWidget = nullptr;
    QHBoxLayout     *m_rootLayout    = nullptr;

    // Global time filter
    QComboBox       *m_globalRangeCombo = nullptr;
    QString          m_currentRange = "all";

    // Sidebar
    QFrame          *m_sidebar         = nullptr;
    QPushButton     *m_navDashboard    = nullptr;
    QPushButton     *m_navHistory      = nullptr;
    QPushButton     *m_navRates        = nullptr;
    QPushButton     *m_navCharts       = nullptr;
    QPushButton     *m_navTips         = nullptr;
    QPushButton     *m_navLimits       = nullptr;
    QPushButton     *m_navAdmin        = nullptr;
    QLabel          *m_userLabel       = nullptr;
    QPushButton     *m_logoutBtn       = nullptr;
    QPushButton     *m_addTransBtnSidebar = nullptr;

    QStackedWidget  *m_pages = nullptr;

    // Dashboard page
    QWidget         *m_dashPage        = nullptr;
    QLabel          *m_balanceLabel    = nullptr;
    QLabel          *m_incomeLabel     = nullptr;
    QLabel          *m_expenseLabel    = nullptr;
    QProgressBar    *m_dashLimitProgress = nullptr;
    QLabel          *m_dashLimitLabel  = nullptr;
    QChartView      *m_chartView       = nullptr;
    QPieSeries      *m_pieSeries       = nullptr;
    QVBoxLayout     *m_catListLayout   = nullptr;
    QPushButton     *m_fabBtn          = nullptr;

    // History page
    QWidget         *m_histPage  = nullptr;
    QComboBox       *m_histRangeCombo = nullptr;
    QLineEdit       *m_searchEdit = nullptr;
    QTableWidget    *m_table     = nullptr;
    QPushButton     *m_deleteBtn = nullptr;

    // Rates page
    QWidget         *m_ratesPage  = nullptr;
    QTableWidget    *m_ratesTable = nullptr;
    QNetworkAccessManager *m_netManager = nullptr;
    QLineEdit       *m_convAmount = nullptr;
    QComboBox       *m_convFrom   = nullptr;
    QComboBox       *m_convTo     = nullptr;
    QLabel          *m_convResult = nullptr;
    QMap<QString, double> m_currentRates;

    // Charts page
    QWidget         *m_chartsPage       = nullptr;
    QComboBox       *m_chartsRangeCombo = nullptr;
    QWidget         *m_expCatBarsWidget = nullptr;
    QWidget         *m_incCatBarsWidget = nullptr;
    QChartView      *m_lineChartView    = nullptr;
    QChartView      *m_dailyChartView   = nullptr;

    // Tips page
    QWidget         *m_tipsPage = nullptr;
    QScrollArea     *m_tipsScrollArea = nullptr;
    QWidget         *m_tipsScrollContainer = nullptr;
    QVBoxLayout     *m_tipsListLayout = nullptr;
    QTextEdit       *m_newTipEdit = nullptr;
    QPushButton     *m_addTipBtn = nullptr;

    // Limits page
    QWidget         *m_limitsPage = nullptr;
    QLineEdit       *m_limitAmountEdit = nullptr;
    QComboBox       *m_limitPeriodCombo = nullptr;
    QProgressBar    *m_limitProgressBar = nullptr;
    QLabel          *m_limitStatusLabel = nullptr;
    double          m_lastLimitPercent = 100.0;

    // Admin page
    QWidget         *m_adminPage = nullptr;
    QTableWidget    *m_adminTable = nullptr;
    QLabel          *m_adminStatsUsers = nullptr;
    QLabel          *m_adminStatsAdmins = nullptr;
    QLabel          *m_adminStatsTrans = nullptr;
    QLabel          *m_adminStatsTurnover = nullptr;
    QLineEdit       *m_newCatName = nullptr;
    QComboBox       *m_newCatType = nullptr;
    QLineEdit       *m_newCatColor = nullptr;
    QLineEdit       *m_newCatIcon = nullptr;
};

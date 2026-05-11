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

private:
    // UI setup
    void setupUi();
    void setupSidebar();
    void setupDashboard();
    void setupHistory();
    void setupRates();
    void setupCharts();
    void setupTips();

    // Data refresh
    void refreshAll();
    void refreshBalance();
    void refreshChart();
    void refreshCategories();
    void refreshTable();
    void refreshRates();
    void refreshCharts();
    void refreshCatBars(QWidget *container, bool isExpense);

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
    QLabel          *m_userLabel       = nullptr;
    QPushButton     *m_logoutBtn       = nullptr;

    QStackedWidget  *m_pages = nullptr;

    // Dashboard page
    QWidget         *m_dashPage        = nullptr;
    QLabel          *m_balanceLabel    = nullptr;
    QLabel          *m_incomeLabel     = nullptr;
    QLabel          *m_expenseLabel    = nullptr;
    QChartView      *m_chartView       = nullptr;
    QPieSeries      *m_pieSeries       = nullptr;
    QVBoxLayout     *m_catListLayout   = nullptr;
    QPushButton     *m_fabBtn          = nullptr;

    // History page
    QWidget         *m_histPage  = nullptr;
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
    QWidget         *m_expCatBarsWidget = nullptr;
    QWidget         *m_incCatBarsWidget = nullptr;
    QChartView      *m_lineChartView    = nullptr;

    // Tips page
    QWidget         *m_tipsPage = nullptr;
};

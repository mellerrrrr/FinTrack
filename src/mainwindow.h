#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QResizeEvent>

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>

#include "datamanager.h"

// ===== Главное окно приложения =====
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DataManager *dm, const QString &username, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAddTransaction();
    void onDeleteTransaction();

private:
    void setupUi();
    void setupSidebar();
    void setupDashboard();
    void setupHistory();

    void refreshAll();
    void refreshBalance();
    void refreshChart();
    void refreshTable();
    void refreshCategories();

    void applyStyle();

    DataManager *m_dm;
    QString      m_username;
    int          m_currentTab = 0;

    QWidget         *m_centralWidget;
    QHBoxLayout     *m_rootLayout;

    QFrame          *m_sidebar;
    QPushButton     *m_navDashboard;
    QPushButton     *m_navHistory;
    QLabel          *m_userLabel;

    QStackedWidget  *m_pages;

    QWidget         *m_dashPage;
    QLabel          *m_balanceLabel;
    QLabel          *m_incomeLabel;
    QLabel          *m_expenseLabel;
    QChartView      *m_chartView;
    QPieSeries      *m_pieSeries;
    QVBoxLayout     *m_catListLayout;

    QWidget         *m_histPage;
    QTableWidget    *m_table;
    QPushButton     *m_deleteBtn;

    QPushButton     *m_fabBtn;
};

#pragma once

#include <QString>
#include <QList>
#include <QDateTime>
#include <QMap>
#include <QColor>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

struct Transaction {
    int id;
    int userId;
    QString type;
    QString category;
    double amount;
    QString comment;
    QDateTime date;
};

struct CategoryInfo {
    QString name;
    QColor color;
    QString icon;
};

struct UserInfo {
    int id;
    QString username;
    int role; // 0 = user, 1 = admin
};

struct GlobalStats {
    int totalUsers;
    int totalAdmins;
    int totalTransactions;
    double totalTurnover;
    QString topCategory;
};

struct UserLimit {
    double amount;
    QString period;
    QDateTime startDate;
};

struct Budget {
    QString category;
    double limit;
    double current;
};

struct Tip {
    int id;
    int userId;
    QString username;
    QString text;
    double averageRating;
    int ratingCount;
    int myRating; // 0 if not rated
};

class DataManager : public QObject {
public:
    DataManager();
    ~DataManager();

    bool registerUser(const QString &username, const QString &password);
    bool loginUser(const QString &username, const QString &password);
    void logoutUser();

    void addTransaction(const Transaction &t);
    void removeTransaction(int id);
    QList<Transaction> transactions() const;

    // Tips methods
    QList<Tip> getTips() const;
    void addTip(const QString &text);
    void rateTip(int tipId, int rating);

    double balance() const;
    double totalIncome() const;
    double totalExpense() const;

    QMap<QString, double> expensesByCategory(const QString &range = "all") const;
    QMap<QString, double> incomeByCategory(const QString &range = "all") const;
    QMap<QDate, double> getDailyData(const QString &type) const; // type="balance", "expense", "income"

    QList<CategoryInfo> expenseCategories() const;
    QList<CategoryInfo> incomeCategories() const;
    QColor colorForCategory(const QString &cat) const;
    QString iconForCategory(const QString &cat) const;

    void setLimit(double amount, const QString &period);
    UserLimit getLimit() const;
    double getSpentInCurrentLimit() const;

    // Admin methods
    bool isAdmin() const;
    QList<UserInfo> getAllUsers() const;
    bool setUserRole(int userId, int role);
    bool deleteUser(int userId);
    void resetUserData(int userId);
    void generateRandomData(int userId);
    GlobalStats getGlobalStats() const;

    void addGlobalCategory(const QString &name, const QString &type, const QString &color, const QString &icon);
    void deleteGlobalCategory(const QString &name);
    void addUserCategory(const QString &name, const QString &type, const QString &color, const QString &icon);

    // Budgets
    void setBudget(const QString &cat, double limit);
    QList<Budget> getBudgets() const;

private:
    void initDatabase();
    QString hashPassword(const QString &password) const;
    QDateTime getStartDate(const QString &range) const;

    QSqlDatabase m_db;
    int m_currentUserId = -1;
    QString m_currentUsername;
    int m_currentUserRole = 0;
};

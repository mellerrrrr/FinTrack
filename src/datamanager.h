#pragma once

#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QColor>

// ===== Структура одной транзакции =====
struct Transaction {
    int     id;
    QString type;       // "expense" или "income"
    QString category;
    double  amount;
    QString comment;
    QDateTime date;

    // Сериализация транзакции в JSON-объект
    QJsonObject toJson() const;
    // Десериализация из JSON-объекта
    static Transaction fromJson(const QJsonObject &obj);
};

// ===== Вспомогательный класс для категорий =====
struct CategoryInfo {
    QString name;
    QColor  color;
    QString icon; // unicode emoji
};

// ===== Менеджер данных: читает/пишет JSON, хранит список транзакций =====
class DataManager {
public:
    DataManager();

    // Устанавливает текущего пользователя (меняет файл данных)
    void setCurrentUser(const QString &username);

    // Загружает транзакции из файла transactions_<username>.json
    bool loadTransactions();

    // Сохраняет все транзакции в файл
    bool saveTransactions();

    // Добавляет транзакцию и сохраняет файл
    void addTransaction(const Transaction &t);

    // Удаляет транзакцию по id
    void removeTransaction(int id);

    // Возвращает весь список
    const QList<Transaction>& transactions() const;

    // Считает баланс (доходы - расходы)
    double balance() const;
    double totalIncome() const;
    double totalExpense() const;

    // Суммы расходов по категориям (для диаграммы)
    QMap<QString, double> expensesByCategory() const;
    QMap<QString, double> incomeByCategory() const;

    // Проверка/создание пользователя
    bool registerUser(const QString &username, const QString &password);
    bool loginUser(const QString &username, const QString &password);

    // Список предустановленных категорий расходов
    static QList<CategoryInfo> expenseCategories();
    // Список предустановленных категорий доходов
    static QList<CategoryInfo> incomeCategories();
    // Цвет для конкретной категории
    static QColor colorForCategory(const QString &cat);

private:
    QString             m_username;
    QString             m_dataFilePath;
    QList<Transaction>  m_transactions;
    int                 m_nextId = 1;

    // Путь к файлу пользователей
    static QString usersFilePath();
    // Путь к файлу транзакций конкретного пользователя
    QString transactionsFilePath() const;
};

#include "datamanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>

// ─────────────────────────────────────────────
//  Transaction: сериализация / десериализация
// ─────────────────────────────────────────────

QJsonObject Transaction::toJson() const {
    QJsonObject obj;
    obj["id"]       = id;
    obj["type"]     = type;
    obj["category"] = category;
    obj["amount"]   = amount;
    obj["comment"]  = comment;
    obj["date"]     = date.toString(Qt::ISODate);
    return obj;
}

Transaction Transaction::fromJson(const QJsonObject &obj) {
    Transaction t;
    t.id       = obj["id"].toInt();
    t.type     = obj["type"].toString();
    t.category = obj["category"].toString();
    t.amount   = obj["amount"].toDouble();
    t.comment  = obj["comment"].toString();
    t.date     = QDateTime::fromString(obj["date"].toString(), Qt::ISODate);
    return t;
}

// ─────────────────────────────────────────────
//  DataManager
// ─────────────────────────────────────────────

DataManager::DataManager() {}

void DataManager::setCurrentUser(const QString &username) {
    m_username = username;
    loadTransactions();
}

// Путь к файлу пользователей (рядом с exe или в AppData)
QString DataManager::usersFilePath() {
    return QDir::currentPath() + "/users.json";
}

QString DataManager::transactionsFilePath() const {
    return QDir::currentPath() + "/transactions_" + m_username + ".json";
}

// ─────────────────────────────────────────────
//  Регистрация: записывает нового юзера в users.json
// ─────────────────────────────────────────────
bool DataManager::registerUser(const QString &username, const QString &password) {
    QFile file(usersFilePath());

    QJsonArray users;

    // Читаем существующих пользователей
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isArray())
            users = doc.array();
    }

    // Проверяем, нет ли уже такого логина
    for (const QJsonValue &v : users) {
        if (v.toObject()["username"].toString() == username)
            return false; // уже занят
    }

    // Простое хэширование пароля через SHA-256
    QString hash = QString(QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha256).toHex());

    QJsonObject newUser;
    newUser["username"] = username;
    newUser["password"] = hash;
    users.append(newUser);

    // Записываем обратно
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(users).toJson());
    file.close();
    return true;
}

// ─────────────────────────────────────────────
//  Логин: проверяет логин/пароль из users.json
// ─────────────────────────────────────────────
bool DataManager::loginUser(const QString &username, const QString &password) {
    QFile file(usersFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return false;

    QString hash = QString(QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha256).toHex());

    for (const QJsonValue &v : doc.array()) {
        QJsonObject obj = v.toObject();
        if (obj["username"].toString() == username &&
            obj["password"].toString() == hash)
            return true;
    }
    return false;
}

// ─────────────────────────────────────────────
//  Загрузка транзакций из transactions_<user>.json
// ─────────────────────────────────────────────
bool DataManager::loadTransactions() {
    m_transactions.clear();
    m_nextId = 1;

    QFile file(transactionsFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false; // файла ещё нет — это нормально для нового юзера

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return false;

    // Разбираем каждый элемент массива как Transaction
    for (const QJsonValue &val : doc.array()) {
        Transaction t = Transaction::fromJson(val.toObject());
        m_transactions.append(t);
        if (t.id >= m_nextId)
            m_nextId = t.id + 1; // поддерживаем уникальный счётчик id
    }
    return true;
}

// ─────────────────────────────────────────────
//  Сохранение всех транзакций в JSON-файл
// ─────────────────────────────────────────────
bool DataManager::saveTransactions() {
    QJsonArray arr;
    // Каждую транзакцию сериализуем через .toJson()
    for (const Transaction &t : m_transactions)
        arr.append(t.toJson());

    QFile file(transactionsFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(arr).toJson());
    file.close();
    return true;
}

void DataManager::addTransaction(const Transaction &t) {
    Transaction copy = t;
    copy.id = m_nextId++;
    if (!copy.date.isValid())
        copy.date = QDateTime::currentDateTime();
    m_transactions.prepend(copy); // новые — сверху
    saveTransactions();
}

void DataManager::removeTransaction(int id) {
    m_transactions.removeIf([id](const Transaction &t){ return t.id == id; });
    saveTransactions();
}

const QList<Transaction>& DataManager::transactions() const {
    return m_transactions;
}

double DataManager::totalIncome() const {
    double s = 0;
    for (const auto &t : m_transactions)
        if (t.type == "income") s += t.amount;
    return s;
}

double DataManager::totalExpense() const {
    double s = 0;
    for (const auto &t : m_transactions)
        if (t.type == "expense") s += t.amount;
    return s;
}

double DataManager::balance() const {
    return totalIncome() - totalExpense();
}

// Суммируем расходы по категориям (для круговой диаграммы)
QMap<QString, double> DataManager::expensesByCategory() const {
    QMap<QString, double> map;
    for (const auto &t : m_transactions)
        if (t.type == "expense")
            map[t.category] += t.amount;
    return map;
}

QMap<QString, double> DataManager::incomeByCategory() const {
    QMap<QString, double> map;
    for (const auto &t : m_transactions)
        if (t.type == "income")
            map[t.category] += t.amount;
    return map;
}

// ─────────────────────────────────────────────
//  Предустановленные категории с цветами
// ─────────────────────────────────────────────
QList<CategoryInfo> DataManager::expenseCategories() {
    return {
        {"Продукты",     QColor("#10b981"), "🛒"},
        {"Транспорт",    QColor("#3b82f6"), "🚗"},
        {"Досуг",        QColor("#6366f1"), "🎮"},
        {"Здоровье",     QColor("#ef4444"), "💊"},
        {"Одежда",       QColor("#f59e0b"), "👗"},
        {"Кафе/Рестор.", QColor("#f97316"), "☕"},
        {"ЖКХ",          QColor("#06b6d4"), "🏠"},
        {"Прочее",       QColor("#64748b"), "📦"},
    };
}

QList<CategoryInfo> DataManager::incomeCategories() {
    return {
        {"Зарплата",    QColor("#10b981"), "💰"},
        {"Фриланс",     QColor("#6366f1"), "💻"},
        {"Подарок",     QColor("#f59e0b"), "🎁"},
        {"Инвестиции",  QColor("#3b82f6"), "📈"},
        {"Прочее",      QColor("#64748b"), "📦"},
    };
}

QColor DataManager::colorForCategory(const QString &cat) {
    for (auto &c : expenseCategories())
        if (c.name == cat) return c.color;
    for (auto &c : incomeCategories())
        if (c.name == cat) return c.color;
    return QColor("#9b8ea8");
}

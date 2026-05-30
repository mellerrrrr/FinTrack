#include "datamanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QVariant>
#include <QDebug>

DataManager::DataManager() {
    initDatabase();
}

DataManager::~DataManager() {
    if (m_db.isOpen()) m_db.close();
}

void DataManager::initDatabase() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    m_db.setDatabaseName(dbPath + "/fintrack.db");
    
    if (!m_db.open()) {
        qDebug() << "Failed to open database! Path: " << dbPath;
        return;
    }

    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS users ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "username TEXT UNIQUE, "
           "password TEXT, "
           "role INTEGER DEFAULT 0)");

    q.exec("CREATE TABLE IF NOT EXISTS transactions ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "user_id INTEGER, "
           "type TEXT, "
           "category TEXT, "
           "amount REAL, "
           "comment TEXT, "
           "date TEXT, "
           "FOREIGN KEY(user_id) REFERENCES users(id))");

    q.exec("CREATE TABLE IF NOT EXISTS global_categories ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "name TEXT UNIQUE, "
           "type TEXT, "
           "color TEXT, "
           "icon TEXT)");

    q.exec("CREATE TABLE IF NOT EXISTS user_categories ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "user_id INTEGER, "
           "name TEXT, "
           "type TEXT, "
           "color TEXT, "
           "icon TEXT, "
           "UNIQUE(user_id, name))");
           
    q.exec("CREATE TABLE IF NOT EXISTS budgets ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "user_id INTEGER, "
           "category TEXT, "
           "limit_amount REAL, "
           "UNIQUE(user_id, category))");

    q.exec("CREATE TABLE IF NOT EXISTS user_limits ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "user_id INTEGER, "
           "amount REAL, "
           "period TEXT, "
           "start_date TEXT, "
           "UNIQUE(user_id))");

    // Create default admin if not exists
    QSqlQuery adminCheck;
    adminCheck.exec("SELECT id FROM users WHERE username='admin'");
    if (!adminCheck.next()) {
        QSqlQuery insertAdmin;
        insertAdmin.prepare("INSERT INTO users (username, password, role) VALUES (?, ?, 1)");
        insertAdmin.addBindValue("admin");
        insertAdmin.addBindValue(hashPassword("admin"));
        insertAdmin.exec();
    }
}

QString DataManager::hashPassword(const QString &password) const {
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool DataManager::registerUser(const QString &username, const QString &password) {
    QSqlQuery q;
    q.prepare("INSERT INTO users (username, password, role) VALUES (?, ?, 0)");
    q.addBindValue(username);
    q.addBindValue(hashPassword(password));
    return q.exec();
}

bool DataManager::loginUser(const QString &username, const QString &password) {
    QSqlQuery q;
    q.prepare("SELECT id, role FROM users WHERE username=? AND password=?");
    q.addBindValue(username);
    q.addBindValue(hashPassword(password));
    if (q.exec() && q.next()) {
        m_currentUserId = q.value(0).toInt();
        m_currentUsername = username;
        m_currentUserRole = q.value(1).toInt();
        return true;
    }
    return false;
}

void DataManager::logoutUser() {
    m_currentUserId = -1;
    m_currentUsername = "";
    m_currentUserRole = 0;
}

void DataManager::setLimit(double amount, const QString &period) {
    if (m_currentUserId == -1) return;
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO user_limits (user_id, amount, period, start_date) VALUES (?, ?, ?, ?)");
    q.addBindValue(m_currentUserId);
    q.addBindValue(amount);
    q.addBindValue(period);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qDebug() << "setLimit error:" << q.lastError().text();
    }
}

UserLimit DataManager::getLimit() const {
    if (m_currentUserId == -1) return {0, "", QDateTime()};
    QSqlQuery q;
    q.prepare("SELECT amount, period, start_date FROM user_limits WHERE user_id = ?");
    q.addBindValue(m_currentUserId);
    if (q.exec() && q.next()) {
        return {q.value(0).toDouble(), q.value(1).toString(), QDateTime::fromString(q.value(2).toString(), Qt::ISODate)};
    }
    return {0, "", QDateTime()};
}

double DataManager::getSpentInCurrentLimit() const {
    UserLimit l = getLimit();
    if (l.amount <= 0) return 0;

    QDateTime start = l.startDate;
    // Adjust start date based on period if needed, but for now we'll just use the set date
    // User said "на какойто срок", so we'll use the start_date from when it was set.

    QSqlQuery q;
    q.prepare("SELECT SUM(amount) FROM transactions WHERE user_id = ? AND type = 'expense' AND date >= ?");
    q.addBindValue(m_currentUserId);
    q.addBindValue(start.toString(Qt::ISODate));
    if (q.exec() && q.next()) {
        return q.value(0).toDouble();
    } else {
        qDebug() << "getSpentInCurrentLimit error:" << q.lastError().text();
    }
    return 0;
}

void DataManager::addTransaction(const Transaction &t) {
    QSqlQuery q;
    q.prepare("INSERT INTO transactions (user_id, type, category, amount, comment, date) VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(m_currentUserId);
    q.addBindValue(t.type);
    q.addBindValue(t.category);
    q.addBindValue(t.amount);
    q.addBindValue(t.comment);
    q.addBindValue(t.date.isValid() ? t.date.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    q.exec();
}

void DataManager::removeTransaction(int id) {
    QSqlQuery q;
    q.prepare("DELETE FROM transactions WHERE id=? AND user_id=?");
    q.addBindValue(id);
    q.addBindValue(m_currentUserId);
    q.exec();
}

QList<Transaction> DataManager::transactions() const {
    QList<Transaction> list;
    QSqlQuery q;
    q.prepare("SELECT id, type, category, amount, comment, date FROM transactions WHERE user_id=? ORDER BY date DESC");
    q.addBindValue(m_currentUserId);
    if (q.exec()) {
        while (q.next()) {
            Transaction t;
            t.id = q.value(0).toInt();
            t.userId = m_currentUserId;
            t.type = q.value(1).toString();
            t.category = q.value(2).toString();
            t.amount = q.value(3).toDouble();
            t.comment = q.value(4).toString();
            t.date = QDateTime::fromString(q.value(5).toString(), Qt::ISODate);
            list.append(t);
        }
    }
    return list;
}

double DataManager::totalIncome() const {
    QSqlQuery q;
    q.prepare("SELECT SUM(amount) FROM transactions WHERE user_id=? AND type='income'");
    q.addBindValue(m_currentUserId);
    if (q.exec() && q.next()) return q.value(0).toDouble();
    return 0;
}

double DataManager::totalExpense() const {
    QSqlQuery q;
    q.prepare("SELECT SUM(amount) FROM transactions WHERE user_id=? AND type='expense'");
    q.addBindValue(m_currentUserId);
    if (q.exec() && q.next()) return q.value(0).toDouble();
    return 0;
}

double DataManager::balance() const {
    return totalIncome() - totalExpense();
}

QDateTime DataManager::getStartDate(const QString &range) const {
    QDateTime start = QDateTime::currentDateTime();
    if (range == "day") start = start.addDays(-1);
    else if (range == "week") start = start.addDays(-7);
    else if (range == "month") start = start.addMonths(-1);
    else if (range == "year") start = start.addYears(-1);
    else return QDateTime::fromMSecsSinceEpoch(0);
    return start;
}

QMap<QString, double> DataManager::expensesByCategory(const QString &range) const {
    QMap<QString, double> map;
    QSqlQuery q;
    QDateTime start = getStartDate(range);
    q.prepare("SELECT category, SUM(amount) FROM transactions WHERE user_id=? AND type='expense' AND date >= ? GROUP BY category");
    q.addBindValue(m_currentUserId);
    q.addBindValue(start.toString(Qt::ISODate));
    if (q.exec()) {
        while (q.next()) {
            map[q.value(0).toString()] = q.value(1).toDouble();
        }
    }
    return map;
}

QMap<QString, double> DataManager::incomeByCategory(const QString &range) const {
    QMap<QString, double> map;
    QSqlQuery q;
    QDateTime start = getStartDate(range);
    q.prepare("SELECT category, SUM(amount) FROM transactions WHERE user_id=? AND type='income' AND date >= ? GROUP BY category");
    q.addBindValue(m_currentUserId);
    q.addBindValue(start.toString(Qt::ISODate));
    if (q.exec()) {
        while (q.next()) {
            map[q.value(0).toString()] = q.value(1).toDouble();
        }
    }
    return map;
}

QMap<QDate, double> DataManager::getDailyData(const QString &type) const {
    QMap<QDate, double> map;
    QSqlQuery q;
    
    if (type == "balance") {
        q.prepare("SELECT substr(date,1,10), SUM(CASE WHEN type='income' THEN amount ELSE -amount END) FROM transactions WHERE user_id=? GROUP BY substr(date,1,10) ORDER BY substr(date,1,10)");
    } else {
        q.prepare("SELECT substr(date,1,10), SUM(amount) FROM transactions WHERE user_id=? AND type=? GROUP BY substr(date,1,10) ORDER BY substr(date,1,10)");
    }
    
    q.addBindValue(m_currentUserId);
    if (type != "balance") q.addBindValue(type);
    
    if (q.exec()) {
        double runningBalance = 0;
        while (q.next()) {
            QDate d = QDate::fromString(q.value(0).toString(), "yyyy-MM-dd");
            double val = q.value(1).toDouble();
            if (type == "balance") {
                runningBalance += val;
                map[d] = runningBalance;
            } else {
                map[d] = val;
            }
        }
    }
    return map;
}

bool DataManager::isAdmin() const {
    return m_currentUserRole == 1;
}

QList<UserInfo> DataManager::getAllUsers() const {
    QList<UserInfo> list;
    QSqlQuery q("SELECT id, username, role FROM users");
    while (q.next()) {
        UserInfo u;
        u.id = q.value(0).toInt();
        u.username = q.value(1).toString();
        u.role = q.value(2).toInt();
        list.append(u);
    }
    return list;
}

bool DataManager::setUserRole(int userId, int role) {
    if (userId == m_currentUserId) return false; // Can't change own role
    QSqlQuery q;
    q.prepare("UPDATE users SET role=? WHERE id=?");
    q.addBindValue(role);
    q.addBindValue(userId);
    return q.exec();
}

bool DataManager::deleteUser(int userId) {
    if (userId == m_currentUserId) return false;
    QSqlQuery q;
    q.prepare("DELETE FROM transactions WHERE user_id=?");
    q.addBindValue(userId);
    q.exec();
    
    q.prepare("DELETE FROM users WHERE id=?");
    q.addBindValue(userId);
    return q.exec();
}

void DataManager::resetUserData(int userId) {
    QSqlQuery q;
    q.prepare("DELETE FROM transactions WHERE user_id=?");
    q.addBindValue(userId);
    q.exec();
}

void DataManager::generateRandomData(int userId) {
    for (int i=0; i<200; ++i) {
        Transaction t;
        t.type = (rand() % 3 == 0) ? "income" : "expense";
        auto cats = t.type == "income" ? incomeCategories() : expenseCategories();
        t.category = cats[rand() % cats.size()].name;
        t.amount = (rand() % 10000) / 10.0 + 10;
        t.comment = "Auto generated";
        t.date = QDateTime::currentDateTime().addDays(-(rand() % 365));
        
        QSqlQuery q;
        q.prepare("INSERT INTO transactions (user_id, type, category, amount, comment, date) VALUES (?, ?, ?, ?, ?, ?)");
        q.addBindValue(userId);
        q.addBindValue(t.type);
        q.addBindValue(t.category);
        q.addBindValue(t.amount);
        q.addBindValue(t.comment);
        q.addBindValue(t.date.toString(Qt::ISODate));
        q.exec();
    }
}

GlobalStats DataManager::getGlobalStats() const {
    GlobalStats s = {0, 0, 0, 0.0, ""};
    QSqlQuery q("SELECT COUNT(*) FROM users");
    if (q.next()) s.totalUsers = q.value(0).toInt();
    
    q.exec("SELECT COUNT(*) FROM users WHERE role=1");
    if (q.next()) s.totalAdmins = q.value(0).toInt();

    q.exec("SELECT COUNT(*), SUM(amount) FROM transactions");
    if (q.next()) {
        s.totalTransactions = q.value(0).toInt();
        s.totalTurnover = q.value(1).toDouble();
    }
    
    q.exec("SELECT category, COUNT(*) as c FROM transactions GROUP BY category ORDER BY c DESC LIMIT 1");
    if (q.next()) s.topCategory = q.value(0).toString();
    
    return s;
}

void DataManager::addGlobalCategory(const QString &name, const QString &type, const QString &color, const QString &icon) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO global_categories (name, type, color, icon) VALUES (?, ?, ?, ?)");
    q.addBindValue(name);
    q.addBindValue(type);
    q.addBindValue(color);
    q.addBindValue(icon);
    q.exec();
}

void DataManager::deleteGlobalCategory(const QString &name) {
    QSqlQuery q;
    q.prepare("DELETE FROM global_categories WHERE name=?");
    q.addBindValue(name);
    q.exec();
}

void DataManager::setBudget(const QString &cat, double limit) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO budgets (user_id, category, limit_amount) VALUES (?, ?, ?)");
    q.addBindValue(m_currentUserId);
    q.addBindValue(cat);
    q.addBindValue(limit);
    q.exec();
}

QList<Budget> DataManager::getBudgets() const {
    QList<Budget> list;
    QSqlQuery q;
    q.prepare("SELECT category, limit_amount FROM budgets WHERE user_id=?");
    q.addBindValue(m_currentUserId);
    if (q.exec()) {
        while (q.next()) {
            Budget b;
            b.category = q.value(0).toString();
            b.limit = q.value(1).toDouble();
            
            QSqlQuery sumQ;
            sumQ.prepare("SELECT SUM(amount) FROM transactions WHERE user_id=? AND type='expense' AND category=? AND date >= ?");
            sumQ.addBindValue(m_currentUserId);
            sumQ.addBindValue(b.category);
            sumQ.addBindValue(QDate::currentDate().addDays(-QDate::currentDate().day() + 1).startOfDay().toString(Qt::ISODate));
            sumQ.exec();
            b.current = sumQ.next() ? sumQ.value(0).toDouble() : 0;
            
            list.append(b);
        }
    }
    return list;
}

QList<CategoryInfo> DataManager::expenseCategories() const {
    QList<CategoryInfo> list = {
        {"Продукты",     QColor("#10b981"), "🛒"},
        {"Транспорт",    QColor("#3b82f6"), "🚗"},
        {"Досуг",        QColor("#6366f1"), "🎮"},
        {"Здоровье",     QColor("#ef4444"), "💊"},
        {"Одежда",       QColor("#f59e0b"), "👗"},
        {"Кафе/Рестор.", QColor("#f97316"), "☕"},
        {"ЖКХ",          QColor("#06b6d4"), "🏠"},
        {"Прочее",       QColor("#64748b"), "📦"}
    };
    
    QSqlQuery q("SELECT name, color, icon FROM global_categories WHERE type='expense'");
    while (q.next()) list.append({q.value(0).toString(), QColor(q.value(1).toString()), q.value(2).toString()});
    
    QSqlQuery uq;
    uq.prepare("SELECT name, color, icon FROM user_categories WHERE user_id=? AND type='expense'");
    uq.addBindValue(m_currentUserId);
    if (uq.exec()) {
        while (uq.next()) list.append({uq.value(0).toString(), QColor(uq.value(1).toString()), uq.value(2).toString()});
    }
    
    return list;
}

QList<CategoryInfo> DataManager::incomeCategories() const {
    QList<CategoryInfo> list = {
        {"Зарплата",    QColor("#10b981"), "💰"},
        {"Фриланс",     QColor("#6366f1"), "💻"},
        {"Подарок",     QColor("#f59e0b"), "🎁"},
        {"Инвестиции",  QColor("#3b82f6"), "📈"},
        {"Прочее",      QColor("#64748b"), "📦"}
    };

    QSqlQuery q("SELECT name, color, icon FROM global_categories WHERE type='income'");
    while (q.next()) list.append({q.value(0).toString(), QColor(q.value(1).toString()), q.value(2).toString()});
    
    QSqlQuery uq;
    uq.prepare("SELECT name, color, icon FROM user_categories WHERE user_id=? AND type='income'");
    uq.addBindValue(m_currentUserId);
    if (uq.exec()) {
        while (uq.next()) list.append({uq.value(0).toString(), QColor(uq.value(1).toString()), uq.value(2).toString()});
    }
    
    return list;
}

QColor DataManager::colorForCategory(const QString &cat) const {
    for (auto &c : expenseCategories()) if (c.name == cat) return c.color;
    for (auto &c : incomeCategories()) if (c.name == cat) return c.color;
    return QColor("#9b8ea8");
}

QString DataManager::iconForCategory(const QString &cat) const {
    for (auto &c : expenseCategories()) if (c.name == cat) return c.icon;
    for (auto &c : incomeCategories()) if (c.name == cat) return c.icon;
    return "📦";
}

void DataManager::addUserCategory(const QString &name, const QString &type, const QString &color, const QString &icon) {
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO user_categories (user_id, name, type, color, icon) VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(m_currentUserId);
    q.addBindValue(name);
    q.addBindValue(type);
    q.addBindValue(color);
    q.addBindValue(icon);
    q.exec();
}

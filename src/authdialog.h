#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include "datamanager.h"

// ===== Диалог авторизации/регистрации =====
class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(DataManager *dm, QWidget *parent = nullptr);

    // Возвращает имя авторизованного пользователя
    QString loggedInUser() const { return m_username; }

private slots:
    void onLogin();
    void onRegister();
    void switchToRegister();
    void switchToLogin();

private:
    void setupUi();
    void applyDarkStyle();

    DataManager  *m_dm;
    QString       m_username;

    // Страница логина
    QWidget     *m_loginPage;
    QLineEdit   *m_loginUser;
    QLineEdit   *m_loginPass;
    QLabel      *m_loginError;

    // Страница регистрации
    QWidget     *m_regPage;
    QLineEdit   *m_regUser;
    QLineEdit   *m_regPass;
    QLineEdit   *m_regPass2;
    QLabel      *m_regError;

    QStackedWidget *m_stack;
};

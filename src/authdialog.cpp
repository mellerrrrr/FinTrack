#include "authdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QFrame>

AuthDialog::AuthDialog(DataManager *dm, QWidget *parent)
    : QDialog(parent), m_dm(dm)
{
    setWindowTitle("FinTrack");
    setFixedSize(400, 520);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUi();
    applyDarkStyle();
}

void AuthDialog::setupUi() {
    // Основной контейнер с тенью
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);

    auto *card = new QFrame(this);
    card->setObjectName("authCard");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 40, 40, 40);
    cardLayout->setSpacing(0);

    // Тень для карточки
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(60);
    shadow->setOffset(0, 20);
    shadow->setColor(QColor(0, 0, 0, 180));
    card->setGraphicsEffect(shadow);

    // Логотип / заголовок
    auto *logoLabel = new QLabel("💰 FinTrack", card);
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(logoLabel);
    cardLayout->addSpacing(6);

    auto *subLabel = new QLabel("Личный учёт финансов", card);
    subLabel->setObjectName("subLabel");
    subLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(subLabel);
    cardLayout->addSpacing(32);

    // ── Стек: страница логина / регистрации
    m_stack = new QStackedWidget(card);

    // ── СТРАНИЦА ЛОГИНА ──────────────────────────
    m_loginPage = new QWidget();
    auto *loginLayout = new QVBoxLayout(m_loginPage);
    loginLayout->setSpacing(12);
    loginLayout->setContentsMargins(0,0,0,0);

    m_loginUser = new QLineEdit();
    m_loginUser->setPlaceholderText("Логин");
    m_loginUser->setObjectName("authInput");

    m_loginPass = new QLineEdit();
    m_loginPass->setPlaceholderText("Пароль");
    m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginPass->setObjectName("authInput");

    auto *loginBtn = new QPushButton("Войти");
    loginBtn->setObjectName("primaryBtn");
    connect(loginBtn, &QPushButton::clicked, this, &AuthDialog::onLogin);
    connect(m_loginPass, &QLineEdit::returnPressed, this, &AuthDialog::onLogin);

    m_loginError = new QLabel("");
    m_loginError->setObjectName("errorLabel");
    m_loginError->setAlignment(Qt::AlignCenter);
    m_loginError->setWordWrap(true);

    auto *switchToRegBtn = new QPushButton("Нет аккаунта? Зарегистрироваться");
    switchToRegBtn->setObjectName("linkBtn");
    connect(switchToRegBtn, &QPushButton::clicked, this, &AuthDialog::switchToRegister);

    loginLayout->addWidget(m_loginUser);
    loginLayout->addWidget(m_loginPass);
    loginLayout->addSpacing(4);
    loginLayout->addWidget(loginBtn);
    loginLayout->addWidget(m_loginError);
    loginLayout->addStretch();
    loginLayout->addWidget(switchToRegBtn);

    // ── СТРАНИЦА РЕГИСТРАЦИИ ──────────────────────
    m_regPage = new QWidget();
    auto *regLayout = new QVBoxLayout(m_regPage);
    regLayout->setSpacing(12);
    regLayout->setContentsMargins(0,0,0,0);

    m_regUser = new QLineEdit();
    m_regUser->setPlaceholderText("Придумайте логин");
    m_regUser->setObjectName("authInput");

    m_regPass = new QLineEdit();
    m_regPass->setPlaceholderText("Придумайте пароль");
    m_regPass->setEchoMode(QLineEdit::Password);
    m_regPass->setObjectName("authInput");

    m_regPass2 = new QLineEdit();
    m_regPass2->setPlaceholderText("Повторите пароль");
    m_regPass2->setEchoMode(QLineEdit::Password);
    m_regPass2->setObjectName("authInput");

    auto *regBtn = new QPushButton("Создать аккаунт");
    regBtn->setObjectName("primaryBtn");
    connect(regBtn, &QPushButton::clicked, this, &AuthDialog::onRegister);

    m_regError = new QLabel("");
    m_regError->setObjectName("errorLabel");
    m_regError->setAlignment(Qt::AlignCenter);
    m_regError->setWordWrap(true);

    auto *switchToLoginBtn = new QPushButton("Уже есть аккаунт? Войти");
    switchToLoginBtn->setObjectName("linkBtn");
    connect(switchToLoginBtn, &QPushButton::clicked, this, &AuthDialog::switchToLogin);

    regLayout->addWidget(m_regUser);
    regLayout->addWidget(m_regPass);
    regLayout->addWidget(m_regPass2);
    regLayout->addSpacing(4);
    regLayout->addWidget(regBtn);
    regLayout->addWidget(m_regError);
    regLayout->addStretch();
    regLayout->addWidget(switchToLoginBtn);

    m_stack->addWidget(m_loginPage);
    m_stack->addWidget(m_regPage);

    cardLayout->addWidget(m_stack);
    root->addWidget(card);
}

void AuthDialog::applyDarkStyle() {
    setStyleSheet(R"(
        QDialog { background: transparent; }

        #authCard {
            background: #13131a;
            border-radius: 20px;
            border: 1px solid rgba(255,255,255,0.08);
        }

        #logoLabel {
            font-size: 28px;
            font-weight: 800;
            color: #f0f0f8;
        }

        #subLabel {
            font-size: 13px;
            color: #6b6b85;
        }

        #authInput {
            background: #1c1c27;
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: 10px;
            padding: 14px 16px;
            font-size: 14px;
            color: #f0f0f8;
            min-height: 48px;
        }
        #authInput:focus {
            border: 1px solid #7c6fff;
            background: #20202e;
        }
        #authInput::placeholder { color: #4a4a60; }

        #primaryBtn {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #7c6fff, stop:1 #5b4fd4);
            color: white;
            border: none;
            border-radius: 10px;
            padding: 14px;
            font-size: 15px;
            font-weight: 600;
            min-height: 50px;
        }
        #primaryBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #9080ff, stop:1 #7060e0);
        }
        #primaryBtn:pressed { background: #5040b0; }

        #errorLabel { color: #ff6b8a; font-size: 13px; min-height: 20px; }

        #linkBtn {
            background: transparent;
            border: none;
            color: #7c6fff;
            font-size: 13px;
            padding: 4px;
        }
        #linkBtn:hover { color: #9080ff; text-decoration: underline; }
    )");
}

void AuthDialog::onLogin() {
    QString user = m_loginUser->text().trimmed();
    QString pass = m_loginPass->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_loginError->setText("Введите логин и пароль");
        return;
    }

    if (m_dm->loginUser(user, pass)) {
        m_username = user;
        accept();
    } else {
        m_loginError->setText("Неверный логин или пароль");
    }
}

void AuthDialog::onRegister() {
    QString user  = m_regUser->text().trimmed();
    QString pass  = m_regPass->text();
    QString pass2 = m_regPass2->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_regError->setText("Заполните все поля");
        return;
    }
    if (pass != pass2) {
        m_regError->setText("Пароли не совпадают");
        return;
    }
    if (pass.length() < 4) {
        m_regError->setText("Пароль слишком короткий (мин. 4 символа)");
        return;
    }

    if (m_dm->registerUser(user, pass)) {
        m_dm->loginUser(user, pass);
        m_username = user;
        accept();
    } else {
        m_regError->setText("Логин уже занят");
    }
}

void AuthDialog::switchToRegister() {
    m_loginError->clear();
    m_stack->setCurrentIndex(1);
}

void AuthDialog::switchToLogin() {
    m_regError->clear();
    m_stack->setCurrentIndex(0);
}

#include "addtransactiondialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

AddTransactionDialog::AddTransactionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Новая транзакция");
    setMinimumWidth(440);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUi();
}

void AddTransactionDialog::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);

    auto *card = new QFrame(this);
    card->setObjectName("addCard");

    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 10);
    shadow->setColor(QColor(0,0,0,150));
    card->setGraphicsEffect(shadow);

    auto *mainLay = new QVBoxLayout(card);
    mainLay->setContentsMargins(32, 30, 32, 32);
    mainLay->setSpacing(24);

    // Заголовок + кнопка закрыть
    auto *titleRow = new QHBoxLayout();
    auto *title = new QLabel("Новая запись");
    title->setObjectName("addTitle");
    auto *closeBtn = new QPushButton("✕");
    closeBtn->setObjectName("closeBtn");
    closeBtn->setFixedSize(32, 32);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(closeBtn);
    mainLay->addLayout(titleRow);

    // Форма
    auto *form = new QWidget();
    auto *formLay = new QVBoxLayout(form); // Используем вертикальный лейаут для четкого разделения
    formLay->setContentsMargins(0, 0, 0, 0);
    formLay->setSpacing(18);

    auto makeField = [&](const QString &lbl, QWidget *widget) {
        auto *v = new QVBoxLayout();
        v->setSpacing(8);
        auto *l = new QLabel(lbl);
        l->setObjectName("fieldLabel");
        v->addWidget(l);
        v->addWidget(widget);
        formLay->addLayout(v);
    };

    // Тип
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem("🔴  Расход", "expense");
    m_typeCombo->addItem("🟢  Доход",  "income");
    m_typeCombo->setObjectName("styledCombo");
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddTransactionDialog::onTypeChanged);
    makeField("Тип операции", m_typeCombo);

    // Категория
    m_categoryCombo = new QComboBox();
    m_categoryCombo->setObjectName("styledCombo");
    for (auto &c : DataManager::expenseCategories())
        m_categoryCombo->addItem(c.icon + "  " + c.name, c.name);
    makeField("Категория", m_categoryCombo);

    // Сумма
    m_amountSpin = new QDoubleSpinBox();
    m_amountSpin->setObjectName("styledSpin");
    m_amountSpin->setRange(0.01, 10000000.0);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setSingleStep(100.0);
    m_amountSpin->setValue(0.0);
    m_amountSpin->setPrefix("₽ ");
    makeField("Сумма (руб.)", m_amountSpin);

    // Комментарий
    m_commentEdit = new QLineEdit();
    m_commentEdit->setObjectName("authInput");
    m_commentEdit->setPlaceholderText("Например: обед в кафе");
    makeField("Комментарий", m_commentEdit);

    mainLay->addWidget(form);

    // Ошибка
    m_errorLabel = new QLabel("");
    m_errorLabel->setObjectName("errorLabel");
    mainLay->addWidget(m_errorLabel);

    // Кнопка сохранить
    m_saveBtn = new QPushButton("Сохранить транзакцию");
    m_saveBtn->setObjectName("primaryBtn");
    connect(m_saveBtn, &QPushButton::clicked, this, &AddTransactionDialog::onSave);
    mainLay->addWidget(m_saveBtn);

    root->addWidget(card);

    // Стили
    setStyleSheet(R"(
        QDialog { background: transparent; }
        #fieldLabel { 
            color: #94a3b8; 
            font-size: 12px; 
            font-weight: 700; 
            text-transform: uppercase;
            letter-spacing: 0.5px;
            background: transparent;
            border: none;
            padding: 0;
            margin: 0;
        }

        #addCard {
            background: #1a1c29;
            border-radius: 28px;
            border: 1px solid rgba(255,255,255,0.08);
        }
        #addTitle { color: #f8fafc; font-size: 22px; font-weight: 800; letter-spacing: -0.5px; }

        #closeBtn {
            background: rgba(255,255,255,0.03);
            border: none;
            border-radius: 12px;
            color: #94a3b8;
            font-size: 14px;
        }
        #closeBtn:hover { background: rgba(255,255,255,0.08); color: #f8fafc; }

        #styledCombo, #styledSpin, #authInput {
            background: #232533;
            border: 1px solid rgba(255,255,255,0.05);
            border-radius: 12px;
            padding: 10px 16px;
            font-size: 14px;
            color: #f8fafc;
            min-height: 44px;
        }
        #styledCombo::drop-down { border: none; width: 30px; }
        #styledCombo QAbstractItemView {
            background: #232533;
            border: 1px solid rgba(255,255,255,0.1);
            color: #f8fafc;
            selection-background-color: #6366f1;
            outline: none;
        }
        #styledCombo:focus, #styledSpin:focus, #authInput:focus { 
            border-color: #6366f1; 
            background: #2a2c3d;
        }

        #primaryBtn {
            background: #6366f1;
            color: white; border: none; border-radius: 12px;
            padding: 14px; font-size: 15px; font-weight: 700;
            min-height: 50px;
            margin-top: 8px;
        }
        #primaryBtn:hover { background: #4f46e5; }
        #primaryBtn:pressed { background: #4338ca; }

        #errorLabel { color: #ef4444; font-size: 13px; font-weight: 500; background: transparent; }
    )");
}

// Меняем список категорий при смене типа операции
void AddTransactionDialog::onTypeChanged(int index) {
    m_categoryCombo->clear();
    bool isExpense = (m_typeCombo->itemData(index).toString() == "expense");
    auto cats = isExpense ? DataManager::expenseCategories() : DataManager::incomeCategories();
    for (auto &c : cats)
        m_categoryCombo->addItem(c.icon + "  " + c.name, c.name);
}

void AddTransactionDialog::onSave() {
    // Валидация суммы
    if (m_amountSpin->value() <= 0) {
        m_errorLabel->setText("Сумма должна быть больше нуля");
        return;
    }

    m_result.type     = m_typeCombo->currentData().toString();
    m_result.category = m_categoryCombo->currentData().toString();
    m_result.amount   = m_amountSpin->value();
    m_result.comment  = m_commentEdit->text().trimmed();
    m_result.date     = QDateTime::currentDateTime();

    accept();
}

Transaction AddTransactionDialog::result() const {
    return m_result;
}

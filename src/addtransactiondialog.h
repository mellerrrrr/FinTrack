#pragma once

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "datamanager.h"

// ===== Диалог добавления новой транзакции =====
class AddTransactionDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddTransactionDialog(DataManager *dm, QWidget *parent = nullptr);

    // Возвращает заполненную транзакцию (вызывать только после accept())
    Transaction result() const;

private slots:
    void onTypeChanged(int index);
    void onSave();

private:
    void setupUi();

    DataManager     *m_dm;

    QComboBox       *m_typeCombo;
    QComboBox       *m_categoryCombo;
    QDoubleSpinBox  *m_amountSpin;
    QLineEdit       *m_commentEdit;
    QPushButton     *m_saveBtn;
    QLabel          *m_errorLabel;

    Transaction      m_result;
};

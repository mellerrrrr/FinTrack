#include <QApplication>
#include <QFont>

#include "datamanager.h"
#include "authdialog.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Глобальный шрифт приложения
    QFont appFont("Segoe UI", 10);
    app.setFont(appFont);

    // Менеджер данных — передаётся и в AuthDialog, и в MainWindow
    DataManager dm;

    // Показываем диалог авторизации
    AuthDialog auth(&dm);
    if (auth.exec() != QDialog::Accepted)
        return 0; // пользователь закрыл окно

    QString username = auth.loggedInUser();

    // Загружаем данные авторизованного пользователя
    dm.setCurrentUser(username);

    // Открываем главное окно
    MainWindow win(&dm, username);
    win.show();

    return app.exec();
}

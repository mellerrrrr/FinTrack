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

    bool logoutRequested = true;
    while (logoutRequested) {
        logoutRequested = false;

        // Показываем диалог авторизации
        AuthDialog auth(&dm);
        if (auth.exec() != QDialog::Accepted)
            return 0; // пользователь закрыл окно или отменил вход

        QString username = auth.loggedInUser();

        // Загружаем данные авторизованного пользователя
        // dm.setCurrentUser(username); // No longer needed, loginUser sets current user

        // Открываем главное окно
        MainWindow win(&dm, username);
        
        // Если окно сигнализирует о выходе, ставим флаг и закрываем окно
        QObject::connect(&win, &MainWindow::logoutRequested, [&]() {
            logoutRequested = true;
            win.close();
        });

        win.show();
        app.exec(); // Ждем закрытия окна
    }

    return 0;
}

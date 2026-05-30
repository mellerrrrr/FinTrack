# 💰 FinTrack — Трекер расходов (Qt 6 + C++)

Курсовой проект. Десктопное приложение для учёта личных финансов.

## Стек
- **C++17**
- **Qt 6** (Widgets + Charts + Sql)
- **SQLite** — хранение данных
- **SHA-256** (QCryptographicHash) — хэширование паролей
- Паттерн **MVC** (DataManager — модель, MainWindow — вид/контроллер)

## Структура проекта
```
fintrack/
├── CMakeLists.txt
└── src/
    ├── main.cpp                  # Точка входа
    ├── datamanager.h / .cpp      # Модель: JSON, транзакции, пользователи
    ├── authdialog.h / .cpp       # Диалог логина / регистрации
    ├── addtransactiondialog.h/.cpp  # Диалог добавления транзакции
    └── mainwindow.h / .cpp       # Главное окно (дашборд + история)
```

## Сборка через CMake

### Требования
- Qt 6.2+ с модулями `Widgets` и `Charts`
- CMake 3.16+
- Компилятор с поддержкой C++17

### Windows (Qt Creator)
1. Открой `CMakeLists.txt` в Qt Creator
2. Выбери кит (Kit) с Qt 6
3. Нажми **Build** → **Run**

### Linux / macOS (командная строка)
```bash
cd fintrack
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build . -j4
./FinTrack
```

### Windows (командная строка, MinGW)
```cmd
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\mingw_64
cmake --build .
FinTrack.exe
```

## Файлы данных
Приложение создаёт файлы рядом с exe:
- `users.json` — список пользователей (логин + SHA-256 хэш пароля)
- `transactions_<username>.json` — транзакции каждого пользователя

## Функциональность

| Модуль | Описание |
|--------|----------|
| Авторизация | Регистрация / Вход, SHA-256 хэш пароля |
| Дашборд | Баланс, доходы, расходы, donut-диаграмма |
| Диаграмма | Переключение Расходы / Доходы, цвета по категориям |
| Категории | Список с процентами и суммами под диаграммой |
| История | Таблица всех транзакций, удаление |
| Добавление | Диалог с валидацией суммы > 0 |

## Дальнейшее расширение
- Добавить фильтрацию по периоду (день/неделя/месяц)
- Экран бюджетов с лимитами и progress-bar
- Экспорт в CSV/PDF
- Уведомления при превышении бюджета

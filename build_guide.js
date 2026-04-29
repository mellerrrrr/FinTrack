const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  HeadingLevel, AlignmentType, BorderStyle, WidthType, ShadingType,
  PageNumber, NumberFormat, LevelFormat, Header, Footer,
  ImageRun, ExternalHyperlink, PageBreak, TabStopType, TabStopPosition
} = require('docx');
const fs = require('fs');

// ── Цвета ──────────────────────────────────────
const C = {
  darkBg:    "0a0a0f",
  darkCard:  "13131a",
  accent:    "7c6fff",
  accentLight: "ede9ff",
  green:     "2dd4a0",
  greenLight:"d1faf1",
  red:       "ff6b8a",
  yellow:    "f5c542",
  yellowLight: "fff8dc",
  text:      "f0f0f8",
  muted:     "6b6b85",
  white:     "FFFFFF",
  border:    "e0e0e8",
  codeBg:    "1e1e2e",
  codeText:  "cdd6f4",
  stepBg:    "f5f3ff",
  stepBorder:"7c6fff",
  warnBg:    "fff8dc",
  warnBorder:"f5c542",
  tipBg:     "d1faf1",
  tipBorder: "2dd4a0",
};

// ── Хелперы ────────────────────────────────────
const border = (color = C.border) => ({
  top:    { style: BorderStyle.SINGLE, size: 1, color },
  bottom: { style: BorderStyle.SINGLE, size: 1, color },
  left:   { style: BorderStyle.SINGLE, size: 1, color },
  right:  { style: BorderStyle.SINGLE, size: 1, color },
});

const noBorder = () => ({
  top:    { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
  bottom: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
  left:   { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
  right:  { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
});

const cell = (children, w, fill, borders, vAlign) => new TableCell({
  children,
  width: { size: w, type: WidthType.DXA },
  shading: fill ? { fill, type: ShadingType.CLEAR } : undefined,
  borders: borders || border(),
  margins: { top: 100, bottom: 100, left: 160, right: 160 },
  verticalAlign: vAlign,
});

const p = (text, opts = {}) => new Paragraph({
  children: [new TextRun({ text, ...opts })],
  spacing: { after: opts.spacingAfter ?? 120 },
  alignment: opts.align,
});

const h1 = (text) => new Paragraph({
  heading: HeadingLevel.HEADING_1,
  children: [new TextRun({ text, bold: true, size: 36, color: "1a1a2e", font: "Arial" })],
  spacing: { before: 320, after: 160 },
  border: { bottom: { style: BorderStyle.SINGLE, size: 8, color: C.accent, space: 4 } },
});

const h2 = (text) => new Paragraph({
  heading: HeadingLevel.HEADING_2,
  children: [new TextRun({ text, bold: true, size: 28, color: "1a1a2e", font: "Arial" })],
  spacing: { before: 280, after: 120 },
});

const h3 = (text) => new Paragraph({
  heading: HeadingLevel.HEADING_3,
  children: [new TextRun({ text, bold: true, size: 24, color: "2d2d4e", font: "Arial" })],
  spacing: { before: 200, after: 80 },
});

// Код (однострочный или многострочный)
const code = (text) => new Paragraph({
  children: [new TextRun({
    text,
    font: "Courier New",
    size: 20,
    color: "c9d1d9",
  })],
  shading: { fill: "1e1e2e", type: ShadingType.CLEAR },
  border: { left: { style: BorderStyle.SINGLE, size: 12, color: C.accent, space: 6 } },
  indent: { left: 240 },
  spacing: { before: 40, after: 40 },
});

// Блок с несколькими строками кода
const codeBlock = (lines) => lines.map(line => code(line));

// Numbered step card
const stepCard = (num, title, desc) => new Table({
  width: { size: 9360, type: WidthType.DXA },
  columnWidths: [600, 8760],
  rows: [new TableRow({
    children: [
      cell([new Paragraph({
        children: [new TextRun({ text: String(num), bold: true, size: 36, color: C.white, font: "Arial" })],
        alignment: AlignmentType.CENTER,
      })], 600, C.accent, border(C.accent)),
      cell([
        new Paragraph({ children: [new TextRun({ text: title, bold: true, size: 24, color: "1a1a2e", font: "Arial" })], spacing: { after: 40 } }),
        new Paragraph({ children: [new TextRun({ text: desc, size: 20, color: "3a3a5e" })], spacing: { after: 0 } }),
      ], 8760, C.stepBg, border(C.accent)),
    ]
  })],
});

const spacer = (pts = 120) => new Paragraph({ children: [], spacing: { after: pts } });

// Предупреждение (желтый)
const warn = (text) => new Table({
  width: { size: 9360, type: WidthType.DXA },
  columnWidths: [480, 8880],
  rows: [new TableRow({
    children: [
      cell([p("⚠️", { spacingAfter: 0 })], 480, C.warnBg, border(C.warnBorder)),
      cell([p(text, { size: 20, spacingAfter: 0 })], 8880, C.warnBg, border(C.warnBorder)),
    ]
  })],
});

// Совет (зеленый)
const tip = (text) => new Table({
  width: { size: 9360, type: WidthType.DXA },
  columnWidths: [480, 8880],
  rows: [new TableRow({
    children: [
      cell([p("✅", { spacingAfter: 0 })], 480, C.tipBg, border(C.tipBorder)),
      cell([p(text, { size: 20, spacingAfter: 0 })], 8880, C.tipBg, border(C.tipBorder)),
    ]
  })],
});

// Строка шага с иконкой
const iconStep = (icon, title, body) => new Table({
  width: { size: 9360, type: WidthType.DXA },
  columnWidths: [560, 8800],
  rows: [new TableRow({
    children: [
      cell([new Paragraph({ children: [new TextRun({ text: icon, size: 32 })], alignment: AlignmentType.CENTER, spacing: { after: 0 } })],
        560, "f8f8ff", border(C.border)),
      cell([
        new Paragraph({ children: [new TextRun({ text: title, bold: true, size: 22, color: "1a1a2e" })], spacing: { after: 40 } }),
        new Paragraph({ children: [new TextRun({ text: body, size: 20, color: "4a4a6e" })], spacing: { after: 0 } }),
      ], 8800, "f8f8ff", border(C.border)),
    ]
  })],
});

// ── Заголовок документа ───────────────────────
const titleBlock = () => [
  new Table({
    width: { size: 9360, type: WidthType.DXA },
    columnWidths: [9360],
    rows: [new TableRow({
      children: [cell([
        new Paragraph({
          children: [new TextRun({ text: "💰 FinTrack", bold: true, size: 64, color: C.white, font: "Arial" })],
          alignment: AlignmentType.CENTER, spacing: { after: 80 },
        }),
        new Paragraph({
          children: [new TextRun({ text: "Руководство по сборке и запуску проекта", size: 28, color: "ccccee" })],
          alignment: AlignmentType.CENTER, spacing: { after: 80 },
        }),
        new Paragraph({
          children: [new TextRun({ text: "Qt 6 • C++17 • CMake • JSON", size: 22, color: "8888bb" })],
          alignment: AlignmentType.CENTER, spacing: { after: 0 },
        }),
      ], 9360, C.darkBg, noBorder())],
    })],
  }),
  spacer(200),
];

// ── Таблица - требования ──────────────────────
const reqTable = () => new Table({
  width: { size: 9360, type: WidthType.DXA },
  columnWidths: [2800, 3200, 3360],
  rows: [
    new TableRow({
      tableHeader: true,
      children: [
        cell([p("Компонент", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 2800, C.accent, border(C.accent)),
        cell([p("Минимальная версия", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 3200, C.accent, border(C.accent)),
        cell([p("Где скачать", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 3360, C.accent, border(C.accent)),
      ],
    }),
    ...[
      ["Qt Framework", "Qt 6.2+", "qt.io/download"],
      ["CMake", "3.16+", "cmake.org/download"],
      ["Компилятор C++", "MSVC 2019+ / MinGW 11+ / GCC 10+", "Идёт с Qt или VS"],
      ["Qt Creator (опц.)", "Любая актуальная", "qt.io/download"],
      ["Git (опц.)", "Любая", "git-scm.com"],
    ].map((row, i) => new TableRow({
      children: row.map((text, ci) => cell(
        [p(text, { size: 20, spacingAfter: 0 })],
        [2800, 3200, 3360][ci],
        i % 2 === 0 ? "f5f5ff" : C.white,
        border()
      )),
    })),
  ],
});

// ─────────────────────────────────────────────────────────────
// ДОКУМЕНТ
// ─────────────────────────────────────────────────────────────
const doc = new Document({
  numbering: {
    config: [
      { reference: "bullets", levels: [{ level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 540, hanging: 260 } } } }] },
    ]
  },
  styles: {
    default: { document: { run: { font: "Arial", size: 22 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 36, bold: true, font: "Arial", color: "1a1a2e" },
        paragraph: { spacing: { before: 320, after: 160 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 28, bold: true, font: "Arial", color: "1a1a2e" },
        paragraph: { spacing: { before: 280, after: 120 }, outlineLevel: 1 } },
      { id: "Heading3", name: "Heading 3", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 24, bold: true, font: "Arial", color: "2d2d4e" },
        paragraph: { spacing: { before: 200, after: 80 }, outlineLevel: 2 } },
    ]
  },

  sections: [{
    properties: {
      page: {
        size: { width: 12240, height: 15840 },
        margin: { top: 1080, right: 1080, bottom: 1080, left: 1080 },
      }
    },
    headers: {
      default: new Header({
        children: [new Table({
          width: { size: 10080, type: WidthType.DXA },
          columnWidths: [7000, 3080],
          rows: [new TableRow({ children: [
            cell([p("💰 FinTrack — Руководство по сборке", { size: 18, color: C.muted, spacingAfter: 0 })],
              7000, C.white, noBorder()),
            cell([p("Qt 6 / C++17 / CMake", { size: 18, color: C.muted, align: AlignmentType.RIGHT, spacingAfter: 0 })],
              3080, C.white, noBorder()),
          ]})],
        })],
      }),
    },
    footers: {
      default: new Footer({
        children: [new Table({
          width: { size: 10080, type: WidthType.DXA },
          columnWidths: [7000, 3080],
          rows: [new TableRow({ children: [
            cell([p("Курсовой проект по C++", { size: 18, color: C.muted, spacingAfter: 0 })],
              7000, C.white, noBorder()),
            cell([new Paragraph({
              children: [new TextRun({ text: "Стр. ", size: 18, color: C.muted }),
                         new TextRun({ children: [PageNumber.CURRENT], size: 18, color: C.muted })],
              alignment: AlignmentType.RIGHT, spacing: { after: 0 },
            })], 3080, C.white, noBorder()),
          ]})],
        })],
      }),
    },
    children: [

      // ══════════════════════════════════════
      //  ТИТУЛ
      // ══════════════════════════════════════
      ...titleBlock(),

      // ══════════════════════════════════════
      //  1. ТРЕБОВАНИЯ
      // ══════════════════════════════════════
      h1("1. Системные требования"),

      p("Перед сборкой убедись, что установлены все необходимые компоненты:", { size: 22 }),
      spacer(120),
      reqTable(),
      spacer(160),

      warn("Qt Charts — отдельный модуль. При установке Qt обязательно поставь галочку напротив Qt Charts в списке компонентов. Без него проект не соберётся."),
      spacer(200),

      // ══════════════════════════════════════
      //  2. УСТАНОВКА Qt
      // ══════════════════════════════════════
      h1("2. Установка Qt 6 (если ещё не установлен)"),

      h2("2.1 Скачивание Qt Online Installer"),
      ...codeBlock(["# Перейди на сайт и скачай Online Installer:", "https://www.qt.io/download-qt-installer"]),
      spacer(120),

      stepCard(1, "Открой установщик Qt", "Запусти скачанный .exe (Windows) или .run (Linux/macOS)"),
      spacer(80),
      stepCard(2, "Создай аккаунт Qt", "Зарегистрируйся на qt.io — это бесплатно для open source использования"),
      spacer(80),
      stepCard(3, "Выбери компоненты для установки", 'В разделе "Qt 6.x.x" отметь нужные компоненты (см. список ниже)'),
      spacer(160),

      h2("2.2 Обязательные компоненты для установки"),

      new Table({
        width: { size: 9360, type: WidthType.DXA },
        columnWidths: [3600, 5760],
        rows: [
          new TableRow({ tableHeader: true, children: [
            cell([p("Компонент", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 3600, C.darkCard, border(C.darkCard)),
            cell([p("Описание", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 5760, C.darkCard, border(C.darkCard)),
          ]}),
          ...[
            ["Qt 6.x.x → MSVC 2019 64-bit", "Основные Qt Widgets (Windows)"],
            ["Qt 6.x.x → MinGW 11.2.0 64-bit", "Альтернатива MSVC для Windows"],
            ["Qt 6.x.x → Qt Charts", "⚠️ ОБЯЗАТЕЛЬНО — модуль диаграмм"],
            ["Qt 6.x.x → Qt Core, Qt GUI, Qt Widgets", "Базовые модули (выбраны по умолчанию)"],
            ["Developer and Designer Tools → CMake", "Система сборки (или установи отдельно)"],
            ["Developer and Designer Tools → Qt Creator", "IDE (рекомендуется для курсача)"],
          ].map((row, i) => new TableRow({ children: [
            cell([p(row[0], { size: 19, bold: i < 2, spacingAfter: 0 })], 3600, i % 2 ? "f8f8ff" : C.white, border()),
            cell([p(row[1], { size: 19, color: i === 2 ? "cc0000" : "3a3a5e", spacingAfter: 0 })], 5760, i % 2 ? "f8f8ff" : C.white, border()),
          ]})),
        ],
      }),
      spacer(200),

      // ══════════════════════════════════════
      //  3. СПОСОБ А — Qt Creator
      // ══════════════════════════════════════
      h1("3. Сборка через Qt Creator (рекомендуется)"),

      tip("Это самый простой способ для курсача. Qt Creator сам найдёт Qt, настроит CMake и позволит сразу запустить проект."),
      spacer(160),

      h2("3.1 Открытие проекта"),

      stepCard(1, "Распакуй архив", "Распакуй FinTrack_Qt6.zip в любую папку, например C:\\Projects\\FinTrack"),
      spacer(80),
      stepCard(2, "Открой Qt Creator", "Запусти Qt Creator из меню Пуск или из папки Qt"),
      spacer(80),
      stepCard(3, "Открой CMakeLists.txt как проект", "Файл → Открыть файл или проект... → выбери файл CMakeLists.txt внутри папки fintrack"),
      spacer(80),
      stepCard(4, "Настрой Kit (набор инструментов)", 'В открывшемся диалоге "Configure Project" выбери Kit с Qt 6.x и нажми "Configure Project"'),
      spacer(160),

      h2("3.2 Сборка и запуск"),

      ...codeBlock([
        "// В нижней панели Qt Creator:",
        "// [▶ Запустить]  — собрать и запустить сразу",
        "// [🔨 Собрать]   — только сборка (Ctrl+B)",
        "// [▶]            — запуск собранного (Ctrl+R)",
      ]),
      spacer(120),

      new Table({
        width: { size: 9360, type: WidthType.DXA },
        columnWidths: [1800, 7560],
        rows: [
          new TableRow({ tableHeader: true, children: [
            cell([p("Кнопка", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 1800, C.accent, border(C.accent)),
            cell([p("Действие", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 7560, C.accent, border(C.accent)),
          ]}),
          ...[
            ["Ctrl + B", "Собрать проект (Build)"],
            ["Ctrl + R", "Запустить (Run)"],
            ["Ctrl + Shift + B", "Пересобрать всё (Rebuild All)"],
            ["F4", "Переключиться между .h и .cpp"],
            ["Ctrl + K", "Быстрый поиск файла/символа"],
          ].map((row, i) => new TableRow({ children: [
            cell([p(row[0], { size: 20, font: "Courier New", bold: true, spacingAfter: 0 })], 1800, i%2?"f8f8ff":C.white, border()),
            cell([p(row[1], { size: 20, spacingAfter: 0 })], 7560, i%2?"f8f8ff":C.white, border()),
          ]})),
        ],
      }),
      spacer(200),

      // ══════════════════════════════════════
      //  4. СПОСОБ Б — Командная строка
      // ══════════════════════════════════════
      h1("4. Сборка из командной строки"),

      h2("4.1 Windows (MinGW)"),

      p("Открой «Qt 6.x MinGW 64-bit» из меню Пуск — это консоль с настроенными путями к Qt.", { size: 22 }),
      spacer(100),
      ...codeBlock([
        "# 1. Перейди в папку проекта",
        "cd C:\\Projects\\FinTrack\\fintrack",
        "",
        "# 2. Создай папку сборки",
        "mkdir build",
        "cd build",
        "",
        "# 3. Сгенерируй файлы сборки",
        "cmake .. -G \"MinGW Makefiles\" -DCMAKE_BUILD_TYPE=Release",
        "",
        "# 4. Собери проект",
        "cmake --build . --parallel",
        "",
        "# 5. Запусти приложение",
        "FinTrack.exe",
      ]),
      spacer(160),

      h2("4.2 Windows (MSVC / Visual Studio)"),
      ...codeBlock([
        "# Открой «Developer Command Prompt for VS 2022»",
        "cd C:\\Projects\\FinTrack\\fintrack",
        "mkdir build && cd build",
        "",
        "cmake .. -G \"Visual Studio 17 2022\" -A x64 ^",
        "  -DCMAKE_PREFIX_PATH=C:\\Qt\\6.x.x\\msvc2019_64",
        "",
        "cmake --build . --config Release",
        "Release\\FinTrack.exe",
      ]),
      spacer(160),

      h2("4.3 Linux (Ubuntu / Debian)"),

      p("Сначала установи зависимости:", { size: 22 }),
      ...codeBlock([
        "sudo apt update",
        "sudo apt install cmake build-essential",
        "sudo apt install qt6-base-dev qt6-charts-dev",
        "# или через Qt Online Installer",
      ]),
      spacer(100),
      ...codeBlock([
        "cd ~/Projects/FinTrack/fintrack",
        "mkdir build && cd build",
        "",
        "cmake .. -DCMAKE_BUILD_TYPE=Release",
        "cmake --build . --parallel",
        "",
        "./FinTrack",
      ]),
      spacer(160),

      h2("4.4 macOS"),
      ...codeBlock([
        "# Установи Qt через Homebrew",
        "brew install qt@6",
        "",
        "cd ~/Projects/FinTrack/fintrack",
        "mkdir build && cd build",
        "",
        "cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)",
        "cmake --build . --parallel",
        "",
        "./FinTrack",
      ]),
      spacer(200),

      // ══════════════════════════════════════
      //  5. РАЗВЁРТЫВАНИЕ — windeployqt
      // ══════════════════════════════════════
      h1("5. Развёртывание на Windows (windeployqt)"),

      p("После сборки .exe не запустится на другом компьютере без DLL Qt. Используй windeployqt:", { size: 22 }),
      spacer(120),

      ...codeBlock([
        "# В консоли Qt MinGW:",
        "cd C:\\Projects\\FinTrack\\fintrack\\build",
        "",
        "windeployqt.exe FinTrack.exe",
        "",
        "# Теперь в папке build появятся все нужные DLL",
        "# Можно копировать всю папку на другой ПК",
      ]),
      spacer(120),

      tip("После windeployqt в папке появятся DLL Qt6Core, Qt6Widgets, Qt6Charts и другие — это нормально. Весь этот набор и нужно передавать вместе с .exe."),
      spacer(200),

      // ══════════════════════════════════════
      //  6. ТИПИЧНЫЕ ОШИБКИ
      // ══════════════════════════════════════
      h1("6. Частые ошибки и их решения"),

      new Table({
        width: { size: 9360, type: WidthType.DXA },
        columnWidths: [3600, 5760],
        rows: [
          new TableRow({ tableHeader: true, children: [
            cell([p("Ошибка", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 3600, "cc3366", border("cc3366")),
            cell([p("Решение", { bold: true, size: 20, color: C.white, spacingAfter: 0 })], 5760, "cc3366", border("cc3366")),
          ]}),
          ...[
            [
              "Could not find Qt6Charts",
              "Зайди в Qt Maintenance Tool → Add components → отметь Qt Charts для своей версии Qt"
            ],
            [
              "CMake Error: Could not find Qt6",
              "Укажи путь явно: cmake .. -DCMAKE_PREFIX_PATH=C:\\Qt\\6.x.x\\mingw_64"
            ],
            [
              "LNK2019 / undefined reference",
              "Не совпадает компилятор. Убедись что Kit в Qt Creator совпадает с типом сборки (MSVC vs MinGW)"
            ],
            [
              "The program can't start because Qt6Core.dll is missing",
              "Запусти windeployqt.exe FinTrack.exe в папке с .exe"
            ],
            [
              "No Kit selected / Kit не настроен",
              "Qt Creator → Tools → Kits → убедись что компилятор и Qt SDK указаны верно"
            ],
            [
              "cannot open source file 'QtCharts/QChartView'",
              "В CMakeLists.txt нужна строка: find_package(Qt6 REQUIRED COMPONENTS Charts)"
            ],
          ].map((row, i) => new TableRow({ children: [
            cell([p(row[0], { size: 18, font: "Courier New", color: "880000", spacingAfter: 0 })], 3600, i%2?"fff5f5":C.white, border()),
            cell([p(row[1], { size: 19, color: "2a4a2a", spacingAfter: 0 })], 5760, i%2?"fff5f5":C.white, border()),
          ]})),
        ],
      }),
      spacer(200),

      // ══════════════════════════════════════
      //  7. СТРУКТУРА ПРОЕКТА
      // ══════════════════════════════════════
      h1("7. Структура проекта"),

      ...codeBlock([
        "fintrack/",
        "├── CMakeLists.txt              # Конфигурация сборки",
        "├── README.md                   # Краткая документация",
        "└── src/",
        "    ├── main.cpp                # Точка входа: запуск AuthDialog → MainWindow",
        "    ├── datamanager.h/.cpp      # Модель: JSON, транзакции, пользователи, SHA-256",
        "    ├── authdialog.h/.cpp       # Диалог логина и регистрации",
        "    ├── addtransactiondialog.h/.cpp  # Диалог добавления транзакции",
        "    └── mainwindow.h/.cpp       # Главное окно: дашборд + история",
      ]),
      spacer(120),

      p("Файлы данных создаются рядом с .exe при первом запуске:", { size: 22 }),
      ...codeBlock([
        "users.json                     # Логины + SHA-256 хэши паролей",
        "transactions_<username>.json   # Транзакции каждого пользователя",
      ]),
      spacer(200),

      // ══════════════════════════════════════
      //  8. ПЕРВЫЙ ЗАПУСК
      // ══════════════════════════════════════
      h1("8. Первый запуск приложения"),

      iconStep("🚀", "Запусти приложение", "Появится окно авторизации FinTrack"),
      spacer(80),
      iconStep("📝", "Зарегистрируй аккаунт", "Нажми «Нет аккаунта? Зарегистрироваться», введи логин и пароль (мин. 4 символа)"),
      spacer(80),
      iconStep("🔑", "Войди в систему", "После регистрации автоматически откроется главное окно"),
      spacer(80),
      iconStep("➕", "Добавь первую транзакцию", "Нажми жёлтую кнопку «+» на дашборде или «+ Добавить» на странице истории"),
      spacer(80),
      iconStep("📊", "Посмотри аналитику", "На дашборде появится доnut-диаграмма с распределением по категориям"),
      spacer(160),

      warn("Данные сохраняются в файлы JSON рядом с .exe. Не удаляй users.json и файлы transactions_*.json — иначе потеряешь все данные."),
      spacer(200),

      // ══════════════════════════════════════
      //  9. СЛЕДУЮЩИЕ ШАГИ
      // ══════════════════════════════════════
      h1("9. Планируемые улучшения"),

      new Table({
        width: { size: 9360, type: WidthType.DXA },
        columnWidths: [600, 8760],
        rows: [
          ...[
            ["🗄️", "Замена JSON на SQLite (QSqlDatabase + QSqlQuery) для лучшей производительности"],
            ["📅", "Фильтрация транзакций по периоду: день / неделя / месяц / год"],
            ["💸", "Экран бюджетов: лимиты по категориям и прогресс-бары"],
            ["📈", "Столбчатая диаграмма расходов по месяцам (QBarSeries)"],
            ["📤", "Экспорт данных в CSV или PDF"],
            ["🔔", "Уведомления при превышении установленного бюджета"],
          ].map((row, i) => new TableRow({ children: [
            cell([p(row[0], { size: 22, spacingAfter: 0, align: AlignmentType.CENTER })], 600, i%2?"f5f3ff":C.stepBg, border(C.stepBorder)),
            cell([p(row[1], { size: 20, color: "2d2d4e", spacingAfter: 0 })], 8760, i%2?"f5f3ff":C.stepBg, border(C.stepBorder)),
          ]})),
        ],
      }),
      spacer(200),

      // ══════════════════════════════════════
      //  Финальная пометка
      // ══════════════════════════════════════
      new Table({
        width: { size: 9360, type: WidthType.DXA },
        columnWidths: [9360],
        rows: [new TableRow({ children: [
          cell([
            new Paragraph({
              children: [new TextRun({ text: "Курсовой проект по C++ • Qt 6 Widgets • MVC • JSON Storage", size: 18, color: "8888bb" })],
              alignment: AlignmentType.CENTER, spacing: { after: 0 },
            }),
          ], 9360, C.darkBg, noBorder()),
        ]}),],
      }),
    ],
  }],
});

Packer.toBuffer(doc).then(buffer => {
  fs.writeFileSync("/mnt/user-data/outputs/FinTrack_Руководство_по_сборке.docx", buffer);
  console.log("Done!");
});

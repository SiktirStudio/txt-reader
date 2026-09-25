#include "mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPageSetupDialog>
#include <QPointer>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QSaveFile>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QShowEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifndef QT_NO_PRINTER
#include <QPageSetupDialog>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#endif

#include "editor.h"
#include "findpanel.h"
#include "highlighter.h"
#include "icons.h"
#include "languages.h"
#include "search.h"
#include "fileio.h"

namespace {

const char *kOrgName = "SiktirStudio";
const char *kAppName = "NovaPad";

QStringList readFileFilter()
{
    return { QObject::tr("All files (*)"),
             QObject::tr("Text files (*.txt *.md *.log *.csv *.ini *.cfg)"),
             QObject::tr("Source files (*.cpp *.cc *.cxx *.h *.hpp *.hh *.hxx *.c *.py *.js *.ts *.tsx *.jsx *.json *.html *.htm *.xml *.css *.scss *.java *.cs *.go *.rs *.sh *.bash *.cmake)") };
}

// Notepad++-style toggle comment, shared by the menu action and the self test.
void toggleLineComment(Editor *editor, const LanguageDef *lang)
{
    if (!editor || !lang || lang->lineComments.isEmpty())
        return;
    const QString marker = lang->lineComments.first();
    QTextDocument *doc = editor->document();

    const QTextCursor cur = editor->textCursor();
    const QTextBlock first = doc->findBlock(cur.selectionStart());
    const QTextBlock last = doc->findBlock(cur.selectionEnd());
    if (!first.isValid() || !last.isValid())
        return;

    bool allCommented = true;
    for (QTextBlock b = first; b.isValid() && b.blockNumber() <= last.blockNumber(); b = b.next()) {
        const QString line = b.text();
        if (line.trimmed().isEmpty())
            continue;
        if (!line.trimmed().startsWith(marker)) {
            allCommented = false;
            break;
        }
    }

    QTextCursor k(doc);
    k.beginEditBlock();
    for (QTextBlock b = first; b.isValid() && b.blockNumber() <= last.blockNumber(); b = b.next()) {
        const QString line = b.text();
        if (line.trimmed().isEmpty())
            continue;
        if (allCommented) {
            const int idx = line.indexOf(marker);
            int removeLen = marker.size();
            if (idx + removeLen < line.size() && line.at(idx + removeLen) == QLatin1Char(' '))
                ++removeLen;
            if (idx >= 0) {
                k.setPosition(b.position() + idx);
                k.setPosition(b.position() + idx + removeLen, QTextCursor::KeepAnchor);
                k.removeSelectedText();
            }
        } else {
            int indent = 0;
            while (indent < line.size()
                   && (line.at(indent) == QLatin1Char(' ') || line.at(indent) == QLatin1Char('\t')))
                ++indent;
            k.setPosition(b.position() + indent);
            k.insertText(marker + QLatin1Char(' '));
        }
    }
    k.endEditBlock();
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    setWindowIcon(QIcon(QStringLiteral(":/appicon.png")));

    m_iconDir = QDir(QDir::temp().absoluteFilePath(
                         QStringLiteral("novapad-style-%1").arg(QCoreApplication::applicationPid())))
                    .absolutePath();
    QDir().mkpath(m_iconDir);

    m_editorFont = QFont(QStringLiteral("JetBrains Mono"));
    m_editorFont.setStyleHint(QFont::Monospace);
    m_editorFontSize = 11;

    setupUi();
    setupMenus();
    setupStatusBar();
    setupConnections();

    readSettings();
    if (m_tabs.isEmpty())
        addTab(QString());
    updateWindowTitle();
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    for (TabData *t : m_tabs) {
        delete t->highlighter;
        t->highlighter = nullptr;
        delete t->editor;
        delete t;
    }
    m_tabs.clear();
}

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------

void MainWindow::setupUi()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(false);
    m_tabWidget->setElideMode(Qt::ElideRight);
    setCentralWidget(m_tabWidget);

    auto *newTabButton = new QToolButton(m_tabWidget);
    newTabButton->setAutoRaise(true);
    newTabButton->setToolTip(tr("New tab (Ctrl+N)"));
    m_tabWidget->setCornerWidget(newTabButton, Qt::TopRightCorner);
    m_btnNewTab = newTabButton;
    connect(newTabButton, &QToolButton::clicked, this, &MainWindow::onNewFile);

    m_tabWidget->tabBar()->installEventFilter(this);

    m_toolbar = addToolBar(tr("Main"));
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(20, 20));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_findPanel = new FindPanel(this);
    m_findPanel->hide();

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &MainWindow::onFileChangedExternally);
}

void MainWindow::setupMenus()
{
    QMenuBar *mb = menuBar();

    // ---- helper lambdas -----------------------------------------------------
    auto plainAct = [this](const QString &text, const QKeySequence &shortcut, auto fn) {
        QAction *a = new QAction(text, this);
        if (!shortcut.isEmpty())
            a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, fn);
        return a;
    };
    Q_UNUSED(plainAct)

    auto item = [this](QMenu *menu, const QString &text, const QKeySequence &shortcut, auto fn) {
        QAction *a = new QAction(text, this);
        if (!shortcut.isEmpty())
            a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, fn);
        menu->addAction(a);
        return a;
    };
    auto iconItem = [this, &item](QMenu *menu, const QString &text, const QKeySequence &shortcut,
                                  QIcon (*factory)(const QColor &), auto fn) {
        QAction *a = item(menu, text, shortcut, fn);
        a->setIcon(factory(m_theme.text));
        m_toolIcons.append({ a, factory });
        return a;
    };

    // ---- File ---------------------------------------------------------------
    QMenu *fileMenu = mb->addMenu(tr("&File"));
    QAction *actNew = iconItem(fileMenu, tr("&New"), QKeySequence::New, Icons::fileNew,
                               [this] { onNewFile(); });
    iconItem(fileMenu, tr("New &Window"), QKeySequence(QStringLiteral("Ctrl+Shift+N")), Icons::docs,
             [this] { newWindow(); });
    QAction *actOpen = iconItem(fileMenu, tr("&Open..."), QKeySequence::Open, Icons::fileOpen,
                                [this] { onOpenDialog(); });

    m_menuRecent = fileMenu->addMenu(tr("Recent &Files"));
    refreshRecentMenu();

    fileMenu->addSeparator();
    m_actSave = iconItem(fileMenu, tr("&Save"), QKeySequence::Save, Icons::fileSave,
                         [this] { onSave(); });
    iconItem(fileMenu, tr("Save &As..."), QKeySequence(QStringLiteral("Ctrl+Shift+S")), Icons::fileSave,
             [this] { onSaveAs(); });
    QAction *actSaveAll = iconItem(fileMenu, tr("Save A&ll"),
                                   QKeySequence(QStringLiteral("Ctrl+Alt+S")), Icons::fileSaveAll,
                                   [this] { onSaveAll(); });

    fileMenu->addSeparator();
    item(fileMenu, tr("Close Ta&b"), QKeySequence(QStringLiteral("Ctrl+W")),
         [this] { onCloseCurrent(); });
    item(fileMenu, tr("Close &All Tabs"), QKeySequence(QStringLiteral("Ctrl+Shift+W")),
         [this] { onCloseAll(); });

    fileMenu->addSeparator();
    QAction *pageSetup = item(fileMenu, tr("Page Set&up..."), QKeySequence(), [this] { onPageSetup(); });
    QAction *printPreview = item(fileMenu, tr("Print Pre&view..."), QKeySequence(), [this] { onPrintPreview(); });
    iconItem(fileMenu, tr("&Print..."), QKeySequence::Print, Icons::filePrint, [this] { onPrint(); });
#ifndef QT_NO_PRINTER
    Q_UNUSED(pageSetup)
    Q_UNUSED(printPreview)
#else
    pageSetup->setEnabled(false);
    printPreview->setEnabled(false);
#endif

    fileMenu->addSeparator();
    item(fileMenu, tr("E&xit"), QKeySequence(QStringLiteral("Alt+F4")), [this] { close(); });

    // ---- Edit ---------------------------------------------------------------
    QMenu *editMenu = mb->addMenu(tr("&Edit"));
    QAction *actUndo = iconItem(editMenu, tr("&Undo"), QKeySequence::Undo, Icons::editUndo, [this] {
        if (Editor *e = currentEditor()) e->undo();
    });
    QAction *actRedo = iconItem(editMenu, tr("&Redo"), QKeySequence::Redo, Icons::editRedo, [this] {
        if (Editor *e = currentEditor()) e->redo();
    });
    actRedo->setShortcuts({ QKeySequence(QStringLiteral("Ctrl+Y")),
                            QKeySequence(QStringLiteral("Ctrl+Shift+Z")) });

    editMenu->addSeparator();
    item(editMenu, tr("Cu&t"), QKeySequence::Cut, [this] {
        if (Editor *e = currentEditor()) e->cut();
    });
    item(editMenu, tr("&Copy"), QKeySequence::Copy, [this] {
        if (Editor *e = currentEditor()) e->copy();
    });
    item(editMenu, tr("&Paste"), QKeySequence::Paste, [this] {
        if (Editor *e = currentEditor()) e->paste();
    });
    item(editMenu, tr("De&lete"), QKeySequence(QStringLiteral("Del")), [this] {
        if (Editor *e = currentEditor()) {
            QTextCursor c = e->textCursor();
            if (c.hasSelection()) {
                c.removeSelectedText();
                e->setTextCursor(c);
            }
        }
    });

    editMenu->addSeparator();
    QAction *actFind = iconItem(editMenu, tr("&Find..."), QKeySequence::Find, Icons::editFind,
                                [this] { onFind(); });
    item(editMenu, tr("Find &Next"), QKeySequence::FindNext, [this] { onFindNext(); });
    item(editMenu, tr("Find &Previous"), QKeySequence(QStringLiteral("Shift+F3")),
         [this] { onFindPrevious(); });
    QAction *actReplace = iconItem(editMenu, tr("Re&place..."), QKeySequence(QStringLiteral("Ctrl+H")),
                                   Icons::editReplace, [this] { onReplace(); });
    item(editMenu, tr("&Go To Line..."), QKeySequence(QStringLiteral("Ctrl+G")), [this] { onGoTo(); });

    editMenu->addSeparator();
    item(editMenu, tr("Select &All"), QKeySequence::SelectAll, [this] {
        if (Editor *e = currentEditor()) e->selectAll();
    });
    item(editMenu, tr("Time/&Date"), QKeySequence(QStringLiteral("F5")), [this] { onTimeDate(); });

    editMenu->addSeparator();
    item(editMenu, tr("&Duplicate Line"), QKeySequence(QStringLiteral("Ctrl+D")), [this] {
        if (Editor *e = currentEditor()) e->duplicateSelectionOrLine();
    });
    item(editMenu, tr("Delete Li&ne"), QKeySequence(QStringLiteral("Ctrl+L")), [this] {
        if (Editor *e = currentEditor()) e->deleteLines();
    });
    item(editMenu, tr("Move Line &Up"), QKeySequence(QStringLiteral("Alt+Up")), [this] {
        if (Editor *e = currentEditor()) e->moveLines(-1);
    });
    item(editMenu, tr("Move Line &Down"), QKeySequence(QStringLiteral("Alt+Down")), [this] {
        if (Editor *e = currentEditor()) e->moveLines(1);
    });
    item(editMenu, tr("Toggle &Comment"), QKeySequence(QStringLiteral("Ctrl+/")), [this] {
        if (TabData *t = currentTab()) toggleLineComment(t->editor, t->highlighter->language());
    });
    item(editMenu, tr("Tri&m Trailing Whitespace"), QKeySequence(), [this] {
        if (Editor *e = currentEditor()) e->trimTrailingWhitespace();
    });

    editMenu->addSeparator();
    item(editMenu, tr("&UPPERCASE"), QKeySequence(QStringLiteral("Ctrl+Shift+U")), [this] {
        if (Editor *e = currentEditor()) e->transformCase(Editor::UpperCase);
    });
    item(editMenu, tr("&lowercase"), QKeySequence(QStringLiteral("Ctrl+Shift+L")), [this] {
        if (Editor *e = currentEditor()) e->transformCase(Editor::LowerCase);
    });
    item(editMenu, tr("&Title Case"), QKeySequence(), [this] {
        if (Editor *e = currentEditor()) e->transformCase(Editor::TitleCase);
    });
    item(editMenu, tr("iN&VERT cASE"), QKeySequence(), [this] {
        if (Editor *e = currentEditor()) e->transformCase(Editor::InvertCase);
    });

    // ---- Format -------------------------------------------------------------
    QMenu *formatMenu = mb->addMenu(tr("F&ormat"));
    m_actWrap = formatMenu->addAction(tr("&Word Wrap"));
    m_actWrap->setCheckable(true);
    m_actWrap->setChecked(true);
    m_actWrap->setShortcut(QStringLiteral("Ctrl+Shift+W")); // will be adjusted below (kept unique)
    m_actWrap->setShortcut(QKeySequence());
    connect(m_actWrap, &QAction::toggled, this, &MainWindow::onWordWrapToggled);

    m_menuEol = formatMenu->addMenu(tr("Line &Ending"));
    auto *eolGroup = new QActionGroup(this);
    QAction *eolCrlf = m_menuEol->addAction(tr("Windows (CRLF)"));
    eolCrlf->setCheckable(true);
    eolCrlf->setData(true);
    QAction *eolLf = m_menuEol->addAction(tr("Unix (LF)"));
    eolLf->setCheckable(true);
    eolLf->setData(false);
    eolGroup->addAction(eolCrlf);
    eolGroup->addAction(eolLf);
    connect(eolGroup, &QActionGroup::triggered, this, &MainWindow::onEolChanged);

    m_menuEncoding = formatMenu->addMenu(tr("&Encoding"));
    auto *encGroup = new QActionGroup(this);
    QAction *encUtf8 = m_menuEncoding->addAction(tr("UTF-8"));
    encUtf8->setCheckable(true);
    encUtf8->setData(false);
    QAction *encBom = m_menuEncoding->addAction(tr("UTF-8 with BOM"));
    encBom->setCheckable(true);
    encBom->setData(true);
    encGroup->addAction(encUtf8);
    encGroup->addAction(encBom);
    connect(encGroup, &QActionGroup::triggered, this, &MainWindow::onEncodingChanged);

    formatMenu->addSeparator();
    item(formatMenu, tr("&Font..."), QKeySequence(), [this] { onFontDialog(); });

    // ---- View ---------------------------------------------------------------
    QMenu *viewMenu = mb->addMenu(tr("&View"));
    QAction *zoomIn = viewMenu->addAction(tr("Zoom &In"));
    zoomIn->setShortcuts({ QKeySequence(QStringLiteral("Ctrl+=")),
                           QKeySequence(QStringLiteral("Ctrl++")) });
    connect(zoomIn, &QAction::triggered, this, &MainWindow::onZoomIn);

    QAction *zoomOut = viewMenu->addAction(tr("Zoom &Out"));
    zoomOut->setShortcuts({ QKeySequence(QStringLiteral("Ctrl+-")),
                            QKeySequence(QStringLiteral("Ctrl+_")) });
    connect(zoomOut, &QAction::triggered, this, &MainWindow::onZoomOut);

    QAction *zoomReset = viewMenu->addAction(tr("&Restore Default Zoom"));
    zoomReset->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(zoomReset, &QAction::triggered, this, &MainWindow::onZoomReset);

    viewMenu->addSeparator();
    m_actLineNumbers = viewMenu->addAction(tr("&Line Numbers"));
    m_actLineNumbers->setCheckable(true);
    m_actLineNumbers->setChecked(true);
    connect(m_actLineNumbers, &QAction::toggled, this, &MainWindow::onLineNumbersToggled);

    m_actStatusbar = viewMenu->addAction(tr("&Status Bar"));
    m_actStatusbar->setCheckable(true);
    m_actStatusbar->setChecked(true);
    connect(m_actStatusbar, &QAction::toggled, this, &MainWindow::onStatusbarToggled);

    viewMenu->addSeparator();
    m_menuTheme = viewMenu->addMenu(tr("&Theme"));
    auto *themeGroup = new QActionGroup(this);
    QAction *themeDark = m_menuTheme->addAction(QStringLiteral("Nova Dark"));
    themeDark->setCheckable(true);
    themeDark->setData(true);
    QAction *themeLight = m_menuTheme->addAction(QStringLiteral("Nova Light"));
    themeLight->setCheckable(true);
    themeLight->setData(false);
    themeGroup->addAction(themeDark);
    themeGroup->addAction(themeLight);
    connect(themeGroup, &QActionGroup::triggered, this, &MainWindow::onThemeChanged);
    themeDark->setChecked(true);

    viewMenu->addSeparator();
    item(viewMenu, tr("Document &Statistics..."), QKeySequence(), [this] { onStats(); });

    // ---- Language -----------------------------------------------------------
    m_menuLanguage = mb->addMenu(tr("&Language"));
    auto *langGroup = new QActionGroup(this);
    QAction *plain = m_menuLanguage->addAction(Languages::PlainText.name);
    plain->setCheckable(true);
    plain->setData(Languages::PlainText.name);
    langGroup->addAction(plain);
    for (const LanguageDef &def : Languages::all()) {
        QAction *a = m_menuLanguage->addAction(def.name);
        a->setCheckable(true);
        a->setData(def.name);
        langGroup->addAction(a);
    }
    connect(langGroup, &QActionGroup::triggered, this, &MainWindow::onLanguageChanged);
    plain->setChecked(true);

    // ---- Help ---------------------------------------------------------------
    QMenu *helpMenu = mb->addMenu(tr("&Help"));
    item(helpMenu, tr("&About NovaPad"), QKeySequence(), [this] { onAbout(); });
    item(helpMenu, tr("About &Qt"), QKeySequence(), [this] { onAboutQt(); });

    // ---- Toolbar ------------------------------------------------------------
    m_toolbar->addAction(actNew);
    m_toolbar->addAction(actOpen);
    m_toolbar->addAction(m_actSave);
    m_toolbar->addSeparator();
    m_toolbar->addAction(actUndo);
    m_toolbar->addAction(actRedo);
    m_toolbar->addSeparator();
    m_toolbar->addAction(actFind);
    m_toolbar->addAction(actReplace);
    m_toolbar->addSeparator();
    m_toolbar->addAction(actSaveAll);
}

// ---------------------------------------------------------------------------
// Status bar / global connections
// ---------------------------------------------------------------------------

void MainWindow::setupStatusBar()
{
    QStatusBar *sb = statusBar();
    sb->clearMessage();

    auto label = [this]() {
        QLabel *l = new QLabel(this);
        l->setTextInteractionFlags(Qt::NoTextInteraction);
        return l;
    };
    m_lblPos = label();
    m_lblSel = label();
    m_lblLines = label();
    m_lblChars = label();
    m_lblEncoding = label();
    m_lblEol = label();
    m_lblLang = label();
    m_lblZoom = label();

    sb->addPermanentWidget(m_lblPos);
    sb->addPermanentWidget(m_lblSel);
    sb->addPermanentWidget(m_lblLines);
    sb->addPermanentWidget(m_lblChars);
    sb->addPermanentWidget(m_lblEncoding);
    sb->addPermanentWidget(m_lblEol);
    sb->addPermanentWidget(m_lblLang);
    sb->addPermanentWidget(m_lblZoom);
}

void MainWindow::setupConnections()
{
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this,
            static_cast<void (MainWindow::*)(int)>(&MainWindow::closeTab));

    connect(m_findPanel, &FindPanel::findNextRequested, this, &MainWindow::onFindNext);
    connect(m_findPanel, &FindPanel::findPreviousRequested, this, &MainWindow::onFindPrevious);
    connect(m_findPanel, &FindPanel::replaceRequested, this, &MainWindow::onDoReplace);
    connect(m_findPanel, &FindPanel::replaceAllRequested, this, &MainWindow::onReplaceAll);
    connect(m_findPanel, &FindPanel::criteriaChanged, this, &MainWindow::onFindCriteriaChanged);
    connect(m_findPanel, &FindPanel::panelClosed, this, &MainWindow::onPanelClosed);

    auto *nextTab = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")), this);
    connect(nextTab, &QShortcut::activated, this, [this]() {
        const int count = m_tabWidget->count();
        if (count > 1)
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % count);
    });
    auto *prevTab = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Tab")), this);
    connect(prevTab, &QShortcut::activated, this, [this]() {
        const int count = m_tabWidget->count();
        if (count > 1)
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() - 1 + count) % count);
    });
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this]() {
        if (m_findPanel->isVisible())
            m_findPanel->closePanel();
    });
}

// ---------------------------------------------------------------------------
// Tab management
// ---------------------------------------------------------------------------

MainWindow::TabData *MainWindow::tabForWidget(QWidget *widget)
{
    for (TabData *t : m_tabs)
        if (t->editor == widget)
            return t;
    return nullptr;
}

MainWindow::TabData *MainWindow::currentTab()
{
    return tabForWidget(m_tabWidget->currentWidget());
}

Editor *MainWindow::currentEditor()
{
    TabData *t = currentTab();
    return t ? t->editor : nullptr;
}

MainWindow::TabData *MainWindow::addTab(const QString &filePath)
{
    auto *tab = new TabData;
    tab->editor = new Editor(this);
    tab->editor->setCustomFont(m_editorFont, m_editorFontSize);
    tab->editor->setPalette(m_theme.editor);
    tab->editor->setLineWrapMode(m_actWrap && m_actWrap->isChecked()
                                     ? QPlainTextEdit::WidgetWidth
                                     : QPlainTextEdit::NoWrap);
    tab->editor->setShowLineNumbers(m_actLineNumbers && m_actLineNumbers->isChecked());

    tab->highlighter = new SyntaxHighlighter(tab->editor->document());
    tab->highlighter->setColors(m_theme.syntax);
    if (filePath.isEmpty()) {
        tab->untitledNumber = ++m_untitledCounter;
#ifdef Q_OS_WIN
        tab->crlf = true;
#else
        tab->crlf = false;
#endif
        tab->encodingLabel = QStringLiteral("UTF-8");
    }

    connect(tab->editor->document(), &QTextDocument::modificationChanged, this,
            [this, tab](bool) {
                if (m_tabs.contains(tab))
                    updateTabTitle(*tab);
                updateWindowTitle();
            });
    connect(tab->editor, &QPlainTextEdit::cursorPositionChanged, this, [this, tab]() {
        if (m_tabWidget->currentWidget() == tab->editor) {
            updateStatusBar();
            updateWordHighlights();
        }
    });
    connect(tab->editor, &QPlainTextEdit::textChanged, this, [this, tab]() {
        if (m_tabWidget->currentWidget() == tab->editor) {
            updateStatusBar();
            if (m_findPanel->isVisible())
                updateSearchHighlights();
        }
    });

    const int index = m_tabWidget->addTab(tab->editor, tabDisplayName(*tab));
    m_tabWidget->setTabToolTip(index, filePath);
    m_tabs.append(tab);
    m_tabWidget->setCurrentIndex(index);
    return tab;
}

void MainWindow::removeTabDirect(TabData *tab)
{
    if (!tab)
        return;
    m_tabs.removeAll(tab);
    if (!tab->filePath.isEmpty() && m_watcher->files().contains(tab->filePath))
        m_watcher->removePath(tab->filePath);
    const int index = m_tabWidget->indexOf(tab->editor);
    if (index >= 0)
        m_tabWidget->removeTab(index);
    delete tab->highlighter;
    tab->highlighter = nullptr;
    tab->editor->deleteLater();
    delete tab;
}

void MainWindow::closeTab(int index)
{
    if (index < 0 || index >= m_tabWidget->count())
        return;
    QWidget *w = m_tabWidget->widget(index);
    TabData *tab = tabForWidget(w);
    if (!tab)
        return;
    if (!maybeSaveTab(*tab))
        return;
    removeTabDirect(tab);
    if (m_tabs.isEmpty())
        addTab(QString());
    onTabChanged(m_tabWidget->currentIndex());
}

void MainWindow::onCloseCurrent()
{
    closeTab(m_tabWidget->currentIndex());
}

void MainWindow::onCloseOthers()
{
    TabData *keep = currentTab();
    const QList<TabData *> tabs = m_tabs;
    for (TabData *t : tabs) {
        if (t == keep)
            continue;
        if (!maybeSaveTab(*t))
            return;
    }
    for (TabData *t : tabs) {
        if (t != keep)
            removeTabDirect(t);
    }
}

void MainWindow::onCloseAll()
{
    const QList<TabData *> tabs = m_tabs;
    for (TabData *t : tabs) {
        if (!maybeSaveTab(*t))
            return;
    }
    for (TabData *t : tabs)
        removeTabDirect(t);
    addTab(QString());
}

QString MainWindow::tabDisplayName(const TabData &tab) const
{
    if (!tab.filePath.isEmpty())
        return QFileInfo(tab.filePath).fileName();
    return tab.untitledNumber > 1 ? tr("Untitled %1").arg(tab.untitledNumber) : tr("Untitled");
}

void MainWindow::updateTabTitle(TabData &tab)
{
    const int index = m_tabWidget->indexOf(tab.editor);
    if (index < 0)
        return;
    QString title = tabDisplayName(tab);
    if (tab.editor->document()->isModified())
        title += QStringLiteral(" *");
    m_tabWidget->setTabText(index, title);
    m_tabWidget->setTabToolTip(index, tab.filePath.isEmpty() ? title : tab.filePath);
}

void MainWindow::updateWindowTitle()
{
    TabData *tab = currentTab();
    if (!tab) {
        setWindowTitle(QStringLiteral("NovaPad"));
        return;
    }
    QString title = tabDisplayName(*tab);
    if (tab->editor->document()->isModified())
        title += QStringLiteral(" *");
    setWindowTitle(title + QStringLiteral(" — NovaPad"));
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index)
    updateWindowTitle();
    updateStatusBar();
    updateWordHighlights();
    updateSearchHighlights();

    // Sync per-tab menus
    if (TabData *tab = currentTab()) {
        const LanguageDef *lang = tab->highlighter->language();
        const QString langName = lang ? lang->name : Languages::PlainText.name;
        for (QAction *a : m_menuLanguage->actions()) {
            if (a->data().toString() == langName) {
                a->setChecked(true);
                break;
            }
        }
        for (QAction *a : m_menuEol->actions())
            a->setChecked(a->data().toBool() == tab->crlf);
        for (QAction *a : m_menuEncoding->actions())
            a->setChecked(a->data().toBool() == tab->bom);
    }
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------

void MainWindow::onNewFile()
{
    addTab(QString());
}

void MainWindow::onOpenDialog()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Open Files"), lastUsedDir(), readFileFilter().join(QStringLiteral(";;")));
    for (const QString &f : files)
        loadFileIntoTab(f);
}

bool MainWindow::openPaths(const QStringList &paths)
{
    bool any = false;
    for (const QString &p : paths)
        any |= loadFileIntoTab(p);
    return any;
}

bool MainWindow::loadFileIntoTab(const QString &path)
{
    const QString canonical = QFileInfo(path).absoluteFilePath();

    for (TabData *t : m_tabs) {
        if (t->filePath == canonical) {
            m_tabWidget->setCurrentWidget(t->editor);
            return true;
        }
    }

    QFile file(canonical);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open File"),
                             tr("Could not open %1:\n%2").arg(QDir::toNativeSeparators(path),
                                                              file.errorString()));
        return false;
    }
    const QByteArray raw = file.readAll();
    file.close();

    const FileIo::Decoded decoded = FileIo::decode(raw);
    TabData *tab = addTab(QString());
    tab->filePath = canonical;
    tab->bom = decoded.bom;
    tab->crlf = decoded.crlf;
    tab->encodingLabel = decoded.encodingLabel;
    tab->editor->setPlainText(decoded.text);
    tab->editor->document()->setModified(false);
    rehighlightTab(*tab);

    if (!m_watcher->files().contains(canonical))
        m_watcher->addPath(canonical);

    pushRecentFile(canonical);
    updateTabTitle(*tab);
    updateWindowTitle();
    updateStatusBar();
    statusBar()->showMessage(tr("Opened %1").arg(QDir::toNativeSeparators(canonical)), 2500);
    return true;
}

void MainWindow::rehighlightTab(TabData &tab)
{
    if (!tab.highlighter || !tab.editor)
        return;
    const LanguageDef *lang = tab.filePath.isEmpty() ? &Languages::PlainText
                                                     : Languages::detect(tab.filePath);
    tab.highlighter->setLanguage(lang);
    tab.editor->setEolLabel(tab.crlf ? QStringLiteral("CRLF") : QStringLiteral("LF"));
}

bool MainWindow::saveTab(TabData &tab, bool saveAs)
{
    QString path = tab.filePath;
    if (path.isEmpty() || saveAs) {
        const QString suggested = path.isEmpty()
            ? (tab.untitledNumber > 1
                   ? QStringLiteral("Untitled%1.txt").arg(tab.untitledNumber)
                   : QStringLiteral("Untitled.txt"))
            : QFileInfo(path).fileName();
        path = QFileDialog::getSaveFileName(this, saveAs ? tr("Save As") : tr("Save"),
                                            QDir(lastUsedDir()).filePath(suggested),
                                            readFileFilter().join(QStringLiteral(";;")));
        if (path.isEmpty())
            return false;
        path = QFileInfo(path).absoluteFilePath();
    }

    if (m_watcher->files().contains(path))
        m_watcher->removePath(path);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Save File"),
                             tr("Could not save %1:\n%2").arg(QDir::toNativeSeparators(path),
                                                              file.errorString()));
        if (!m_watcher->files().contains(path))
            m_watcher->addPath(path);
        return false;
    }
    const QByteArray data = FileIo::encode(tab.editor->toPlainText(), tab.crlf, tab.bom);
    file.write(data);
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Save File"),
                             tr("Could not save %1:\n%2").arg(QDir::toNativeSeparators(path),
                                                              file.errorString()));
        return false;
    }

    const bool pathChanged = (tab.filePath != path);
    tab.filePath = path;
    tab.encodingLabel = tab.bom ? QStringLiteral("UTF-8-BOM") : QStringLiteral("UTF-8");
    tab.editor->document()->setModified(false);
    if (pathChanged)
        rehighlightTab(tab);
    if (!m_watcher->files().contains(path))
        m_watcher->addPath(path);
    pushRecentFile(path);
    updateTabTitle(tab);
    updateWindowTitle();
    updateStatusBar();
    statusBar()->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(path)), 2500);
    return true;
}

bool MainWindow::maybeSaveTab(TabData &tab)
{
    if (!tab.editor->document()->isModified())
        return true;
    const QString name = tabDisplayName(tab);
    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this, tr("Save Changes"),
        tr("Do you want to save the changes to \"%1\"?").arg(name),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return saveTab(tab, false);
    return ret != QMessageBox::Cancel;
}

void MainWindow::onSave()
{
    if (TabData *tab = currentTab())
        saveTab(*tab, false);
}

void MainWindow::onSaveAs()
{
    if (TabData *tab = currentTab())
        saveTab(*tab, true);
}

void MainWindow::onSaveAll()
{
    for (TabData *tab : m_tabs) {
        if (tab->editor->document()->isModified() || tab->filePath.isEmpty())
            saveTab(*tab, false);
    }
}

void MainWindow::onFileChangedExternally(const QString &path)
{
    TabData *found = nullptr;
    for (TabData *t : m_tabs) {
        if (t->filePath == path) {
            found = t;
            break;
        }
    }
    if (!found)
        return;

    if (!QFile::exists(path)) {
        statusBar()->showMessage(tr("%1 was deleted or moved.").arg(QDir::toNativeSeparators(path)),
                                 4000);
        return;
    }
    if (!m_watcher->files().contains(path))
        m_watcher->addPath(path);

    if (found->editor->document()->isModified()) {
        const auto ret = QMessageBox::question(
            this, tr("File Changed"),
            tr("\"%1\" was changed on disk.\nReload it and discard your changes?")
                .arg(QDir::toNativeSeparators(path)),
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const FileIo::Decoded decoded = FileIo::decode(file.readAll());
    file.close();

    Editor *e = found->editor;
    const int pos = qMin(e->textCursor().position(), decoded.text.size());
    e->setPlainText(decoded.text);
    QTextCursor c(e->document());
    c.setPosition(pos);
    e->setTextCursor(c);
    e->document()->setModified(false);
    found->bom = decoded.bom;
    found->crlf = decoded.crlf;
    found->encodingLabel = decoded.encodingLabel;
    updateTabTitle(*found);
    updateStatusBar();
}

void MainWindow::newWindow()
{
    QProcess::startDetached(QApplication::applicationFilePath(), {});
}

// ---------------------------------------------------------------------------
// Printing
// ---------------------------------------------------------------------------

void MainWindow::onPrint()
{
#ifndef QT_NO_PRINTER
    QPointer<Editor> e = currentEditor();
    if (!e)
        return;
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    if (dialog.exec() == QDialog::Accepted)
        e->print(&printer);
#endif
}

void MainWindow::onPrintPreview()
{
#ifndef QT_NO_PRINTER
    QPointer<Editor> e = currentEditor();
    if (!e)
        return;
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle(tr("Print Preview"));
    connect(&preview, &QPrintPreviewDialog::paintRequested, this,
            [e](QPrinter *p) { if (e) e->print(p); });
    preview.exec();
#endif
}

void MainWindow::onPageSetup()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPageSetupDialog dialog(&printer, this);
    dialog.exec();
#endif
}

// ---------------------------------------------------------------------------
// Find / Replace
// ---------------------------------------------------------------------------

void MainWindow::onFind()
{
    Editor *e = currentEditor();
    QString seed;
    if (e) {
        const QTextCursor c = e->textCursor();
        if (c.hasSelection() && !c.selectedText().contains(QChar(0x2029))
            && c.selectedText().size() <= 200)
            seed = c.selectedText();
    }
    m_findPanel->openFind(seed);
    updateSearchHighlights();
}

void MainWindow::onReplace()
{
    Editor *e = currentEditor();
    QString seed;
    if (e) {
        const QTextCursor c = e->textCursor();
        if (c.hasSelection() && !c.selectedText().contains(QChar(0x2029))
            && c.selectedText().size() <= 200)
            seed = c.selectedText();
    }
    m_findPanel->openReplace(seed);
    updateSearchHighlights();
}

void MainWindow::onFindCriteriaChanged()
{
    m_currentMatch = -1;
    updateSearchHighlights();
}

void MainWindow::onPanelClosed()
{
    for (TabData *t : m_tabs)
        if (t->editor)
            t->editor->setSearchSelections({});
    m_matches.clear();
    m_currentMatch = -1;
    if (Editor *e = currentEditor())
        e->setFocus();
}

void MainWindow::updateSearchHighlights()
{
    Editor *e = currentEditor();

    for (TabData *t : m_tabs)
        if (t->editor && t->editor != e)
            t->editor->setSearchSelections({});

    if (!e || !m_findPanel->isVisible() || m_findPanel->findText().isEmpty()) {
        if (e)
            e->setSearchSelections({});
        m_matches.clear();
        m_currentMatch = -1;
        m_findPanel->setStatusText(QString());
        return;
    }

    const QRegularExpression re = Search::buildPattern(m_findPanel->findText(),
                                                       m_findPanel->matchCase(),
                                                       m_findPanel->wholeWord(),
                                                       m_findPanel->useRegex());
    if (!re.isValid()) {
        m_findPanel->setStatusText(tr("Invalid pattern"));
        m_matches.clear();
        e->setSearchSelections({});
        return;
    }

    m_matches = Search::findAll(e->document(), re, 10000);
    if (m_currentMatch < 0 || m_currentMatch >= m_matches.size())
        m_currentMatch = -1;

    QList<QTextEdit::ExtraSelection> selections;
    const int paintLimit = qMin(m_matches.size(), 2000);
    for (int i = 0; i < paintLimit; ++i) {
        QTextEdit::ExtraSelection s;
        s.format.setBackground(m_theme.editor.searchHighlight);
        s.cursor = m_matches.at(i);
        selections.append(s);
    }
    if (m_currentMatch >= 0 && m_currentMatch < paintLimit) {
        QTextEdit::ExtraSelection s;
        s.format.setBackground(m_theme.editor.searchCurrent);
        s.cursor = m_matches.at(m_currentMatch);
        selections.append(s);
    }
    e->setSearchSelections(selections);

    if (m_matches.isEmpty())
        m_findPanel->setStatusText(tr("No results"));
    else if (m_currentMatch >= 0)
        m_findPanel->setStatusText(tr("%1 of %2").arg(m_currentMatch + 1).arg(m_matches.size()));
    else
        m_findPanel->setStatusText(tr("%1 results").arg(m_matches.size()));
}

void MainWindow::selectMatch(int index)
{
    if (index < 0 || index >= m_matches.size())
        return;
    m_currentMatch = index;
    Editor *e = currentEditor();
    if (!e)
        return;
    e->setTextCursor(m_matches.at(index));
    e->ensureCursorVisible();
    updateSearchHighlights();
}

void MainWindow::onFindNext()
{
    Editor *e = currentEditor();
    if (!e || m_findPanel->findText().isEmpty())
        return;
    if (m_matches.isEmpty())
        return;
    const int cursorPos = e->textCursor().selectionEnd();
    int idx = -1;
    for (int i = 0; i < m_matches.size(); ++i) {
        if (m_matches.at(i).selectionStart() >= cursorPos) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        idx = 0; // wrap around
    selectMatch(idx);
}

void MainWindow::onFindPrevious()
{
    Editor *e = currentEditor();
    if (!e || m_findPanel->findText().isEmpty() || m_matches.isEmpty())
        return;
    const int cursorPos = e->textCursor().selectionStart();
    int idx = -1;
    for (int i = m_matches.size() - 1; i >= 0; --i) {
        if (m_matches.at(i).selectionStart() < cursorPos) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        idx = m_matches.size() - 1; // wrap around
    selectMatch(idx);
}

void MainWindow::onDoReplace()
{
    Editor *e = currentEditor();
    if (!e || m_findPanel->findText().isEmpty())
        return;

    const QRegularExpression re = Search::buildPattern(m_findPanel->findText(),
                                                       m_findPanel->matchCase(),
                                                       m_findPanel->wholeWord(),
                                                       m_findPanel->useRegex());
    if (!re.isValid()) {
        m_findPanel->setStatusText(tr("Invalid pattern"));
        return;
    }

    QTextCursor c = e->textCursor();
    if (c.hasSelection()) {
        const QRegularExpressionMatch m = re.match(e->toPlainText(), c.selectionStart());
        if (m.hasMatch() && m.capturedStart() == c.selectionStart()
            && m.capturedEnd() == c.selectionEnd()) {
            c.insertText(Search::expandBackrefs(m_findPanel->replaceText(), m));
        }
    }
    onFindNext();
}

void MainWindow::onReplaceAll()
{
    Editor *e = currentEditor();
    if (!e || m_findPanel->findText().isEmpty())
        return;

    const QRegularExpression re = Search::buildPattern(m_findPanel->findText(),
                                                       m_findPanel->matchCase(),
                                                       m_findPanel->wholeWord(),
                                                       m_findPanel->useRegex());
    if (!re.isValid()) {
        m_findPanel->setStatusText(tr("Invalid pattern"));
        return;
    }
    const int count = Search::replaceAll(e->document(), re, m_findPanel->replaceText());
    updateSearchHighlights();
    statusBar()->showMessage(count == 1 ? tr("Replaced 1 occurrence")
                                        : tr("Replaced %n occurrence(s)", "", count),
                             3000);
}

void MainWindow::updateWordHighlights()
{
    Editor *e = currentEditor();
    if (!e)
        return;
    QList<QTextEdit::ExtraSelection> selections;
    const QTextCursor c = e->textCursor();
    if (c.hasSelection()) {
        const QString sel = c.selectedText();
        if (sel.size() >= 1 && sel.size() <= 100) {
            static const QRegularExpression wordRe(QStringLiteral("^\\w+$"));
            if (wordRe.match(sel).hasMatch()) {
                const QRegularExpression re(
                    QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(sel)));
                const QList<QTextCursor> matches = Search::findAll(e->document(), re, 300);
                for (const QTextCursor &m : matches) {
                    QTextEdit::ExtraSelection s;
                    s.format.setBackground(m_theme.editor.wordHighlight);
                    s.cursor = m;
                    selections.append(s);
                }
            }
        }
    }
    e->setWordSelections(selections);
}

// ---------------------------------------------------------------------------
// Edit helpers & dialogs
// ---------------------------------------------------------------------------

void MainWindow::onGoTo()
{
    Editor *e = currentEditor();
    if (!e)
        return;

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Go To Line"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *label = new QLabel(tr("Line number (1 - %1):").arg(e->blockCount()), &dialog);
    auto *spin = new QSpinBox(&dialog);
    spin->setRange(1, qMax(1, e->blockCount()));
    spin->setValue(e->textCursor().blockNumber() + 1);
    spin->setFocus();
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(label);
    layout->addWidget(spin);
    layout->addWidget(buttons);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    if (dialog.exec() == QDialog::Accepted) {
        QTextCursor c(e->document());
        const QTextBlock block = e->document()->findBlockByNumber(spin->value() - 1);
        if (block.isValid()) {
            c.setPosition(block.position());
            e->setTextCursor(c);
            e->ensureCursorVisible();
            e->setFocus();
        }
    }
}

void MainWindow::onTimeDate()
{
    Editor *e = currentEditor();
    if (!e)
        return;
    const QLocale locale;
    const QString stamp = locale.toString(QTime::currentTime(), QLocale::ShortFormat) + QLatin1Char(' ')
        + locale.toString(QDate::currentDate(), QLocale::ShortFormat);
    e->insertPlainText(stamp);
}

void MainWindow::onFontDialog()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, m_editorFont, this, tr("Select Editor Font"));
    if (!ok)
        return;
    m_editorFont = font;
    m_editorFontSize = font.pointSize() > 0 ? font.pointSize() : 11;
    for (TabData *t : m_tabs)
        t->editor->setCustomFont(m_editorFont, m_editorFontSize);
    updateStatusBar();
}

void MainWindow::onStats()
{
    Editor *e = currentEditor();
    if (!e)
        return;
    const QString text = e->toPlainText();
    const int lines = e->blockCount();
    const int chars = text.size();
    int charsNoSpace = 0;
    int words = 0;
    bool inWord = false;
    for (const QChar ch : text) {
        if (!ch.isSpace())
            ++charsNoSpace;
        const bool wordChar = ch.isLetterOrNumber() || ch == QLatin1Char('_');
        if (wordChar && !inWord)
            ++words;
        inWord = wordChar;
    }
    QMessageBox::information(
        this, tr("Document Statistics"),
        tr("Lines: %1\nWords: %2\nCharacters: %3\nCharacters (no spaces): %4\n\nFile: %5")
            .arg(QLocale().toString(lines))
            .arg(QLocale().toString(words))
            .arg(QLocale().toString(chars))
            .arg(QLocale().toString(charsNoSpace))
            .arg(currentTab()->filePath.isEmpty() ? tr("unsaved")
                                                  : QDir::toNativeSeparators(currentTab()->filePath)));
}

void MainWindow::onAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("About NovaPad"));
    box.setTextFormat(Qt::RichText);
    box.setText(QStringLiteral(
        "<p style=\"font-size:16pt; font-weight:600; margin-bottom:2px;\">NovaPad %1</p>"
        "<p>A modern Notepad / Notepad++ style text editor.</p>"
        "<p>Built with C++17 and Qt %2.</p>"
        "<p style=\"color:%3;\">https://github.com/SiktirStudio/txt-reader</p>")
        .arg(QString::fromLatin1(NOVAPAD_VERSION), QString::fromLatin1(qVersion()),
             m_theme.textMuted.name()));
    box.setIconPixmap(QPixmap(QStringLiteral(":/appicon.png"))
                          .scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    box.exec();
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

// ---------------------------------------------------------------------------
// View actions
// ---------------------------------------------------------------------------

void MainWindow::onZoomIn()
{
    for (TabData *t : m_tabs)
        t->editor->zoomStep(1);
    updateStatusBar();
}

void MainWindow::onZoomOut()
{
    for (TabData *t : m_tabs)
        t->editor->zoomStep(-1);
    updateStatusBar();
}

void MainWindow::onZoomReset()
{
    for (TabData *t : m_tabs)
        t->editor->resetZoom();
    updateStatusBar();
}

void MainWindow::onWordWrapToggled(bool on)
{
    for (TabData *t : m_tabs)
        t->editor->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void MainWindow::onLineNumbersToggled(bool on)
{
    for (TabData *t : m_tabs)
        t->editor->setShowLineNumbers(on);
}

void MainWindow::onStatusbarToggled(bool on)
{
    statusBar()->setVisible(on);
    positionFindPanel();
}

void MainWindow::onThemeChanged(QAction *action)
{
    applyTheme(action->data().toBool());
}

void MainWindow::onEolChanged(QAction *action)
{
    TabData *tab = currentTab();
    if (!tab)
        return;
    tab->crlf = action->data().toBool();
    tab->editor->setEolLabel(tab->crlf ? QStringLiteral("CRLF") : QStringLiteral("LF"));
    updateStatusBar();
}

void MainWindow::onEncodingChanged(QAction *action)
{
    TabData *tab = currentTab();
    if (!tab)
        return;
    tab->bom = action->data().toBool();
    tab->encodingLabel = tab->bom ? QStringLiteral("UTF-8-BOM") : QStringLiteral("UTF-8");
    updateStatusBar();
}

void MainWindow::onLanguageChanged(QAction *action)
{
    TabData *tab = currentTab();
    if (!tab)
        return;
    const LanguageDef *lang = Languages::byName(action->data().toString());
    if (lang)
        tab->highlighter->setLanguage(lang);
    updateStatusBar();
}

// ---------------------------------------------------------------------------
// Recent files
// ---------------------------------------------------------------------------

void MainWindow::pushRecentFile(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 10)
        m_recentFiles.removeLast();
    refreshRecentMenu();
}

void MainWindow::refreshRecentMenu()
{
    if (!m_menuRecent)
        return;
    m_menuRecent->clear();
    if (m_recentFiles.isEmpty()) {
        QAction *empty = m_menuRecent->addAction(tr("No recent files"));
        empty->setEnabled(false);
        return;
    }
    for (int i = 0; i < m_recentFiles.size(); ++i) {
        const QString &path = m_recentFiles.at(i);
        QAction *a = m_menuRecent->addAction(
            QStringLiteral("%1  %2").arg(i + 1).arg(QDir::toNativeSeparators(path)));
        a->setToolTip(path);
        connect(a, &QAction::triggered, this, [this, path] { loadFileIntoTab(path); });
    }
    m_menuRecent->addSeparator();
    QAction *clear = m_menuRecent->addAction(tr("&Clear Recent Files"));
    connect(clear, &QAction::triggered, this, [this]() {
        m_recentFiles.clear();
        refreshRecentMenu();
    });
}

void MainWindow::openRecentFile()
{
    // handled per-action above
}

void MainWindow::clearRecentFiles()
{
    m_recentFiles.clear();
    refreshRecentMenu();
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------

void MainWindow::updateStatusBar()
{
    TabData *tab = currentTab();
    Editor *e = tab ? tab->editor : nullptr;
    if (!e || !tab) {
        m_lblPos->clear();
        m_lblSel->clear();
        m_lblLines->clear();
        m_lblChars->clear();
        m_lblEncoding->clear();
        m_lblEol->clear();
        m_lblLang->clear();
        m_lblZoom->clear();
        return;
    }
    const QTextCursor c = e->textCursor();
    m_lblPos->setText(tr("Ln %1, Col %2").arg(c.blockNumber() + 1).arg(c.columnNumber() + 1));
    m_lblSel->setText(c.hasSelection() ? tr("Sel %1").arg(c.selectedText().size()) : QString());
    m_lblLines->setText(tr("%1 lines").arg(QLocale().toString(e->blockCount())));
    m_lblChars->setText(
        tr("%1 chars").arg(QLocale().toString(qMax(0, e->document()->characterCount() - 1))));
    m_lblEncoding->setText(tab->encodingLabel);
    m_lblEol->setText(tab->crlf ? QStringLiteral("CRLF") : QStringLiteral("LF"));
    const LanguageDef *lang = tab->highlighter->language();
    m_lblLang->setText(lang ? lang->name : QString());
    m_lblZoom->setText(
        QStringLiteral("%1%").arg(qRound(e->fontSize() * 100.0 / e->baseFontSize())));
}

void MainWindow::onCursorPositionChanged()
{
    updateStatusBar();
    updateWordHighlights();
}

void MainWindow::onModificationChanged(bool modified)
{
    Q_UNUSED(modified)
    updateWindowTitle();
}

void MainWindow::onTextChanged()
{
    updateStatusBar();
}

void MainWindow::onBlockCountChanged(int count)
{
    Q_UNUSED(count)
    updateStatusBar();
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

void MainWindow::applyTheme(bool dark)
{
    m_dark = dark;
    m_theme = dark ? Themes::dark() : Themes::light();

    const QString dir = QDir(m_iconDir).filePath(dark ? QStringLiteral("dark")
                                                      : QStringLiteral("light"));
    QDir().mkpath(dir);
    Icons::writeCheckIcon(QDir(dir).filePath(QStringLiteral("check.png")), m_theme.accentText);
    Icons::writeCloseIcon(QDir(dir).filePath(QStringLiteral("close.png")), m_theme.textMuted);
    Icons::writeCloseIcon(QDir(dir).filePath(QStringLiteral("close-hover.png")), m_theme.text);
    Icons::writeArrowIcon(QDir(dir).filePath(QStringLiteral("arrow.png")), m_theme.textMuted);

    qApp->setStyleSheet(m_theme.styleSheet(dir));

    for (TabData *t : m_tabs) {
        t->editor->setPalette(m_theme.editor);
        t->highlighter->setColors(m_theme.syntax);
    }
    m_findPanel->setIconColors(m_theme.text);
    m_btnNewTab->setIcon(Icons::plus(m_theme.text));
    for (const ToolIcon &ti : m_toolIcons)
        ti.action->setIcon(ti.factory(m_theme.text));

    for (QAction *a : m_menuTheme->actions())
        a->setChecked(a->data().toBool() == dark);
}

void MainWindow::rebuildToolbarIcons()
{
    for (const ToolIcon &ti : m_toolIcons)
        ti.action->setIcon(ti.factory(m_theme.text));
    m_btnNewTab->setIcon(Icons::plus(m_theme.text));
}

// ---------------------------------------------------------------------------
// Settings / session
// ---------------------------------------------------------------------------

void MainWindow::readSettings()
{
    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("ui/geometry")).toByteArray());

    const bool dark = settings.value(QStringLiteral("ui/dark"), true).toBool();
    applyTheme(dark);

    const bool wrap = settings.value(QStringLiteral("ui/wrap"), true).toBool();
    m_actWrap->setChecked(wrap);
    onWordWrapToggled(wrap);

    const bool lineNumbers = settings.value(QStringLiteral("ui/lineNumbers"), true).toBool();
    m_actLineNumbers->setChecked(lineNumbers);
    onLineNumbersToggled(lineNumbers);

    const bool statusbar = settings.value(QStringLiteral("ui/statusbar"), true).toBool();
    m_actStatusbar->setChecked(statusbar);
    onStatusbarToggled(statusbar);

    const QString family = settings.value(QStringLiteral("ui/fontFamily"),
                                          QStringLiteral("JetBrains Mono")).toString();
    const int size = settings.value(QStringLiteral("ui/fontSize"), 11).toInt();
    m_editorFont = QFont(family, size > 0 ? size : 11);
    m_editorFont.setStyleHint(QFont::Monospace);
    m_editorFontSize = size > 0 ? size : 11;
    m_recentFiles = settings.value(QStringLiteral("recent/files")).toStringList();
    refreshRecentMenu();

    const QStringList files = settings.value(QStringLiteral("session/files")).toStringList();
    bool any = false;
    for (const QString &f : files) {
        if (QFile::exists(f))
            any |= loadFileIntoTab(f);
    }
    if (any) {
        const int active = settings.value(QStringLiteral("session/active"), 0).toInt();
        if (active >= 0 && active < m_tabWidget->count())
            m_tabWidget->setCurrentIndex(active);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("ui/dark"), m_dark);
    settings.setValue(QStringLiteral("ui/wrap"), m_actWrap->isChecked());
    settings.setValue(QStringLiteral("ui/lineNumbers"), m_actLineNumbers->isChecked());
    settings.setValue(QStringLiteral("ui/statusbar"), m_actStatusbar->isChecked());
    settings.setValue(QStringLiteral("ui/fontFamily"), m_editorFont.family());
    settings.setValue(QStringLiteral("ui/fontSize"), m_editorFontSize);
    settings.setValue(QStringLiteral("recent/files"), m_recentFiles);

    QStringList files;
    for (TabData *t : m_tabs) {
        if (!t->filePath.isEmpty())
            files << t->filePath;
    }
    settings.setValue(QStringLiteral("session/files"), files);
    settings.setValue(QStringLiteral("session/active"), m_tabWidget->currentIndex());
}

QString MainWindow::lastUsedDir() const
{
    if (!m_recentFiles.isEmpty())
        return QFileInfo(m_recentFiles.first()).absolutePath();
    return QDir::homePath();
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *event)
{
    const QList<TabData *> tabs = m_tabs;
    for (TabData *t : tabs) {
        if (!maybeSaveTab(*t)) {
            event->ignore();
            return;
        }
    }
    writeSettings();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionFindPanel();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    positionFindPanel();
}

void MainWindow::positionFindPanel()
{
    if (!m_findPanel)
        return;
    m_findPanel->adjustSize();
    const QSize hint = m_findPanel->sizeHint();
    const int w = qMin(hint.width(), width() - 24);
    const int h = hint.height();
    m_findPanel->resize(w, h);
    int y = height() - h - 14;
    if (statusBar() && statusBar()->isVisible())
        y -= statusBar()->height();
    m_findPanel->move((width() - w) / 2, qMax(0, y));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (!url.isLocalFile())
                return;
        }
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;
    for (const QUrl &url : event->mimeData()->urls())
        loadFileIntoTab(url.toLocalFile());
    event->acceptProposedAction();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Tab bar: middle click closes, double click on empty area opens a new tab,
    // right click opens a context menu.
    if (watched == m_tabWidget->tabBar()) {
        QTabBar *bar = m_tabWidget->tabBar();
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::MiddleButton) {
                const int index = bar->tabAt(me->pos());
                if (index >= 0) {
                    closeTab(index);
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && bar->tabAt(me->pos()) < 0) {
                onNewFile();
                return true;
            }
        } else if (event->type() == QEvent::ContextMenu) {
            auto *ce = static_cast<QContextMenuEvent *>(event);
            const int index = bar->tabAt(ce->pos());
            QMenu menu(this);
            if (index >= 0) {
                TabData *tab = tabForWidget(m_tabWidget->widget(index));
                QAction *closeOne = menu.addAction(tr("Close Tab"));
                connect(closeOne, &QAction::triggered, this, [this, index] { closeTab(index); });
                QAction *closeOthers = menu.addAction(tr("Close Other Tabs"));
                connect(closeOthers, &QAction::triggered, this, [this, index] {
                    m_tabWidget->setCurrentIndex(index);
                    onCloseOthers();
                });
                QAction *closeAll = menu.addAction(tr("Close All Tabs"));
                connect(closeAll, &QAction::triggered, this, [this] { onCloseAll(); });
                if (tab && !tab->filePath.isEmpty()) {
                    menu.addSeparator();
                    QAction *copyPath = menu.addAction(tr("Copy Full &Path"));
                    connect(copyPath, &QAction::triggered, this, [tab] {
                        QApplication::clipboard()->setText(QDir::toNativeSeparators(tab->filePath));
                    });
                    QAction *copyName = menu.addAction(tr("Copy File &Name"));
                    connect(copyName, &QAction::triggered, this, [tab] {
                        QApplication::clipboard()->setText(QFileInfo(tab->filePath).fileName());
                    });
                    QAction *reveal = menu.addAction(tr("Open Containing &Folder"));
                    connect(reveal, &QAction::triggered, this, [tab] {
                        QDesktopServices::openUrl(
                            QUrl::fromLocalFile(QFileInfo(tab->filePath).absolutePath()));
                    });
                }
            } else {
                QAction *newTab = menu.addAction(tr("New Tab"));
                connect(newTab, &QAction::triggered, this, [this] { onNewFile(); });
            }
            menu.exec(ce->globalPos());
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------
// Demo content & screenshots (--screenshot)
// ---------------------------------------------------------------------------

void MainWindow::loadDemoContent()
{
    const QList<TabData *> tabs = m_tabs;
    for (TabData *t : tabs)
        removeTabDirect(t);

    const QString dir = QDir::temp().absoluteFilePath(QStringLiteral("novapad-demo"));
    QDir().mkpath(dir);

    static const char *demoCpp = R"CPP(// main.cpp — NovaPad demo file
#include <iostream>
#include <string>
#include <vector>

/*  NovaPad — a modern, tabbed text editor
    written in C++ with the Qt framework. */

namespace novapad {

enum class Theme { Dark, Light };

class Editor {
public:
    Editor(const std::string &name)
        : m_name(name), m_line(1), m_column(1) {}

    void open(const std::string &path);
    bool save() const;

    int line() const { return m_line; }
    int column() const { return m_column; }

private:
    std::string m_name;
    std::vector<std::string> m_history;
    int m_line = 0;
    int m_column = 0;
    static constexpr int kMaxTabs = 64;
};

} // namespace novapad

int main(int argc, char **argv) {
    using namespace novapad;

    Editor editor("notes.txt");
    editor.open("main.cpp");

    for (int i = 0; i < 10; ++i) {
        std::cout << "NovaPad tab #" << i << "\n";
    }

    // TODO: add syntax highlighting for more languages
    Theme theme = Theme::Dark;
    if (argc > 1 && std::string(argv[1]) == "--light") {
        theme = Theme::Light;
    }

    std::cout << "Theme: " << (theme == Theme::Dark ? "dark" : "light") << std::endl;
    return 0;
}
)CPP";

    static const char *notes = R"TXT(NovaPad notes
=============

[x] Tabs with drag & drop
[x] Dark / light themes
[x] Find & replace with regex
[x] Syntax highlighting (13 languages)
[ ] Plugin system

Shortcuts
---------
Ctrl+N   new tab          Ctrl+F   find
Ctrl+S   save             Ctrl+H   replace
Ctrl+G   go to line       F5       time/date
Ctrl+D   duplicate line   Ctrl+/   toggle comment
)TXT";

    const QString cppPath = QDir(dir).filePath(QStringLiteral("main.cpp"));
    const QString notesPath = QDir(dir).filePath(QStringLiteral("notes.txt"));
    QFile cppFile(cppPath);
    if (cppFile.open(QIODevice::WriteOnly))
        cppFile.write(demoCpp);
    cppFile.close();
    QFile notesFile(notesPath);
    if (notesFile.open(QIODevice::WriteOnly))
        notesFile.write(notes);
    notesFile.close();

    loadFileIntoTab(cppPath);
    loadFileIntoTab(notesPath);
    m_tabWidget->setCurrentIndex(0);

    if (Editor *e = currentEditor()) {
        QTextCursor c(e->document());
        c.setPosition(qMin(360, e->document()->characterCount() - 1));
        e->setTextCursor(c);
        e->ensureCursorVisible();
    }
}

bool MainWindow::takeScreenshots(const QString &dir, const QString &themeName)
{
    Q_UNUSED(themeName)
    QDir().mkpath(dir);

    loadDemoContent();

    resize(1280, 800);
    show();
    raise();
    for (int i = 0; i < 8; ++i)
        QApplication::processEvents();

    bool ok = true;

    applyTheme(true);
    for (int i = 0; i < 8; ++i)
        QApplication::processEvents();
    ok &= grab().save(QDir(dir).filePath(QStringLiteral("novapad-dark.png")));

    m_findPanel->openFind(QStringLiteral("Nova"));
    positionFindPanel();
    updateSearchHighlights();
    selectMatch(0);
    for (int i = 0; i < 8; ++i)
        QApplication::processEvents();
    ok &= grab().save(QDir(dir).filePath(QStringLiteral("novapad-find.png")));
    m_findPanel->closePanel();
    for (int i = 0; i < 4; ++i)
        QApplication::processEvents();

    applyTheme(false);
    for (int i = 0; i < 8; ++i)
        QApplication::processEvents();
    ok &= grab().save(QDir(dir).filePath(QStringLiteral("novapad-light.png")));

    return ok;
}

// ---------------------------------------------------------------------------
// Self test (--selftest)
// ---------------------------------------------------------------------------

int MainWindow::runSelfTest()
{
    int failures = 0;
    auto CHECK = [&failures](bool ok, const char *what) {
        if (ok) {
            qInfo("[pass] %s", what);
        } else {
            ++failures;
            qWarning("[FAIL] %s", what);
        }
    };

    // ---- FileIo -------------------------------------------------------------
    {
        const QByteArray plain = FileIo::encode(QStringLiteral("a\nb"), false, false);
        CHECK(plain == QByteArrayLiteral("a\nb"), "FileIo::encode LF");

        const QByteArray crlfBom = FileIo::encode(QStringLiteral("a\nb"), true, true);
        CHECK(crlfBom == QByteArrayLiteral("\xEF\xBB\xBF") + QByteArrayLiteral("a\r\nb"),
              "FileIo::encode CRLF + BOM");

        const FileIo::Decoded d = FileIo::decode(
            QByteArrayLiteral("\xEF\xBB\xBF") + QByteArrayLiteral("a\r\nb"));
        CHECK(d.bom && d.crlf && d.text == QStringLiteral("a\nb")
                  && d.encodingLabel == QStringLiteral("UTF-8-BOM"),
              "FileIo::decode BOM + CRLF");

        const FileIo::Decoded d2 = FileIo::decode(QByteArrayLiteral("caf\xE9 100%"));
        CHECK(d2.text.contains(QStringLiteral("caf"))
                  && d2.encodingLabel == QStringLiteral("System (ANSI)"),
              "FileIo::decode non-UTF8 fallback");

        const FileIo::Decoded d3 = FileIo::decode(QByteArrayLiteral("h\xC3\xA9llo"));
        CHECK(d3.encodingLabel == QStringLiteral("UTF-8") && d3.text.startsWith(QLatin1Char('h')),
              "FileIo::decode UTF-8");
    }

    // ---- Search -------------------------------------------------------------
    {
        QTextDocument doc;
        doc.setPlainText(QStringLiteral("foo bar foo\nfoobar foo"));
        const QRegularExpression re = Search::buildPattern(QStringLiteral("foo"), true, true, false);
        const QList<QTextCursor> ms = Search::findAll(&doc, re);
        CHECK(ms.size() == 3, "Search::findAll whole word");

        const QRegularExpression reSub = Search::buildPattern(QStringLiteral("foo"), true, false, false);
        CHECK(Search::findAll(&doc, reSub).size() == 4, "Search::findAll substring");

        const QRegularExpression reRe = Search::buildPattern(
            QStringLiteral("f(\\w+)"), false, false, true);
        const QRegularExpressionMatch m = reRe.match(QStringLiteral("fun foo"));
        CHECK(Search::expandBackrefs(QStringLiteral("<\\1> [$1]"), m) == QStringLiteral("<un> [un]"),
              "Search::expandBackrefs");

        QTextDocument doc2;
        doc2.setPlainText(QStringLiteral("one two three two ONE"));
        const int replaced = Search::replaceAll(
            &doc2, Search::buildPattern(QStringLiteral("two"), true, false, false),
            QStringLiteral("[x]"));
        CHECK(replaced == 2 && doc2.toPlainText() == QStringLiteral("one [x] three [x] ONE"),
              "Search::replaceAll");

        QTextDocument doc3;
        doc3.setPlainText(QStringLiteral("a1 b2 c3"));
        const int reGrouped = Search::replaceAll(
            &doc3, Search::buildPattern(QStringLiteral("(\\w)(\\d)"), false, false, true),
            QStringLiteral("\\2\\1"));
        CHECK(reGrouped == 3 && doc3.toPlainText() == QStringLiteral("1a 2b 3c"),
              "Search::replaceAll with backrefs");
    }

    // ---- Languages ----------------------------------------------------------
    {
        CHECK(Languages::detect(QStringLiteral("src/main.cpp"))->name
                  == QStringLiteral("C++"),
              "Languages::detect .cpp");
        CHECK(Languages::detect(QStringLiteral("CMakeLists.txt"))->name
                  == QStringLiteral("CMake"),
              "Languages::detect CMakeLists.txt");
        CHECK(Languages::detect(QStringLiteral("notes.txt"))->name == QStringLiteral("Text"),
              "Languages::detect .txt");
        CHECK(Languages::detect(QStringLiteral("script.py"))->name == QStringLiteral("Python"),
              "Languages::detect .py");
    }

    // ---- Highlighter block states -------------------------------------------
    {
        // Use an Editor host: a bare QTextDocument has no layout and never
        // emits contentsChange, so attach the document to a real widget.
        Editor host;
        SyntaxHighlighter hl(host.document());
        hl.setLanguage(Languages::byName(QStringLiteral("C++")));
        host.setPlainText(QStringLiteral("/* multi\nline */ int x;"));
        hl.rehighlight();
        CHECK(host.document()->firstBlock().userState() == 1,
              "Highlighter: in-block-comment state");
        CHECK(host.document()->firstBlock().next().userState() == 0,
              "Highlighter: block comment closed");

        host.setPlainText(QStringLiteral("s = \"still\nopen;"));
        hl.rehighlight();
        CHECK(host.document()->firstBlock().userState() == 0,
              "Highlighter: unterminated string stays on line");

        hl.setLanguage(Languages::byName(QStringLiteral("Python")));
        host.setPlainText(QStringLiteral("x = \"\"\"triple\nquoted\"\"\" 1"));
        hl.rehighlight();
        CHECK(host.document()->firstBlock().userState() == 2,
              "Highlighter: python triple-quoted state");
    }

    // ---- Editor line operations ---------------------------------------------
    {
        Editor e;
        e.setPlainText(QStringLiteral("alpha\nbeta\ngamma"));

        QTextCursor c(e.document());
        c.setPosition(10); // end of "beta" (at its newline)
        e.setTextCursor(c);
        e.moveLines(1);
        CHECK(e.toPlainText() == QStringLiteral("alpha\ngamma\nbeta"), "Editor::moveLines down");

        e.moveLines(-1);
        CHECK(e.toPlainText() == QStringLiteral("alpha\nbeta\ngamma"), "Editor::moveLines up");

        e.moveLines(-1);
        CHECK(e.toPlainText() == QStringLiteral("beta\nalpha\ngamma"), "Editor::moveLines up to top");

        e.moveLines(-1); // already at top: no-op
        CHECK(e.toPlainText() == QStringLiteral("beta\nalpha\ngamma"),
              "Editor::moveLines no-op at top");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("alpha\nbeta\ngamma"));
        QTextCursor c(e.document());
        c.setPosition(6); // start of "beta"
        e.setTextCursor(c);
        e.moveLines(1);
        CHECK(e.toPlainText() == QStringLiteral("alpha\ngamma\nbeta"),
              "Editor::moveLines down (cursor at line start)");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("one\ntwo\nthree"));
        // select "two" fully
        QTextCursor c(e.document());
        c.setPosition(4);
        c.setPosition(7, QTextCursor::KeepAnchor);
        e.setTextCursor(c);
        e.moveLines(1);
        CHECK(e.toPlainText() == QStringLiteral("one\nthree\ntwo"), "Editor::moveLines selection down");
        CHECK(e.textCursor().hasSelection() && e.textCursor().selectedText() == QStringLiteral("two"),
              "Editor::moveLines selection restored");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("last"));
        QTextCursor c(e.document());
        c.setPosition(3);
        e.setTextCursor(c);
        e.moveLines(1);
        CHECK(e.toPlainText() == QStringLiteral("last"), "Editor::moveLines single line at bottom");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("one\ntwo\nthree"));
        QTextCursor c(e.document());
        c.setPosition(5);
        e.setTextCursor(c);
        e.deleteLines();
        CHECK(e.toPlainText() == QStringLiteral("one\nthree"), "Editor::deleteLines middle");

        QTextCursor last(e.document());
        last.setPosition(e.document()->characterCount() - 1);
        e.setTextCursor(last);
        e.deleteLines();
        CHECK(e.toPlainText() == QStringLiteral("one"), "Editor::deleteLines last");

        e.deleteLines();
        CHECK(e.toPlainText().isEmpty(), "Editor::deleteLines only line");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("abc"));
        QTextCursor c(e.document());
        c.setPosition(3);
        e.setTextCursor(c);
        e.duplicateSelectionOrLine();
        CHECK(e.toPlainText() == QStringLiteral("abc\nabc"), "Editor::duplicateLine");

        QTextCursor sel(e.document());
        sel.setPosition(0);
        sel.setPosition(3, QTextCursor::KeepAnchor);
        e.setTextCursor(sel);
        e.duplicateSelectionOrLine();
        CHECK(e.toPlainText() == QStringLiteral("abcabc\nabc"), "Editor::duplicateSelection");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("Hello World"));
        QTextCursor c(e.document());
        c.setPosition(0);
        c.setPosition(11, QTextCursor::KeepAnchor);
        e.setTextCursor(c);
        e.transformCase(Editor::InvertCase);
        CHECK(e.toPlainText() == QStringLiteral("hELLO wORLD"), "Editor::transformCase invert");

        e.selectAll();
        e.transformCase(Editor::TitleCase);
        CHECK(e.toPlainText() == QStringLiteral("Hello World"), "Editor::transformCase title");
    }
    {
        Editor e;
        e.setPlainText(QStringLiteral("keep  \n\ttrim\t\n"));
        e.trimTrailingWhitespace();
        CHECK(e.toPlainText() == QStringLiteral("keep\n\ttrim\n"),
              "Editor::trimTrailingWhitespace");
    }

    // ---- Toggle comment -------------------------------------------------------
    {
        Editor e;
        e.setPlainText(QStringLiteral("int a;\nint b;"));
        QTextCursor c(e.document());
        c.setPosition(0);
        c.setPosition(e.document()->characterCount() - 1, QTextCursor::KeepAnchor);
        e.setTextCursor(c);
        toggleLineComment(&e, Languages::byName(QStringLiteral("C++")));
        CHECK(e.toPlainText() == QStringLiteral("// int a;\n// int b;"), "toggle comment: comment");
        toggleLineComment(&e, Languages::byName(QStringLiteral("C++")));
        CHECK(e.toPlainText() == QStringLiteral("int a;\nint b;"), "toggle comment: uncomment");
    }

    qInfo("SELFTEST %s", failures == 0 ? "PASSED" : "FAILED");
    return failures == 0 ? 0 : 1;
}

#pragma once

#include <QList>
#include <QMainWindow>
#include <QTextCursor>

#include "themes.h"

class Editor;
class FindPanel;
class QLabel;
class QMenu;
class QAction;
class QActionGroup;
class QFileSystemWatcher;
class SyntaxHighlighter;
class QShortcut;
class QTabWidget;
class QToolBar;
class QToolButton;
struct LanguageDef;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool openPaths(const QStringList &paths);
    void newWindow();

    // Developer helpers used by CI / --screenshot and --selftest
    static int runSelfTest();
    bool takeScreenshots(const QString &dir, const QString &themeName);
    void loadDemoContent();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // file
    void onNewFile();
    void onOpenDialog();
    void onSave();
    void onSaveAs();
    void onSaveAll();
    void closeTab(int index);
    void onCloseCurrent();
    void onCloseOthers();
    void onCloseAll();
    void onPrint();
    void onPrintPreview();
    void onPageSetup();
    // edit
    void onFind();
    void onReplace();
    void onFindNext();
    void onFindPrevious();
    void onDoReplace();
    void onReplaceAll();
    void onGoTo();
    void onTimeDate();
    void onFontDialog();
    void onStats();
    void onAbout();
    void onAboutQt();
    void openRecentFile();
    void clearRecentFiles();
    // view
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onWordWrapToggled(bool on);
    void onLineNumbersToggled(bool on);
    void onStatusbarToggled(bool on);
    void onThemeChanged(QAction *action);
    void onEolChanged(QAction *action);
    void onEncodingChanged(QAction *action);
    void onLanguageChanged(QAction *action);
    // tabs / editors
    void onTabChanged(int index);
    void onModificationChanged(bool modified);
    void onCursorPositionChanged();
    void onTextChanged();
    void onBlockCountChanged(int count);
    void onFileChangedExternally(const QString &path);
    // search
    void onFindCriteriaChanged();
    void onPanelClosed();

private:
    struct TabData {
        Editor *editor = nullptr;
        SyntaxHighlighter *highlighter = nullptr;
        QString filePath; // empty = untitled
        QString encodingLabel = QStringLiteral("UTF-8");
        bool bom = false;
        bool crlf = false;
        int untitledNumber = 0;
    };

    struct ToolIcon {
        QAction *action = nullptr;
        QIcon (*factory)(const QColor &) = nullptr;
    };

    TabData *tabForWidget(QWidget *widget);
    TabData *currentTab();
    Editor *currentEditor();
    TabData *addTab(const QString &filePath);
    bool loadFileIntoTab(const QString &path);
    bool saveTab(TabData &tab, bool saveAs);
    bool maybeSaveTab(TabData &tab);
    void removeTabDirect(TabData *tab);
    void updateTabTitle(TabData &tab);
    void updateWindowTitle();
    void updateStatusBar();
    void updateSearchHighlights();
    void updateWordHighlights();
    void selectMatch(int index);
    void refreshRecentMenu();
    void pushRecentFile(const QString &path);
    void applyTheme(bool dark);
    void rebuildToolbarIcons();
    void setupUi();
    void setupMenus();
    void setupStatusBar();
    void setupConnections();
    void readSettings();
    void writeSettings();
    void positionFindPanel();
    void rehighlightTab(TabData &tab);
    QString tabDisplayName(const TabData &tab) const;
    QString lastUsedDir() const;

    QTabWidget *m_tabWidget = nullptr;
    QToolBar *m_toolbar = nullptr;
    QToolButton *m_btnNewTab = nullptr;
    FindPanel *m_findPanel = nullptr;
    QFileSystemWatcher *m_watcher = nullptr;
    QLabel *m_lblPos = nullptr;
    QLabel *m_lblSel = nullptr;
    QLabel *m_lblLines = nullptr;
    QLabel *m_lblChars = nullptr;
    QLabel *m_lblEncoding = nullptr;
    QLabel *m_lblEol = nullptr;
    QLabel *m_lblLang = nullptr;
    QLabel *m_lblZoom = nullptr;

    QMenu *m_menuRecent = nullptr;
    QMenu *m_menuTheme = nullptr;
    QMenu *m_menuEol = nullptr;
    QMenu *m_menuEncoding = nullptr;
    QMenu *m_menuLanguage = nullptr;

    QAction *m_actSave = nullptr;
    QAction *m_actWrap = nullptr;
    QAction *m_actLineNumbers = nullptr;
    QAction *m_actStatusbar = nullptr;

    QList<TabData *> m_tabs;
    QList<ToolIcon> m_toolIcons;
    Theme m_theme;
    bool m_dark = true;
    QFont m_editorFont;
    int m_editorFontSize = 11;
    QStringList m_recentFiles;
    QList<QTextCursor> m_matches;
    int m_currentMatch = -1;
    int m_untitledCounter = 0;
    QString m_iconDir;
    bool m_restoring = false;
};

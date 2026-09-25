#include "themes.h"

#include <QColor>

namespace {

inline QString hex(const QColor &c)
{
    return c.name();
}

} // namespace

namespace Themes {

// Palette based on the VS Code "Dark+" / "Light+" families, tuned to match
// the rest of the Nova chrome.
Theme dark()
{
    Theme t;
    t.name = QStringLiteral("Nova Dark");

    t.window        = QColor(0x19, 0x1c, 0x22);
    t.base          = QColor(0x22, 0x26, 0x2e);
    t.sunken        = QColor(0x1b, 0x1e, 0x25);
    t.border        = QColor(0x2a, 0x2e, 0x38);
    t.borderStrong  = QColor(0x39, 0x3f, 0x4b);
    t.text          = QColor(0xe6, 0xe9, 0xef);
    t.textMuted     = QColor(0x9a, 0xa1, 0xad);
    t.accent        = QColor(0x4d, 0x9e, 0xf7);
    t.accentHover   = QColor(0x6a, 0xaf, 0xf9);
    t.accentText    = QColor(0x0b, 0x12, 0x1e);
    t.hover         = QColor(0x26, 0x2a, 0x33);
    t.pressed       = QColor(0x20, 0x24, 0x2c);
    t.danger        = QColor(0xf0, 0x6c, 0x6c);
    t.tabBar        = QColor(0x19, 0x1c, 0x22);
    t.tabActive     = QColor(0x2b, 0x30, 0x3c);
    t.tabInactive   = QColor(0x21, 0x25, 0x2d);
    t.tabHover      = QColor(0x25, 0x29, 0x33);
    t.statusBar     = QColor(0x16, 0x18, 0x1e);
    t.statusBarBorder = QColor(0x26, 0x2a, 0x33);
    t.scrollHandle      = QColor(0x3b, 0x41, 0x4d);
    t.scrollHandleHover = QColor(0x4a, 0x51, 0x5f);

    t.editor.background            = QColor(0x1e, 0x21, 0x28);
    t.editor.foreground            = QColor(0xd7, 0xdb, 0xe4);
    t.editor.gutterBackground      = QColor(0x1e, 0x21, 0x28);
    t.editor.lineNumber            = QColor(0x56, 0x5d, 0x6b);
    t.editor.currentLineNumber     = QColor(0xcb, 0xd0, 0xdc);
    t.editor.currentLineBackground = QColor(0x26, 0x2a, 0x33);
    t.editor.selectionBackground   = QColor(0x2f, 0x4a, 0x6e);
    t.editor.searchHighlight       = QColor(0x3f, 0x45, 0x50);
    t.editor.searchCurrent         = QColor(0x8f, 0x62, 0x0f);
    t.editor.wordHighlight         = QColor(0x32, 0x38, 0x43);

    t.syntax.keyword      = QColor(0x56, 0x9c, 0xd6);
    t.syntax.control      = QColor(0xc5, 0x86, 0xc0);
    t.syntax.type         = QColor(0x4e, 0xc9, 0xb0);
    t.syntax.literal      = QColor(0x56, 0x9c, 0xd6);
    t.syntax.string       = QColor(0xce, 0x91, 0x78);
    t.syntax.number       = QColor(0xb5, 0xce, 0xa8);
    t.syntax.comment      = QColor(0x6a, 0x99, 0x55);
    t.syntax.function     = QColor(0xdc, 0xdc, 0xaa);
    t.syntax.preprocessor = QColor(0xc5, 0x86, 0xc0);
    t.syntax.tag          = QColor(0x56, 0x9c, 0xd6);
    t.syntax.attribute    = QColor(0x9c, 0xdc, 0xfe);
    t.syntax.entity       = QColor(0xb5, 0xce, 0xa8);
    return t;
}

Theme light()
{
    Theme t;
    t.name = QStringLiteral("Nova Light");

    t.window        = QColor(0xf3, 0xf4, 0xf7);
    t.base          = QColor(0xff, 0xff, 0xff);
    t.sunken        = QColor(0xf2, 0xf3, 0xf6);
    t.border        = QColor(0xd9, 0xdc, 0xe2);
    t.borderStrong  = QColor(0xc3, 0xc7, 0xd0);
    t.text          = QColor(0x1f, 0x23, 0x28);
    t.textMuted     = QColor(0x6e, 0x75, 0x81);
    t.accent        = QColor(0x18, 0x6f, 0xf0);
    t.accentHover   = QColor(0x3b, 0x82, 0xf4);
    t.accentText    = QColor(0xff, 0xff, 0xff);
    t.hover         = QColor(0xe9, 0xeb, 0xf0);
    t.pressed       = QColor(0xdd, 0xe0, 0xe7);
    t.danger        = QColor(0xd9, 0x30, 0x25);
    t.tabBar        = QColor(0xf3, 0xf4, 0xf7);
    t.tabActive     = QColor(0xff, 0xff, 0xff);
    t.tabInactive   = QColor(0xea, 0xec, 0xf1);
    t.tabHover      = QColor(0xe6, 0xe8, 0xee);
    t.statusBar     = QColor(0xec, 0xee, 0xf2);
    t.statusBarBorder = QColor(0xd9, 0xdc, 0xe2);
    t.scrollHandle      = QColor(0xc4, 0xc9, 0xd2);
    t.scrollHandleHover = QColor(0xab, 0xb1, 0xbc);

    t.editor.background            = QColor(0xff, 0xff, 0xff);
    t.editor.foreground            = QColor(0x24, 0x29, 0x2f);
    t.editor.gutterBackground      = QColor(0xfa, 0xfb, 0xfc);
    t.editor.foreground            = QColor(0x24, 0x29, 0x2f);
    t.editor.lineNumber            = QColor(0x8b, 0x92, 0x9e);
    t.editor.currentLineNumber     = QColor(0x24, 0x29, 0x2f);
    t.editor.currentLineBackground = QColor(0xf2, 0xf4, 0xf7);
    t.editor.selectionBackground   = QColor(0xad, 0xd6, 0xff);
    t.editor.searchHighlight       = QColor(0xfe, 0xf3, 0xc8);
    t.editor.searchCurrent         = QColor(0xfd, 0xd8, 0x7a);
    t.editor.wordHighlight         = QColor(0xed, 0xef, 0xf4);

    t.syntax.keyword      = QColor(0x00, 0x00, 0xff);
    t.syntax.control      = QColor(0xaf, 0x00, 0xdb);
    t.syntax.type         = QColor(0x26, 0x7f, 0x99);
    t.syntax.literal      = QColor(0x00, 0x00, 0xff);
    t.syntax.string       = QColor(0xa3, 0x15, 0x15);
    t.syntax.number       = QColor(0x09, 0x86, 0x58);
    t.syntax.comment      = QColor(0x00, 0x80, 0x00);
    t.syntax.function     = QColor(0x79, 0x5e, 0x26);
    t.syntax.preprocessor = QColor(0xaf, 0x00, 0xdb);
    t.syntax.tag          = QColor(0x80, 0x00, 0x00);
    t.syntax.attribute    = QColor(0x00, 0x10, 0x80);
    t.syntax.entity       = QColor(0x81, 0x1f, 0x3f);
    return t;
}

} // namespace Themes

QString Theme::styleSheet(const QString &iconDir) const
{
    const QString qss = QStringLiteral(R"CSS(
* { outline: none; }

QMainWindow { background: %WINDOW%; }
QMainWindow::separator { background: %BORDER%; width: 2px; height: 2px; }

QDialog, QMessageBox { background: %WINDOW%; }
QMessageBox QLabel { color: %TEXT%; }

QMenuBar { background: %WINDOW%; border-bottom: 1px solid %BORDER%; padding: 2px 6px 0px 6px; }
QMenuBar::item { padding: 6px 10px; border-radius: 6px; color: %TEXT%; background: transparent; }
QMenuBar::item:selected { background: %HOVER%; }
QMenuBar::item:pressed { background: %PRESSED%; }

QMenu { background: %BASE%; border: 1px solid %BORDER%; border-radius: 10px; padding: 6px; }
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 6px; color: %TEXT%; background: transparent; }
QMenu::item:selected { background: %ACCENT%; color: %ACCENTTEXT%; }
QMenu::item:disabled { color: %MUTED%; }
QMenu::item:disabled:selected { background: transparent; color: %MUTED%; }
QMenu::separator { height: 1px; background: %BORDER%; margin: 6px 8px; }
QMenu::icon { padding-left: 8px; }
QMenu::right-arrow { width: 14px; height: 14px; image: url("%ICONDIR%/arrow.png"); }
QMenu::indicator { width: 15px; height: 15px; margin-left: 8px; border: 1px solid %BORDERSTRONG%; border-radius: 4px; background: %SUNKEN%; }
QMenu::indicator:checked { background: %ACCENT%; border: none; image: url("%ICONDIR%/check.png"); }

QToolBar { background: %WINDOW%; border: none; border-bottom: 1px solid %BORDER%; padding: 4px 8px; spacing: 2px; }
QToolBar::separator { width: 1px; background: %BORDER%; margin: 8px 5px; }
QToolButton { background: transparent; border: none; border-radius: 6px; padding: 5px; }
QToolButton:hover { background: %HOVER%; }
QToolButton:pressed, QToolButton:checked { background: %PRESSED%; }

QTabWidget::pane { border: none; border-top: 1px solid %BORDER%; background: %EDITBG%; }
QTabBar { background: %TABBAR%; }
QTabBar::tab {
    background: %TABINACTIVE%; color: %MUTED%;
    padding: 7px 10px 7px 14px;
    margin: 5px 2px 0px 2px;
    border-top-left-radius: 8px; border-top-right-radius: 8px;
    border-bottom: 2px solid transparent;
    min-width: 130px; max-width: 230px;
}
QTabBar::tab:hover { background: %TABHOVER%; color: %TEXT%; }
QTabBar::tab:selected { background: %TABACTIVE%; color: %TEXT%; border-bottom: 2px solid %ACCENT%; }
QTabBar::close-button { image: url("%ICONDIR%/close.png"); background: transparent; border-radius: 4px; padding: 2px; subcontrol-position: right; }
QTabBar::close-button:hover { image: url("%ICONDIR%/close-hover.png"); background: %HOVER%; }

QPlainTextEdit {
    background: %EDITBG%; color: %EDITFG%;
    border: none;
    selection-background-color: %SELBG%;
    selection-color: %EDITFG%;
}

QLineEdit {
    background: %SUNKEN%; border: 1px solid %BORDER%; border-radius: 8px;
    padding: 6px 10px; color: %TEXT%;
    selection-background-color: %ACCENT%; selection-color: %ACCENTTEXT%;
}
QLineEdit:hover { border-color: %BORDERSTRONG%; }
QLineEdit:focus { border: 1px solid %ACCENT%; }

QSpinBox, QDoubleSpinBox {
    background: %SUNKEN%; border: 1px solid %BORDER%; border-radius: 8px;
    padding: 5px 8px; color: %TEXT%;
}
QSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    width: 0px; border: none; background: transparent;
}

QPushButton {
    background: %BASE%; color: %TEXT%;
    border: 1px solid %BORDERSTRONG%; border-radius: 8px;
    padding: 6px 16px;
}
QPushButton:hover { background: %HOVER%; border-color: %BORDERSTRONG%; }
QPushButton:pressed { background: %PRESSED%; }
QPushButton:default { background: %ACCENT%; color: %ACCENTTEXT%; border: none; }
QPushButton:default:hover { background: %ACCENTHOVER%; }
QPushButton:disabled { color: %MUTED%; border-color: %BORDER%; }

QComboBox {
    background: %SUNKEN%; border: 1px solid %BORDER%; border-radius: 8px;
    padding: 5px 10px; color: %TEXT%;
}
QComboBox:hover { border-color: %BORDERSTRONG%; }
QComboBox:focus { border: 1px solid %ACCENT%; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox::down-arrow { image: url("%ICONDIR%/arrow.png"); width: 12px; height: 12px; }

QCheckBox { color: %TEXT%; spacing: 8px; }
QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid %BORDERSTRONG%; border-radius: 4px; background: %SUNKEN%; }
QCheckBox::indicator:checked { background: %ACCENT%; border: none; image: url("%ICONDIR%/check.png"); }

QStatusBar { background: %STATUSBG%; border-top: 1px solid %STATUSBORDER%; color: %MUTED%; }
QStatusBar::item { border: none; }
QStatusBar QLabel { padding: 3px 10px; background: transparent; border-right: 1px solid %STATUSBORDER%; }

QToolTip { background: %BASE%; color: %TEXT%; border: 1px solid %BORDERSTRONG%; padding: 5px 9px; border-radius: 6px; }

QScrollBar:vertical { background: transparent; width: 11px; margin: 1px; }
QScrollBar::handle:vertical { background: %SCROLL%; border-radius: 4px; min-height: 36px; }
QScrollBar::handle:vertical:hover { background: %SCROLLHOVER%; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; width: 0px; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 1px; }
QScrollBar::handle:horizontal { background: %SCROLL%; border-radius: 4px; min-width: 36px; }
QScrollBar::handle:horizontal:hover { background: %SCROLLHOVER%; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { height: 0px; width: 0px; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QAbstractScrollArea::corner { background: transparent; }

#FindPanel { background: %BASE%; border: 1px solid %BORDERSTRONG%; border-radius: 12px; }
#FindPanel QLabel#FindCaption { color: %MUTED%; }
#FindCount { color: %MUTED%; }
)CSS");

    QString s = qss;
    s.replace("%WINDOW%", hex(window));
    s.replace("%BASE%", hex(base));
    s.replace("%SUNKEN%", hex(sunken));
    s.replace("%BORDERSTRONG%", hex(borderStrong));
    s.replace("%BORDER%", hex(border));
    s.replace("%TEXT%", hex(text));
    s.replace("%MUTED%", hex(textMuted));
    s.replace("%ACCENTHOVER%", hex(accentHover));
    s.replace("%ACCENTTEXT%", hex(accentText));
    s.replace("%ACCENT%", hex(accent));
    s.replace("%HOVER%", hex(hover));
    s.replace("%PRESSED%", hex(pressed));
    s.replace("%TABBAR%", hex(tabBar));
    s.replace("%TABACTIVE%", hex(tabActive));
    s.replace("%TABINACTIVE%", hex(tabInactive));
    s.replace("%TABHOVER%", hex(tabHover));
    s.replace("%STATUSBORDER%", hex(statusBarBorder));
    s.replace("%STATUSBG%", hex(statusBar));
    s.replace("%SCROLLHOVER%", hex(scrollHandleHover));
    s.replace("%SCROLL%", hex(scrollHandle));
    s.replace("%EDITFG%", hex(editor.foreground));
    s.replace("%EDITBG%", hex(editor.background));
    s.replace("%SELBG%", hex(editor.selectionBackground));
    s.replace("%ICONDIR%", iconDir);
    return s;
}

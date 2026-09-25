#pragma once

#include <QColor>
#include <QString>

// Colors used by the syntax highlighter.
struct SyntaxColors {
    QColor keyword;
    QColor control;       // control-flow keywords (if / for / return ...)
    QColor type;
    QColor literal;       // true / false / null ...
    QColor string;
    QColor number;
    QColor comment;
    QColor function;
    QColor preprocessor;  // #include, @decorator, #[attribute] ...
    QColor tag;           // html/xml tags, ini sections
    QColor attribute;     // html attributes, ini keys
    QColor entity;        // &amp; entities, css colors ...
};

// Colors used by the editor widget (text area + gutter).
struct EditorPalette {
    QColor background;
    QColor foreground;
    QColor gutterBackground;
    QColor lineNumber;
    QColor currentLineNumber;
    QColor currentLineBackground;
    QColor selectionBackground;
    QColor searchHighlight;   // all search matches
    QColor searchCurrent;     // the active search match
    QColor wordHighlight;     // occurrences of the word under the cursor
};

struct Theme {
    QString name;

    // UI chrome
    QColor window;        // window / toolbar / tab-bar background
    QColor base;          // menus, popups, floating panels
    QColor sunken;        // text inputs
    QColor border;
    QColor borderStrong;
    QColor text;
    QColor textMuted;
    QColor accent;
    QColor accentHover;
    QColor accentText;
    QColor hover;
    QColor pressed;
    QColor danger;
    QColor tabBar;
    QColor tabActive;
    QColor tabInactive;
    QColor tabHover;
    QColor statusBar;
    QColor statusBarBorder;
    QColor scrollHandle;
    QColor scrollHandleHover;

    EditorPalette editor;
    SyntaxColors syntax;

    // Builds the full application stylesheet. `iconDir` must contain
    // check.png, close.png, close-hover.png and arrow.png (generated at runtime).
    QString styleSheet(const QString &iconDir) const;
};

namespace Themes {
Theme dark();
Theme light();
}

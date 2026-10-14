#pragma once

#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QTextCursor>

#include "themes.h"

class Editor;
class QKeyEvent;
class QMouseEvent;

// Tiny gutter widget that asks its parent Editor to paint the line numbers.
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(Editor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Editor *m_editor;
};

class Editor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    void setPalette(const EditorPalette &palette);
    EditorPalette palette() const { return m_palette; }

    static int indentationColumns(const QString &line);
    static int smartHomeTarget(const QString &line, int column);
    static int findMatchingBracket(const QString &text, int position, int scanLimit = 20000);

    void setAutoCloseBrackets(bool on) { m_autoCloseBrackets = on; }
    bool autoCloseBrackets() const { return m_autoCloseBrackets; }

    void setCustomFont(const QFont &font, int baseSize);
    QFont customFont() const { return m_font; }
    int fontSize() const { return m_font.pointSize(); }
    int baseFontSize() const { return m_baseFontSize; }
    void zoomStep(int delta);
    void resetZoom();

    void setShowLineNumbers(bool on);
    bool showLineNumbers() const { return m_showLineNumbers; }
    void selectLineAtY(int y);
    void setIndentationGuides(bool on);
    bool indentationGuides() const { return m_indentationGuides; }

    // Extra selections managed by MainWindow (search highlights, word matches)
    void setSearchSelections(const QList<QTextEdit::ExtraSelection> &selections);
    void setWordSelections(const QList<QTextEdit::ExtraSelection> &selections);

    // Editing operations (exposed for menu actions + self test)
    void duplicateSelectionOrLine();
    void deleteLines();
    void moveLines(int direction); // -1 = up, +1 = down
    enum CaseMode { UpperCase, LowerCase, TitleCase, InvertCase };
    void transformCase(CaseMode mode);
    void trimTrailingWhitespace();
    void setEolLabel(const QString &label) { m_eolLabel = label; }
    QString eolLabel() const { return m_eolLabel; }

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

signals:
    void fontSizeChanged(int size);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateBracketMatch();

private:
    void refreshExtraSelections();
    void indentSelection(bool dedent);
    bool handleTypingKey(QKeyEvent *event);

    LineNumberArea *m_lineArea = nullptr;
    EditorPalette m_palette;
    QFont m_font;
    int m_baseFontSize = 11;
    bool m_showLineNumbers = true;
    bool m_indentationGuides = true;
    bool m_highlightCurrentLine = true;
    bool m_autoCloseBrackets = true;
    QString m_eolLabel;

    QList<QTextEdit::ExtraSelection> m_currentLine;
    QList<QTextEdit::ExtraSelection> m_bracketSelections;
    QList<QTextEdit::ExtraSelection> m_searchSelections;
    QList<QTextEdit::ExtraSelection> m_wordSelections;
};

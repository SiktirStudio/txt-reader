#include "editor.h"

#include <QAbstractTextDocumentLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QTextBlock>

LineNumberArea::LineNumberArea(Editor *editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setFrameShape(QFrame::NoFrame);
    setCursorWidth(2);
    setCenterOnScroll(false);

    m_lineArea = new LineNumberArea(this);

    connect(document(), &QTextDocument::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void Editor::setPalette(const EditorPalette &palette)
{
    m_palette = palette;
    QPalette p = viewport()->palette();
    p.setColor(QPalette::Base, palette.background);
    p.setColor(QPalette::Text, palette.foreground);
    p.setColor(QPalette::Highlight, palette.selectionBackground);
    p.setColor(QPalette::HighlightedText, palette.foreground);
    viewport()->setPalette(p);
    QWidget::setPalette(p); // for consistency; viewport palette does the real work
    highlightCurrentLine();
    m_lineArea->update();
}

void Editor::setCustomFont(const QFont &font, int baseSize)
{
    m_font = font;
    m_baseFontSize = baseSize;
    if (m_font.pointSize() <= 0)
        m_font.setPointSize(baseSize);
    QPlainTextEdit::setFont(m_font);
    updateLineNumberAreaWidth(0);
    emit fontSizeChanged(m_font.pointSize());
}

void Editor::zoomStep(int delta)
{
    int size = m_font.pointSize() + delta;
    size = qBound(6, size, 60);
    if (size == m_font.pointSize())
        return;
    m_font.setPointSize(size);
    QPlainTextEdit::setFont(m_font);
    updateLineNumberAreaWidth(0);
    emit fontSizeChanged(size);
}

void Editor::resetZoom()
{
    m_font.setPointSize(m_baseFontSize);
    QPlainTextEdit::setFont(m_font);
    updateLineNumberAreaWidth(0);
    emit fontSizeChanged(m_baseFontSize);
}

void Editor::setShowLineNumbers(bool on)
{
    m_showLineNumbers = on;
    updateLineNumberAreaWidth(0);
}

void Editor::setSearchSelections(const QList<QTextEdit::ExtraSelection> &selections)
{
    m_searchSelections = selections;
    refreshExtraSelections();
}

void Editor::setWordSelections(const QList<QTextEdit::ExtraSelection> &selections)
{
    m_wordSelections = selections;
    refreshExtraSelections();
}

void Editor::refreshExtraSelections()
{
    QList<QTextEdit::ExtraSelection> all;
    all << m_currentLine << m_wordSelections << m_searchSelections;
    setExtraSelections(all);
}

int Editor::lineNumberAreaWidth() const
{
    if (!m_showLineNumbers)
        return 0;
    int digits = 1;
    int max = qMax(1, document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    const int space = 14 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void Editor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineArea->scroll(0, dy);
    else
        m_lineArea->update(0, rect.y(), m_lineArea->width(), rect.height());
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (m_highlightCurrentLine && !isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(m_palette.currentLineBackground);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        selections.append(sel);
    }
    m_currentLine = selections;
    refreshExtraSelections();
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineArea);
    painter.fillRect(event->rect(), m_palette.gutterBackground);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    const int currentBlock = textCursor().blockNumber();
    QFont baseFont = font();
    QFont boldFont = baseFont;
    boldFont.setBold(true);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const bool current = (blockNumber == currentBlock);
            painter.setFont(current ? boldFont : baseFont);
            painter.setPen(current ? m_palette.currentLineNumber : m_palette.lineNumber);
            const QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, m_lineArea->width() - 8,
                             fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    const Qt::KeyboardModifiers mods = event->modifiers();

    // Auto indent on Enter
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && mods == Qt::NoModifier) {
        const QString lineText = textCursor().block().text();
        QString indent;
        for (const QChar ch : lineText) {
            if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t'))
                indent += ch;
            else
                break;
        }
        // extra indent right after an opening brace
        const int col = textCursor().positionInBlock();
        if (col > 0 && col <= lineText.size()
            && lineText.left(col).endsWith(QLatin1Char('{'))
            && lineText.mid(col).trimmed().startsWith(QLatin1Char('}'))) {
            insertPlainText(QStringLiteral("\n") + indent + QStringLiteral("  ") + lineText.mid(col).trimmed());
            QTextCursor c = textCursor();
            c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, lineText.mid(col).trimmed().size());
            setTextCursor(c);
            return;
        }
        if (lineText.left(col).endsWith(QLatin1Char('{')))
            indent += QStringLiteral("  ");
        insertPlainText(QStringLiteral("\n") + indent);
        return;
    }

    // Tab / Shift+Tab: indent or dedent (multi-line aware)
    if (event->key() == Qt::Key_Tab
        && (mods == Qt::NoModifier || mods == Qt::ShiftModifier)) {
        const QTextCursor c = textCursor();
        if (c.hasSelection() && c.selectedText().contains(QChar(0x2029))) {
            indentSelection(mods == Qt::ShiftModifier);
            return;
        }
        if (mods == Qt::NoModifier) {
            insertPlainText(QStringLiteral("\t"));
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(event);
}

void Editor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int steps = event->angleDelta().y() / 120;
        if (steps != 0)
            zoomStep(steps > 0 ? 1 : -1);
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void Editor::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->ignore(); // let the MainWindow open files dropped on the editor
    else
        QPlainTextEdit::dragEnterEvent(event);
}

void Editor::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->ignore();
    else
        QPlainTextEdit::dropEvent(event);
}

void Editor::indentSelection(bool dedent)
{
    QTextCursor cursor = textCursor();
    QTextDocument *doc = document();
    const QTextBlock firstBlock = doc->findBlock(cursor.selectionStart());
    const QTextBlock lastBlock = doc->findBlock(cursor.selectionEnd());
    if (!firstBlock.isValid() || !lastBlock.isValid())
        return;

    cursor.beginEditBlock();
    for (QTextBlock b = firstBlock; b.isValid() && b.blockNumber() <= lastBlock.blockNumber(); b = b.next()) {
        if (dedent) {
            const QString t = b.text();
            int count = 0;
            if (t.startsWith(QLatin1Char('\t'))) {
                count = 1;
            } else {
                while (count < 4 && count < t.size() && t.at(count) == QLatin1Char(' '))
                    ++count;
            }
            if (count > 0) {
                QTextCursor k(doc);
                k.setPosition(b.position());
                k.setPosition(b.position() + count, QTextCursor::KeepAnchor);
                k.removeSelectedText();
            }
        } else {
            QTextCursor k(doc);
            k.setPosition(b.position());
            k.insertText(QStringLiteral("\t"));
        }
    }
    cursor.endEditBlock();

    // Restore a selection spanning the affected lines.
    QTextBlock nbFirst = doc->findBlockByNumber(firstBlock.blockNumber());
    QTextBlock nbLast = doc->findBlockByNumber(lastBlock.blockNumber());
    if (nbFirst.isValid() && nbLast.isValid()) {
        QTextCursor k(doc);
        k.setPosition(nbFirst.position());
        k.setPosition(nbLast.position() + nbLast.text().size(), QTextCursor::KeepAnchor);
        setTextCursor(k);
    }
}

void Editor::duplicateSelectionOrLine()
{
    QTextCursor c = textCursor();
    if (c.hasSelection()) {
        const QString sel = c.selectedText().replace(QChar(0x2029), QLatin1Char('\n'));
        const int end = c.selectionEnd();
        c.setPosition(end);
        c.insertText(sel);
        QTextCursor k(document());
        k.setPosition(end);
        k.setPosition(end + sel.size(), QTextCursor::KeepAnchor);
        setTextCursor(k);
    } else {
        const QTextBlock b = c.block();
        const QString t = b.text();
        QTextCursor k(document());
        k.setPosition(b.position() + t.size());
        k.insertText(QStringLiteral("\n") + t);
        setTextCursor(k);
    }
}

void Editor::deleteLines()
{
    QTextCursor c = textCursor();
    QTextDocument *doc = document();
    const QTextBlock firstBlock = doc->findBlock(c.selectionStart());
    const QTextBlock lastBlock = doc->findBlock(c.selectionEnd());
    if (!firstBlock.isValid())
        return;

    const int start = firstBlock.position();
    const int lastEnd = lastBlock.position() + lastBlock.text().size();

    if (lastEnd == doc->characterCount() - 1) {
        // last block of the document: also swallow the preceding newline
        if (start == 0) {
            QTextCursor k(doc);
            k.select(QTextCursor::Document);
            k.removeSelectedText();
        } else {
            QTextCursor k(doc);
            k.setPosition(start - 1);
            k.setPosition(lastEnd, QTextCursor::KeepAnchor);
            k.removeSelectedText();
        }
    } else {
        QTextCursor k(doc);
        k.setPosition(start);
        k.setPosition(lastBlock.next().position(), QTextCursor::KeepAnchor);
        k.removeSelectedText();
    }
}

void Editor::moveLines(int direction)
{
    if (direction == 0)
        return;

    QTextCursor c = textCursor();
    QTextDocument *doc = document();
    const QTextBlock firstBlock = doc->findBlock(c.selectionStart());
    const QTextBlock lastBlock = doc->findBlock(c.selectionEnd());
    if (!firstBlock.isValid() || !lastBlock.isValid())
        return;

    const int regionStart = firstBlock.position();
    const bool regionIsLastBlock = !lastBlock.next().isValid();
    const int regionEnd = regionIsLastBlock
        ? lastBlock.position() + lastBlock.text().size()
        : lastBlock.next().position();

    const QString docText = doc->toPlainText();
    const QString regionText = docText.mid(regionStart, regionEnd - regionStart);

    // Offsets of the original cursor / selection inside the region.
    const int selStartOff = c.selectionStart() - regionStart;
    const int selEndOff = c.selectionEnd() - regionStart;
    const bool hadSelection = c.hasSelection();

    QString combined;
    int newRegionStart = regionStart;

    if (direction < 0) {
        if (firstBlock.blockNumber() == 0)
            return;
        const QTextBlock aboveBlock = firstBlock.previous();
        const int aboveStart = aboveBlock.position();
        const int aboveEnd = firstBlock.position(); // above block always ends with a newline
        const QString aboveText = docText.mid(aboveStart, aboveEnd - aboveStart);

        if (regionIsLastBlock) {
            combined = regionText + QLatin1Char('\n') + aboveText;
            combined.chop(1); // aboveText ends with \n -> keep the document without trailing newline
        } else {
            combined = regionText + aboveText;
        }
        newRegionStart = aboveStart;

        QTextCursor k(doc);
        k.beginEditBlock();
        k.setPosition(aboveStart);
        k.setPosition(regionEnd, QTextCursor::KeepAnchor);
        k.removeSelectedText();
        k.setPosition(aboveStart);
        k.insertText(combined);
        k.endEditBlock();
    } else {
        if (lastBlock.next().isValid() == false)
            return;
        const QTextBlock belowBlock = lastBlock.next();
        const int belowStart = belowBlock.position();
        const bool belowIsLastBlock = !belowBlock.next().isValid();
        const int belowEnd = belowIsLastBlock
            ? belowBlock.position() + belowBlock.text().size()
            : belowBlock.next().position();
        const QString belowText = docText.mid(belowStart, belowEnd - belowStart);

        if (belowIsLastBlock) {
            combined = belowText + QLatin1Char('\n') + regionText;
            combined.chop(1);
            newRegionStart = regionStart + belowText.size() + 1;
        } else {
            combined = belowText + regionText; // both end with \n
            newRegionStart = regionStart + belowText.size();
        }

        QTextCursor k(doc);
        k.beginEditBlock();
        k.setPosition(regionStart);
        k.setPosition(belowEnd, QTextCursor::KeepAnchor);
        k.removeSelectedText();
        k.setPosition(regionStart);
        k.insertText(combined);
        k.endEditBlock();
    }

    const int regionLen = combined.size();
    QTextCursor k(doc);
    k.setPosition(newRegionStart + qBound(0, selStartOff, regionLen));
    if (hadSelection)
        k.setPosition(newRegionStart + qBound(0, selEndOff, regionLen), QTextCursor::KeepAnchor);
    else
        k.setPosition(newRegionStart + qBound(0, selEndOff, regionLen));
    setTextCursor(k);
}

void Editor::transformCase(CaseMode mode)
{
    QTextCursor c = textCursor();
    if (!c.hasSelection())
        c.select(QTextCursor::WordUnderCursor);
    if (!c.hasSelection())
        return;
    const QString sel = c.selectedText();
    QString out;
    out.reserve(sel.size());
    bool prevIsLetter = false;
    for (const QChar ch : sel) {
        QChar r = ch;
        switch (mode) {
        case UpperCase:
            r = ch.toUpper();
            break;
        case LowerCase:
            r = ch.toLower();
            break;
        case InvertCase:
            r = ch.isLower() ? ch.toUpper() : ch.toLower();
            break;
        case TitleCase:
            if (ch.isLetter() && !prevIsLetter)
                r = ch.toUpper();
            else
                r = ch.toLower();
            break;
        }
        out += r;
        prevIsLetter = ch.isLetter() || ch == QLatin1Char('\'');
    }
    c.insertText(out);
}

void Editor::trimTrailingWhitespace()
{
    QTextDocument *doc = document();
    QTextCursor k(doc);
    k.beginEditBlock();
    for (QTextBlock b = doc->firstBlock(); b.isValid(); b = b.next()) {
        const QString t = b.text();
        int end = t.size();
        while (end > 0 && (t.at(end - 1) == QLatin1Char(' ') || t.at(end - 1) == QLatin1Char('\t')))
            --end;
        if (end != t.size()) {
            QTextCursor j(doc);
            j.setPosition(b.position() + end);
            j.setPosition(b.position() + t.size(), QTextCursor::KeepAnchor);
            j.removeSelectedText();
        }
    }
    k.endEditBlock();
}

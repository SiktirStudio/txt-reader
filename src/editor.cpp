#include "editor.h"

#include <QAbstractTextDocumentLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QTextLayout>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QTextBlock>
#include <QVector>

namespace {

QChar closingFor(QChar opening)
{
    if (opening == QLatin1Char('(')) return QLatin1Char(')');
    if (opening == QLatin1Char('[')) return QLatin1Char(']');
    if (opening == QLatin1Char('{')) return QLatin1Char('}');
    if (opening == QLatin1Char('\"')) return QLatin1Char('\"');
    if (opening == QLatin1Char('\'')) return QLatin1Char('\'');
    return QChar();
}

QChar openingFor(QChar closing)
{
    if (closing == QLatin1Char(')')) return QLatin1Char('(');
    if (closing == QLatin1Char(']')) return QLatin1Char('[');
    if (closing == QLatin1Char('}')) return QLatin1Char('{');
    if (closing == QLatin1Char('\"')) return QLatin1Char('\"');
    if (closing == QLatin1Char('\'')) return QLatin1Char('\'');
    return QChar();
}

bool isOpeningBracket(QChar ch)
{
    return ch == QLatin1Char('(') || ch == QLatin1Char('[') || ch == QLatin1Char('{');
}

bool isClosingBracket(QChar ch)
{
    return ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}');
}

bool isPairedCloser(QChar ch)
{
    return isClosingBracket(ch) || ch == QLatin1Char('\"') || ch == QLatin1Char('\'');
}

bool isWordCharacter(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

} // namespace

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

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_editor->selectLineAtY(event->position().toPoint().y());
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setFrameShape(QFrame::NoFrame);
    setCursorWidth(2);
    setCenterOnScroll(false);
    setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' ')) * 4);

    m_lineArea = new LineNumberArea(this);

    connect(document(), &QTextDocument::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::updateBracketMatch);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    updateBracketMatch();
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
    updateBracketMatch();
    m_lineArea->update();
}

int Editor::indentationColumns(const QString &line)
{
    int columns = 0;
    for (QChar ch : line) {
        if (ch == QLatin1Char(' '))
            ++columns;
        else if (ch == QLatin1Char('\t'))
            columns += 4 - columns % 4;
        else
            break;
    }
    return columns;
}

void Editor::setIndentationGuides(bool on)
{
    m_indentationGuides = on;
    viewport()->update();
}

void Editor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    if (!m_indentationGuides)
        return;
    QPainter painter(viewport());
    painter.setClipRect(event->rect());
    painter.setPen(QPen(m_palette.indentGuide, 1, Qt::DotLine));
    const qreal space = QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' '));
    if (space <= 0)
        return;
    for (QTextBlock block = firstVisibleBlock(); block.isValid(); block = block.next()) {
        const QRectF geometry = blockBoundingGeometry(block).translated(contentOffset());
        if (geometry.top() > event->rect().bottom())
            break;
        if (!block.isVisible() || geometry.bottom() < event->rect().top())
            continue;
        const QTextLayout *layout = block.layout();
        if (!layout || layout->lineCount() == 0)
            continue;
        const QTextLine line = layout->lineAt(0);
        const qreal left = geometry.left() + line.cursorToX(0);
        const int columns = indentationColumns(block.text());
        // Bound work by viewport width, even for very deeply indented lines.
        int column = qMax(4, int((event->rect().left() - left) / (space * 4)) * 4);
        for (; column <= columns; column += 4) {
            const qreal x = left + (column - 0.5) * space;
            if (x > event->rect().right())
                break;
            painter.drawLine(QPointF(x, geometry.top() + line.y()),
                             QPointF(x, geometry.top() + line.y() + line.height()));
        }
    }
}

int Editor::smartHomeTarget(const QString &line, int column)
{
    const int col = qBound(0, column, line.size());
    int first = 0;
    while (first < line.size() && line.at(first).isSpace())
        ++first;

    if (col == 0)
        return first;
    if (col == first)
        return 0;
    return first;
}

int Editor::findMatchingBracket(const QString &text, int position, int scanLimit)
{
    if (position < 0 || position >= text.size() || scanLimit <= 0)
        return -1;

    const QChar target = text.at(position);
    if (isOpeningBracket(target)) {
        QVector<QChar> expectedClosers;
        expectedClosers.append(closingFor(target));
        int scanned = 0;
        for (int i = position + 1; i < text.size() && scanned < scanLimit; ++i, ++scanned) {
            const QChar ch = text.at(i);
            if (isOpeningBracket(ch)) {
                expectedClosers.append(closingFor(ch));
            } else if (isClosingBracket(ch) && ch == expectedClosers.constLast()) {
                expectedClosers.removeLast();
                if (expectedClosers.isEmpty())
                    return i;
            }
        }
    } else if (isClosingBracket(target)) {
        QVector<QChar> expectedOpeners;
        expectedOpeners.append(openingFor(target));
        int scanned = 0;
        for (int i = position - 1; i >= 0 && scanned < scanLimit; --i, ++scanned) {
            const QChar ch = text.at(i);
            if (isClosingBracket(ch)) {
                expectedOpeners.append(openingFor(ch));
            } else if (isOpeningBracket(ch) && ch == expectedOpeners.constLast()) {
                expectedOpeners.removeLast();
                if (expectedOpeners.isEmpty())
                    return i;
            }
        }
    }
    return -1;
}

void Editor::setCustomFont(const QFont &font, int baseSize)
{
    m_font = font;
    m_baseFontSize = baseSize;
    if (m_font.pointSize() <= 0)
        m_font.setPointSize(baseSize);
    QPlainTextEdit::setFont(m_font);
    setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' ')) * 4);
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
    setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' ')) * 4);
    updateLineNumberAreaWidth(0);
    emit fontSizeChanged(size);
}

void Editor::resetZoom()
{
    m_font.setPointSize(m_baseFontSize);
    QPlainTextEdit::setFont(m_font);
    setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' ')) * 4);
    updateLineNumberAreaWidth(0);
    emit fontSizeChanged(m_baseFontSize);
}

void Editor::setShowLineNumbers(bool on)
{
    m_showLineNumbers = on;
    updateLineNumberAreaWidth(0);
}

void Editor::selectLineAtY(int y)
{
    QTextBlock block = firstVisibleBlock();
    if (!block.isValid())
        return;

    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid()) {
        const int height = qMax(1, qRound(blockBoundingRect(block).height()));
        if (y >= top && y < top + height) {
            QTextCursor cursor(block);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            setTextCursor(cursor);
            setFocus(Qt::MouseFocusReason);
            return;
        }
        if (top > y)
            return;
        top += height;
        block = block.next();
    }
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
    all << m_currentLine << m_wordSelections << m_searchSelections << m_bracketSelections;
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

void Editor::updateBracketMatch()
{
    m_bracketSelections.clear();
    const int documentEnd = document()->characterCount() - 1;
    const int cursorPosition = textCursor().position();
    int bracketPosition = -1;

    auto isBracket = [](QChar ch) {
        return isOpeningBracket(ch) || isClosingBracket(ch);
    };
    if (cursorPosition > 0 && isBracket(document()->characterAt(cursorPosition - 1)))
        bracketPosition = cursorPosition - 1;
    else if (cursorPosition < documentEnd && isBracket(document()->characterAt(cursorPosition)))
        bracketPosition = cursorPosition;

    if (bracketPosition >= 0) {
        constexpr int scanLimit = 20000;
        const int start = qMax(0, bracketPosition - scanLimit);
        const int end = qMin(documentEnd, bracketPosition + scanLimit + 1);
        QTextCursor range(document());
        range.setPosition(start);
        range.setPosition(end, QTextCursor::KeepAnchor);
        const QString fragment = range.selectedText();
        const int match = findMatchingBracket(fragment, bracketPosition - start, scanLimit);

        auto addSelection = [this](int position) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(m_palette.braceMatch);
            selection.cursor = QTextCursor(document());
            selection.cursor.setPosition(position);
            selection.cursor.setPosition(position + 1, QTextCursor::KeepAnchor);
            m_bracketSelections.append(selection);
        };
        addSelection(bracketPosition);
        if (match >= 0)
            addSelection(start + match);
    }
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

    // Home alternates between the first non-whitespace character and column 0.
    if (event->key() == Qt::Key_Home
        && (mods == Qt::NoModifier || mods == Qt::ShiftModifier)) {
        QTextCursor cursor = textCursor();
        const int target = smartHomeTarget(cursor.block().text(), cursor.positionInBlock());
        cursor.setPosition(cursor.block().position() + target,
                           mods == Qt::ShiftModifier ? QTextCursor::KeepAnchor
                                                    : QTextCursor::MoveAnchor);
        setTextCursor(cursor);
        event->accept();
        return;
    }

    // Ctrl+Backspace / Ctrl+Delete remove the previous / next word.
    if (mods == Qt::ControlModifier
        && (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)) {
        QTextCursor cursor = textCursor();
        if (!cursor.hasSelection()) {
            cursor.movePosition(event->key() == Qt::Key_Backspace ? QTextCursor::PreviousWord
                                                                  : QTextCursor::NextWord,
                                QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
        setTextCursor(cursor);
        event->accept();
        return;
    }

    // Backspace removes an empty auto-inserted pair as one unit.
    if (m_autoCloseBrackets && event->key() == Qt::Key_Backspace
        && mods == Qt::NoModifier) {
        QTextCursor cursor = textCursor();
        const int position = cursor.position();
        const int end = document()->characterCount() - 1;
        if (!cursor.hasSelection() && position > 0 && position < end) {
            const QChar before = document()->characterAt(position - 1);
            const QChar after = document()->characterAt(position);
            if (closingFor(before) == after) {
                cursor.setPosition(position - 1);
                cursor.setPosition(position + 1, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                setTextCursor(cursor);
                event->accept();
                return;
            }
        }
    }

    // Auto indent on Enter, including a split block before an existing closer.
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && mods == Qt::NoModifier) {
        const QTextCursor current = textCursor();
        const QString lineText = current.block().text();
        QString indent;
        for (const QChar ch : lineText) {
            if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t'))
                indent += ch;
            else
                break;
        }
        const int col = current.positionInBlock();
        if (!current.hasSelection() && col > 0 && col <= lineText.size()
            && lineText.left(col).endsWith(QLatin1Char('{'))) {
            const QString rest = lineText.mid(col).trimmed();
            if (rest.startsWith(QLatin1Char('}'))) {
                const int blockPosition = current.block().position();
                QTextCursor cursor = current;
                cursor.beginEditBlock();
                cursor.setPosition(blockPosition + col);
                cursor.setPosition(blockPosition + lineText.size(), QTextCursor::KeepAnchor);
                cursor.insertText(QStringLiteral("\n") + indent + QStringLiteral("  \n")
                                  + indent + rest);
                cursor.endEditBlock();
                cursor.setPosition(blockPosition + col + 1 + indent.size() + 2);
                setTextCursor(cursor);
                event->accept();
                return;
            }
        }
        if (lineText.left(col).endsWith(QLatin1Char('{')))
            indent += QStringLiteral("  ");
        insertPlainText(QStringLiteral("\n") + indent);
        event->accept();
        return;
    }

    // Tab / Shift+Tab: indent or dedent (multi-line aware)
    if (event->key() == Qt::Key_Tab
        && (mods == Qt::NoModifier || mods == Qt::ShiftModifier)) {
        const QTextCursor c = textCursor();
        if (c.hasSelection() && c.selectedText().contains(QChar(0x2029))) {
            indentSelection(mods == Qt::ShiftModifier);
            event->accept();
            return;
        }
        if (mods == Qt::NoModifier) {
            insertPlainText(QStringLiteral("\t"));
            event->accept();
            return;
        }
    }

    if (handleTypingKey(event))
        return;

    QPlainTextEdit::keyPressEvent(event);
}

bool Editor::handleTypingKey(QKeyEvent *event)
{
    if (!m_autoCloseBrackets)
        return false;
    if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return false;

    const QString text = event->text();
    if (text.size() != 1)
        return false;

    const QChar typed = text.at(0);
    QTextCursor cursor = textCursor();
    const int position = cursor.position();
    const int documentEnd = document()->characterCount() - 1;

    // Type over an automatically inserted closing bracket or quote.
    if (!cursor.hasSelection() && position < documentEnd
        && isPairedCloser(typed) && typed == document()->characterAt(position)) {
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        event->accept();
        return true;
    }

    const QChar closer = closingFor(typed);
    if (closer.isNull())
        return false;

    const bool isQuote = typed == QLatin1Char('"') || typed == QLatin1Char('\'');
    if (isQuote && !cursor.hasSelection()) {
        const QChar before = position > 0 ? document()->characterAt(position - 1) : QChar();
        const QChar after = position < documentEnd ? document()->characterAt(position) : QChar();
        // Avoid inserting a closing quote in the middle of a word (for example, don't).
        if (isWordCharacter(before) && isWordCharacter(after))
            return false;
    }

    cursor.beginEditBlock();
    if (cursor.hasSelection()) {
        const int selectionStart = cursor.selectionStart();
        const int selectionEnd = cursor.selectionEnd();
        cursor.setPosition(selectionEnd);
        cursor.insertText(QString(closer));
        cursor.setPosition(selectionStart);
        cursor.insertText(QString(typed));
        cursor.setPosition(selectionStart + 1);
        cursor.setPosition(selectionEnd + 1, QTextCursor::KeepAnchor);
    } else {
        cursor.insertText(QString(typed) + closer);
        cursor.movePosition(QTextCursor::Left);
    }
    cursor.endEditBlock();
    setTextCursor(cursor);
    event->accept();
    return true;
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

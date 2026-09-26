#include "highlighter.h"

#include <QRegularExpression>
#include <QTextBlock>

namespace {

enum BlockState : int {
    InCode = 0,
    InBlockComment = 1,
    InTripleDouble = 2,
    InTripleSingle = 3,
};

inline bool isIdentChar(QChar c)
{
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

inline bool isHexDigit(QChar c)
{
    return (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
        || (c >= QLatin1Char('A') && c <= QLatin1Char('F'));
}

} // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    setLanguage(&Languages::PlainText);
}

void SyntaxHighlighter::setLanguage(const LanguageDef *lang)
{
    m_lang = lang ? lang : &Languages::PlainText;
    mKeywordSet = QSet<QString>(m_lang->keywords.begin(), m_lang->keywords.end());
    mControlSet = QSet<QString>(m_lang->control.begin(), m_lang->control.end());
    mTypeSet = QSet<QString>(m_lang->types.begin(), m_lang->types.end());
    mLiteralSet = QSet<QString>(m_lang->literals.begin(), m_lang->literals.end());
    rehighlight();
}

void SyntaxHighlighter::setColors(const SyntaxColors &colors)
{
    m_colors = colors;
    rebuildFormats();
    rehighlight();
}

void SyntaxHighlighter::rebuildFormats()
{
    mKeyword.setForeground(m_colors.keyword);
    mControl.setForeground(m_colors.control);
    mType.setForeground(m_colors.type);
    mLiteral.setForeground(m_colors.literal);
    mString.setForeground(m_colors.string);
    mNumber.setForeground(m_colors.number);
    mComment.setForeground(m_colors.comment);
    mComment.setFontItalic(true);
    mFunction.setForeground(m_colors.function);
    mPre.setForeground(m_colors.preprocessor);
    mTag.setForeground(m_colors.tag);
    mAttribute.setForeground(m_colors.attribute);
    mEntity.setForeground(m_colors.entity);
}

static int scanString(const QString &text, int start, QChar quote, bool escapes)
{
    // Returns index just past the closing quote (or text length if unterminated).
    const int n = text.size();
    int i = start + 1;
    while (i < n) {
        const QChar c = text.at(i);
        if (escapes && c == QLatin1Char('\\') && i + 1 < n) {
            i += 2;
            continue;
        }
        if (c == quote)
            return i + 1;
        ++i;
    }
    return n;
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    const LanguageDef &lang = *m_lang;
    const int n = text.size();

    if (n == 0) {
        setCurrentBlockState(InCode);
        return;
    }

    int pos = 0;
    int state = previousBlockState();
    if (state < 0)
        state = InCode;

    setCurrentBlockState(InCode);

    // ----- Continue multi-line spans from the previous block -----------------
    if (state == InBlockComment) {
        const int end = text.indexOf(lang.blockCommentEnd, 0);
        if (end < 0) {
            setFormat(0, n, mComment);
            setCurrentBlockState(InBlockComment);
            return;
        }
        const int len = end + lang.blockCommentEnd.size();
        setFormat(0, len, mComment);
        pos = len;
    } else if (state == InTripleDouble || state == InTripleSingle) {
        const QString delim = (state == InTripleDouble) ? QStringLiteral("\"\"\"")
                                                        : QStringLiteral("'''");
        const int end = text.indexOf(delim, 0);
        if (end < 0) {
            setFormat(0, n, mString);
            setCurrentBlockState(state);
            return;
        }
        const int len = end + 3;
        setFormat(0, len, mString);
        pos = len;
    }

    const bool markup = lang.markup;
    bool lineStart = true; // only whitespace seen so far on this line

    // ----- Markup mode (HTML / XML) ------------------------------------------
    auto highlightTag = [&](int from, int to) {
        // '<' [/] name ... '>' with attributes and strings
        int i = from;
        if (i < to && text.at(i) == QLatin1Char('<'))
            ++i;
        if (i < to && text.at(i) == QLatin1Char('/'))
            ++i;
        int nameStart = i;
        while (i < to && (isIdentChar(text.at(i)) || text.at(i) == QLatin1Char('-')
                          || text.at(i) == QLatin1Char(':') || text.at(i) == QLatin1Char('.')))
            ++i;
        if (i > nameStart)
            setFormat(nameStart, i - nameStart, mTag);
        while (i < to) {
            const QChar c = text.at(i);
            if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
                const int end = scanString(text, i, c, false);
                setFormat(i, qMin(end, to) - i, mString);
                i = qMin(end, to);
            } else if (c.isLetter() || c == QLatin1Char('_')) {
                const int a = i;
                while (i < to && isIdentChar(text.at(i)))
                    ++i;
                if (i < to && text.at(i) == QLatin1Char('='))
                    setFormat(a, i - a, mAttribute);
            } else {
                ++i;
            }
        }
    };

    // ----- Generic scanner -----------------------------------------------------
    while (pos < n) {
        const QChar c = text.at(pos);

        // whitespace / line-start tracking
        if (c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
            ++pos;
            continue;
        }

        if (markup) {
            if (c == QLatin1Char('<')) {
                const int end = text.indexOf(QLatin1Char('>'), pos);
                const int to = (end < 0) ? n : end + 1;
                highlightTag(pos, to);
                pos = to;
                lineStart = false;
                continue;
            }
            if (c == QLatin1Char('&')) {
                int i = pos + 1;
                while (i < n && i - pos <= 10 && text.at(i) != QLatin1Char(';'))
                    ++i;
                if (i < n && text.at(i) == QLatin1Char(';')) {
                    setFormat(pos, i - pos + 1, mEntity);
                    pos = i + 1;
                    lineStart = false;
                    continue;
                }
            }
            lineStart = false;
            ++pos;
            continue;
        }

        // Line comments
        bool wasComment = false;
        for (const QString &lc : lang.lineComments) {
            if (text.indexOf(lc, pos) == pos) {
                setFormat(pos, n - pos, mComment);
                pos = n;
                wasComment = true;
                break;
            }
        }
        if (wasComment)
            break;

        // Block comments
        if (!lang.blockCommentStart.isEmpty() && text.indexOf(lang.blockCommentStart, pos) == pos) {
            const int end = text.indexOf(lang.blockCommentEnd, pos + lang.blockCommentStart.size());
            if (end < 0) {
                setFormat(pos, n - pos, mComment);
                setCurrentBlockState(InBlockComment);
                return;
            }
            const int len = end + lang.blockCommentEnd.size() - pos;
            setFormat(pos, len, mComment);
            pos += len;
            lineStart = false;
            continue;
        }

        // Python triple-quoted strings
        if (lang.tripleQuotedStrings && text.indexOf(QStringLiteral("\"\"\""), pos) == pos) {
            const int end = text.indexOf(QStringLiteral("\"\"\""), pos + 3);
            if (end < 0) {
                setFormat(pos, n - pos, mString);
                setCurrentBlockState(InTripleDouble);
                return;
            }
            const int len = end + 3 - pos;
            setFormat(pos, len, mString);
            pos += len;
            lineStart = false;
            continue;
        }
        if (lang.tripleQuotedStrings && text.indexOf(QStringLiteral("'''"), pos) == pos) {
            const int end = text.indexOf(QStringLiteral("'''"), pos + 3);
            if (end < 0) {
                setFormat(pos, n - pos, mString);
                setCurrentBlockState(InTripleSingle);
                return;
            }
            const int len = end + 3 - pos;
            setFormat(pos, len, mString);
            pos += len;
            lineStart = false;
            continue;
        }

        // Strings
        if (c == QLatin1Char('"') && lang.hasStrings) {
            const int end = scanString(text, pos, QLatin1Char('"'), true);
            setFormat(pos, end - pos, mString);
            pos = end;
            lineStart = false;
            continue;
        }
        if (c == QLatin1Char('\'') && lang.hasCharLiterals) {
            const int end = scanString(text, pos, QLatin1Char('\''), true);
            setFormat(pos, end - pos, mString);
            pos = end;
            lineStart = false;
            continue;
        }
        if (c == QLatin1Char('`') && lang.backtickStrings) {
            const int end = scanString(text, pos, QLatin1Char('`'), true);
            setFormat(pos, end - pos, mString);
            pos = end;
            lineStart = false;
            continue;
        }

        // Preprocessor / decorators / attributes / at-rules
        if (c == QLatin1Char('#') && lineStart) {
            if (lang.attributeBracket && pos + 1 < n && text.at(pos + 1) == QLatin1Char('[')) {
                int end = pos + 1;
                int depth = 0;
                while (end < n) {
                    if (text.at(end) == QLatin1Char('['))
                        ++depth;
                    else if (text.at(end) == QLatin1Char(']')) {
                        --depth;
                        if (depth == 0) {
                            ++end;
                            break;
                        }
                    }
                    ++end;
                }
                setFormat(pos, end - pos, mPre);
                pos = end;
                lineStart = false;
                continue;
            }
            if (lang.preprocessor) {
                int i = pos + 1;
                while (i < n && (text.at(i).isLetterOrNumber() || text.at(i) == QLatin1Char('_')))
                    ++i;
                setFormat(pos, i - pos, mPre);
                // #include <...>
                int j = i;
                while (j < n && text.at(j).isSpace())
                    ++j;
                if (i - pos > 1 && j < n && text.at(j) == QLatin1Char('<')) {
                    const int close = text.indexOf(QLatin1Char('>'), j);
                    const int to = (close < 0) ? n : close + 1;
                    setFormat(j, to - j, mString);
                    i = to;
                }
                pos = i;
                lineStart = false;
                continue;
            }
        }
        if (c == QLatin1Char('@') && lineStart && (lang.decorator || lang.atRules)) {
            int i = pos + 1;
            while (i < n && isIdentChar(text.at(i)))
                ++i;
            setFormat(pos, i - pos, mPre);
            pos = i;
            lineStart = false;
            continue;
        }

        // INI sections and key=value
        if (lang.ini && c == QLatin1Char('[')) {
            const int end = text.indexOf(QLatin1Char(']'), pos);
            const int to = (end < 0) ? n : end + 1;
            setFormat(pos, to - pos, mTag);
            pos = to;
            lineStart = false;
            continue;
        }
        if (lang.ini) {
            const int eq = text.indexOf(QLatin1Char('='), pos);
            if (eq >= 0) {
                const int valStart = eq + 1;
                // key
                int k = pos;
                while (k < eq && text.at(k).isSpace())
                    ++k;
                if (k < eq)
                    setFormat(k, eq - k, mAttribute);
                // value
                int v = valStart;
                while (v < n && text.at(v).isSpace())
                    ++v;
                if (v < n)
                    setFormat(v, n - v, mString);
                pos = n;
                continue;
            }
            lineStart = false;
            ++pos;
            continue;
        }

        // CSS hex colors #aabbcc
        if (c == QLatin1Char('#') && lang.hashColors && pos + 1 < n && isHexDigit(text.at(pos + 1))) {
            int i = pos + 1;
            while (i < n && isHexDigit(text.at(i)))
                ++i;
            setFormat(pos, i - pos, mNumber);
            pos = i;
            lineStart = false;
            continue;
        }

        // Numbers
        if (c.isDigit()) {
            int i = pos;
            while (i < n && (text.at(i).isLetterOrNumber() || text.at(i) == QLatin1Char('.')
                             || text.at(i) == QLatin1Char('_')))
                ++i;
            // exponent sign: 1e-5
            if (i < n && (text.at(i) == QLatin1Char('+') || text.at(i) == QLatin1Char('-'))
                && i > pos && (text.at(i - 1) == QLatin1Char('e') || text.at(i - 1) == QLatin1Char('E'))) {
                ++i;
                while (i < n && (text.at(i).isLetterOrNumber() || text.at(i) == QLatin1Char('.')))
                    ++i;
            }
            setFormat(pos, i - pos, mNumber);
            pos = i;
            lineStart = false;
            continue;
        }

        // Identifiers / keywords
        if (c.isLetter() || c == QLatin1Char('_')) {
            const int start = pos;
            while (pos < n && isIdentChar(text.at(pos)))
                ++pos;
            const QString word = text.mid(start, pos - start);
            if (mControlSet.contains(word)) {
                setFormat(start, pos - start, mControl);
            } else if (mKeywordSet.contains(word)) {
                setFormat(start, pos - start, mKeyword);
            } else if (mTypeSet.contains(word)) {
                setFormat(start, pos - start, mType);
            } else if (mLiteralSet.contains(word)) {
                setFormat(start, pos - start, mLiteral);
            } else {
                int k = pos;
                while (k < n && text.at(k).isSpace())
                    ++k;
                if (k < n && text.at(k) == QLatin1Char('('))
                    setFormat(start, pos - start, mFunction);
            }
            lineStart = false;
            continue;
        }

        lineStart = false;
        ++pos;
    }
}

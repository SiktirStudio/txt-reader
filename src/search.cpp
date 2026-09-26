#include "search.h"

namespace Search {

QList<QTextCursor> findAll(QTextDocument *doc, const QRegularExpression &re, int limit)
{
    QList<QTextCursor> out;
    if (!doc || !re.isValid() || re.pattern().isEmpty())
        return out;
    const QString text = doc->toPlainText();
    auto it = re.globalMatch(text);
    while (it.hasNext() && out.size() < limit) {
        const QRegularExpressionMatch m = it.next();
        if (m.capturedEnd() == m.capturedStart())
            continue; // skip zero-width matches
        QTextCursor c(doc);
        c.setPosition(m.capturedStart());
        c.setPosition(m.capturedEnd(), QTextCursor::KeepAnchor);
        out.append(c);
    }
    return out;
}

QRegularExpression buildPattern(const QString &text, bool matchCase, bool wholeWord, bool regex)
{
    QString pattern = regex ? text : QRegularExpression::escape(text);
    if (wholeWord)
        pattern = QStringLiteral("\\b(?:") + pattern + QStringLiteral(")\\b");
    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;
    if (!matchCase)
        options |= QRegularExpression::CaseInsensitiveOption;
    return QRegularExpression(pattern, options);
}

QString expandBackrefs(const QString &replacement, const QRegularExpressionMatch &match)
{
    QString out;
    out.reserve(replacement.size());
    for (int i = 0; i < replacement.size(); ++i) {
        const QChar ch = replacement.at(i);
        if ((ch == QLatin1Char('\\') || ch == QLatin1Char('$')) && i + 1 < replacement.size()) {
            const QChar next = replacement.at(i + 1);
            if (next.isDigit()) {
                const int group = next.digitValue();
                if (group <= match.lastCapturedIndex())
                    out += match.captured(group);
                ++i;
                continue;
            }
            if (ch == QLatin1Char('\\')) {
                if (next == QLatin1Char('n')) {
                    out += QLatin1Char('\n');
                    ++i;
                    continue;
                }
                if (next == QLatin1Char('t')) {
                    out += QLatin1Char('\t');
                    ++i;
                    continue;
                }
                if (next == QLatin1Char('\\')) {
                    out += QLatin1Char('\\');
                    ++i;
                    continue;
                }
            }
        }
        out += ch;
    }
    return out;
}

int replaceAll(QTextDocument *doc, const QRegularExpression &re, const QString &replacement)
{
    if (!doc || !re.isValid() || re.pattern().isEmpty())
        return -1;

    struct Replacement {
        int pos = 0;
        int len = 0;
        QString text;
    };
    QList<Replacement> reps;

    const QString text = doc->toPlainText();
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (m.capturedEnd() == m.capturedStart())
            continue; // skip zero-width matches
        Replacement r;
        r.pos = m.capturedStart();
        r.len = m.capturedLength();
        r.text = expandBackrefs(replacement, m);
        reps.append(r);
        if (reps.size() >= 100000)
            break;
    }

    // Apply from the end so earlier offsets stay valid.
    QTextCursor k(doc);
    k.beginEditBlock();
    for (int i = reps.size() - 1; i >= 0; --i) {
        k.setPosition(reps.at(i).pos);
        k.setPosition(reps.at(i).pos + reps.at(i).len, QTextCursor::KeepAnchor);
        k.insertText(reps.at(i).text);
    }
    k.endEditBlock();
    return reps.size();
}

} // namespace Search

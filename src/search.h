#pragma once

#include <QList>
#include <QRegularExpression>
#include <QTextCursor>

// Reusable search/replace primitives (unit-tested by --selftest).
namespace Search {

// All non-empty matches of `re` in the document, as cursors with a selection.
QList<QTextCursor> findAll(QTextDocument *doc, const QRegularExpression &re, int limit = 10000);

// Builds a regular expression from the user's find criteria.
QRegularExpression buildPattern(const QString &text, bool matchCase, bool wholeWord, bool regex);

// Expands \1..\9, $1..$9, \n, \t and \\ in a replacement string.
QString expandBackrefs(const QString &replacement, const QRegularExpressionMatch &match);

// Replaces every non-empty match, as a single undo step. Returns the number
// of replacements (or -1 on invalid input).
int replaceAll(QTextDocument *doc, const QRegularExpression &re, const QString &replacement);

} // namespace Search

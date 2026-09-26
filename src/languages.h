#pragma once

#include <QString>
#include <QStringList>

// Declarative description of a language for the syntax highlighter.
struct LanguageDef {
    QString name;
    QStringList extensions;   // lower-case, without the dot
    QStringList keywords;
    QStringList control;      // control-flow keywords (separate color)
    QStringList types;
    QStringList literals;
    QStringList lineComments; // one or more line-comment prefixes
    QString blockCommentStart;
    QString blockCommentEnd;

    bool hasStrings = true;
    bool hasCharLiterals = true;
    bool preprocessor = false;         // '#word' at line start (C/C++/C#)
    bool decorator = false;            // '@word' at line start (Python)
    bool attributeBracket = false;     // '#[...]' at line start (Rust)
    bool atRules = false;              // '@word' at line start (CSS)
    bool hashColors = false;           // '#rrggbb' (CSS)
    bool tripleQuotedStrings = false;  // Python """..."""
    bool backtickStrings = false;      // JS template strings
    bool markup = false;               // HTML / XML
    bool ini = false;                  // [section] + key=value
};

namespace Languages {

extern const LanguageDef PlainText;

const LanguageDef *detect(const QString &fileName);
const LanguageDef *byName(const QString &name);
const QList<LanguageDef> &all();

} // namespace Languages

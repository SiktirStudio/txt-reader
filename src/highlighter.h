#pragma once

#include <QSet>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

#include "languages.h"
#include "themes.h"

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QTextDocument *document);

    void setLanguage(const LanguageDef *lang);
    void setColors(const SyntaxColors &colors);

    const LanguageDef *language() const { return m_lang; }

protected:
    void highlightBlock(const QString &text) override;

private:
    void rebuildFormats();

    const LanguageDef *m_lang = &Languages::PlainText;
    SyntaxColors m_colors;
    QTextCharFormat mKeyword, mControl, mType, mLiteral, mString, mNumber,
        mComment, mFunction, mPre, mTag, mAttribute, mEntity;
    QSet<QString> mKeywordSet, mControlSet, mTypeSet, mLiteralSet;
};

#include "fileio.h"

#include <QString>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringEncoder>
#endif

namespace FileIo {

Decoded decode(const QByteArray &raw)
{
    Decoded d;
    QByteArray body = raw;
    if (raw.startsWith(QByteArrayLiteral("\xEF\xBB\xBF"))) {
        d.bom = true;
        body = raw.mid(3);
    }

    QString text;
    bool validUtf8 = true;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    text = utf8Decoder.decode(body);
    validUtf8 = !utf8Decoder.hasError();
    if (!validUtf8) {
        const QByteArray forSystem = d.bom ? body : raw;
        QStringDecoder systemDecoder(QStringConverter::System);
        text = systemDecoder.decode(forSystem);
        if (systemDecoder.hasError())
            text = QString::fromLatin1(forSystem);
    }
#else
    text = QString::fromUtf8(body);
    validUtf8 = true;
#endif

    if (text.contains(QLatin1String("\r\n")))
        d.crlf = true;
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    d.text = text;
    if (d.bom)
        d.encodingLabel = QStringLiteral("UTF-8-BOM");
    else if (validUtf8)
        d.encodingLabel = QStringLiteral("UTF-8");
    else
        d.encodingLabel = QStringLiteral("System (ANSI)");
    return d;
}

QByteArray encode(const QString &text, bool crlf, bool bom)
{
    QString t = text;
    if (crlf)
        t.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
    QByteArray out;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringEncoder encoder(QStringConverter::Utf8);
    out = encoder.encode(t);
#else
    out = t.toUtf8();
#endif
    if (bom)
        out.prepend(QByteArrayLiteral("\xEF\xBB\xBF"));
    return out;
}

} // namespace FileIo

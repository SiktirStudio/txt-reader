#pragma once

#include <QByteArray>
#include <QString>

// File decoding/encoding helpers (unit-tested by --selftest).
namespace FileIo {

struct Decoded {
    QString text;          // always uses '\n' line endings
    QString encodingLabel; // "UTF-8", "UTF-8-BOM", "System (ANSI)"
    bool bom = false;
    bool crlf = false;
};

// Decodes raw file bytes: detects a UTF-8 BOM, falls back to the system
// locale when the content is not valid UTF-8, and normalizes line endings.
Decoded decode(const QByteArray &raw);

// Encodes text for saving: UTF-8, optional BOM, LF or CRLF line endings.
QByteArray encode(const QString &text, bool crlf, bool bom);

} // namespace FileIo

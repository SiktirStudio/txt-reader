#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

// All toolbar / action icons are drawn programmatically so they always match
// the active theme color.
namespace Icons {

QIcon fileNew(const QColor &c);
QIcon fileOpen(const QColor &c);
QIcon fileSave(const QColor &c);
QIcon fileSaveAll(const QColor &c);
QIcon filePrint(const QColor &c);
QIcon editUndo(const QColor &c);
QIcon editRedo(const QColor &c);
QIcon editFind(const QColor &c);
QIcon editReplace(const QColor &c);
QIcon plus(const QColor &c);
QIcon chevronUp(const QColor &c);
QIcon chevronDown(const QColor &c);
QIcon close(const QColor &c);
QIcon docs(const QColor &c);

// Small PNGs written at runtime and referenced from the stylesheet
// (QSS image: urls can point to files).
void writeCheckIcon(const QString &path, const QColor &c);
void writeCloseIcon(const QString &path, const QColor &c);
void writeArrowIcon(const QString &path, const QColor &c);

} // namespace Icons

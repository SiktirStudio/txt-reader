#include "icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>

#include <functional>

namespace Icons {

namespace {

using PaintFn = void (*)(QPainter &);

QIcon make(const QColor &color, PaintFn paint)
{
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 5.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    paint(p);
    p.end();
    return QIcon(pm);
}

void drawFloppy(QPainter &p)
{
    QPainterPath body;
    body.addRoundedRect(14, 12, 36, 40, 4, 4);
    p.drawPath(body);
    QPainterPath label;
    label.moveTo(22, 26);
    label.lineTo(22, 12);
    label.lineTo(42, 12);
    label.lineTo(42, 26);
    label.closeSubpath();
    p.drawPath(label);
    p.drawRoundedRect(23, 33, 18, 19, 3, 3);
    p.drawLine(27, 44, 37, 44);
}

void drawUndoArrow(QPainter &p)
{
    // Curved tail
    QPainterPath tail;
    tail.moveTo(30, 40);
    tail.cubicTo(31, 18, 56, 18, 53, 42);
    p.drawPath(tail);
    // Arrow head pointing left
    QPen pen = p.pen();
    p.setPen(Qt::NoPen);
    p.setBrush(pen.color());
    QPolygonF head;
    head << QPointF(13, 40) << QPointF(31, 28) << QPointF(31, 52);
    p.drawPolygon(head);
    p.setBrush(Qt::NoBrush);
    p.setPen(pen);
}

void drawRedoArrow(QPainter &p)
{
    p.save();
    p.translate(64, 0);
    p.scale(-1, 1);
    drawUndoArrow(p);
    p.restore();
}

void drawMagnifier(QPainter &p, int cx, int cy, int r)
{
    p.drawEllipse(cx - r, cy - r, 2 * r, 2 * r);
    QPen pen = p.pen();
    p.setPen(QPen(pen.color(), pen.width() + 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(cx + r - 3, cy + r - 3, cx + r + 10, cy + r + 10);
    p.setPen(pen);
}

void drawChevronDown(QPainter &p)
{
    p.drawLine(18, 26, 32, 42);
    p.drawLine(32, 42, 46, 26);
}

void drawCloseX(QPainter &p, int a, int b)
{
    p.drawLine(a, a, b, b);
    p.drawLine(b, a, a, b);
}

void writePixmap(const QString &path, int size, const std::function<void(QPainter &)> &paint)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    paint(p);
    p.end();
    pm.save(path);
}

} // namespace

QIcon fileNew(const QColor &c)
{
    return make(c, [](QPainter &p) {
        QPainterPath page;
        page.moveTo(23, 9);
        page.lineTo(39, 9);
        page.lineTo(47, 17);
        page.lineTo(47, 55);
        page.lineTo(17, 55);
        page.lineTo(17, 9);
        page.closeSubpath();
        p.drawPath(page);
        p.drawLine(39, 9, 39, 17);
        p.drawLine(39, 17, 47, 17);
        // plus mark
        p.drawLine(24, 40, 40, 40);
        p.drawLine(32, 32, 32, 48);
    });
}

QIcon fileOpen(const QColor &c)
{
    return make(c, [](QPainter &p) {
        QPainterPath folder;
        folder.moveTo(11, 20);
        folder.lineTo(25, 20);
        folder.lineTo(31, 27);
        folder.lineTo(53, 27);
        folder.lineTo(53, 50);
        folder.lineTo(11, 50);
        folder.closeSubpath();
        p.drawPath(folder);
    });
}

QIcon fileSave(const QColor &c)
{
    return make(c, [](QPainter &p) { drawFloppy(p); });
}

QIcon fileSaveAll(const QColor &c)
{
    return make(c, [](QPainter &p) {
        p.save();
        p.translate(4, 1);
        p.scale(0.64, 0.64);
        drawFloppy(p);
        p.restore();
        p.save();
        p.translate(19, 17);
        p.scale(0.64, 0.64);
        drawFloppy(p);
        p.restore();
    });
}

QIcon filePrint(const QColor &c)
{
    return make(c, [](QPainter &p) {
        p.drawRoundedRect(22, 10, 20, 14, 2, 2);   // paper in
        p.drawRoundedRect(14, 24, 36, 18, 3, 3);   // body
        p.drawRect(24, 40, 16, 12);                // paper out
        QPen pen = p.pen();
        p.setPen(QPen(pen.color(), 6, Qt::SolidLine, Qt::RoundCap));
        p.drawPoint(43, 33);                       // button
        p.setPen(pen);
    });
}

QIcon editUndo(const QColor &c)
{
    return make(c, drawUndoArrow);
}

QIcon editRedo(const QColor &c)
{
    return make(c, drawRedoArrow);
}

QIcon editFind(const QColor &c)
{
    return make(c, [](QPainter &p) { drawMagnifier(p, 28, 28, 15); });
}

QIcon editReplace(const QColor &c)
{
    return make(c, [](QPainter &p) {
        p.drawLine(14, 21, 44, 21);
        p.drawLine(44, 21, 36, 14);
        p.drawLine(44, 21, 36, 28);
        p.drawLine(50, 43, 20, 43);
        p.drawLine(20, 43, 28, 36);
        p.drawLine(20, 43, 28, 50);
    });
}

QIcon plus(const QColor &c)
{
    return make(c, [](QPainter &p) {
        p.drawLine(32, 18, 32, 46);
        p.drawLine(18, 32, 46, 32);
    });
}

QIcon chevronUp(const QColor &c)
{
    return make(c, [](QPainter &p) {
        p.drawLine(18, 38, 32, 22);
        p.drawLine(32, 22, 46, 38);
    });
}

QIcon chevronDown(const QColor &c)
{
    return make(c, drawChevronDown);
}

QIcon close(const QColor &c)
{
    return make(c, [](QPainter &p) { drawCloseX(p, 20, 44); });
}

QIcon docs(const QColor &c)
{
    return make(c, [](QPainter &p) {
        QPainterPath back;
        back.moveTo(20, 12);
        back.lineTo(40, 12);
        back.lineTo(40, 18);
        back.lineTo(26, 18);
        back.lineTo(26, 50);
        back.lineTo(20, 50);
        back.closeSubpath();
        p.drawPath(back);
        QPainterPath front;
        front.moveTo(24, 14);
        front.lineTo(44, 14);
        front.lineTo(44, 52);
        front.lineTo(24, 52);
        front.closeSubpath();
        p.drawPath(front);
        p.drawLine(30, 26, 38, 26);
        p.drawLine(30, 34, 38, 34);
        p.drawLine(30, 42, 38, 42);
    });
}

void writeCheckIcon(const QString &path, const QColor &c)
{
    writePixmap(path, 30, [c](QPainter &p) {
        p.setPen(QPen(c, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(6, 16, 12, 22);
        p.drawLine(12, 22, 24, 8);
    });
}

void writeCloseIcon(const QString &path, const QColor &c)
{
    writePixmap(path, 30, [c](QPainter &p) {
        p.setPen(QPen(c, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(9, 9, 21, 21);
        p.drawLine(21, 9, 9, 21);
    });
}

void writeArrowIcon(const QString &path, const QColor &c)
{
    writePixmap(path, 26, [c](QPainter &p) {
        p.setPen(QPen(c, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(10, 5, 17, 13);
        p.drawLine(17, 13, 10, 21);
    });
}

} // namespace Icons

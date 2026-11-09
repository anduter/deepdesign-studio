#include "Icons.h"
#include "Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>

namespace Icons {
namespace {

QPolygonF starPolygon(const QPointF& center, double outer, double inner, int points)
{
    QPolygonF poly;
    for (int i = 0; i < points * 2; ++i) {
        const double radius = (i % 2 == 0) ? outer : inner;
        const double angle = -M_PI / 2.0 + i * M_PI / points;
        poly << QPointF(center.x() + radius * qCos(angle),
                        center.y() + radius * qSin(angle));
    }
    return poly;
}

QPolygonF regularPolygon(const QPointF& center, double radius, int sides)
{
    QPolygonF poly;
    for (int i = 0; i < sides; ++i) {
        const double angle = -M_PI / 2.0 + i * 2.0 * M_PI / sides;
        poly << QPointF(center.x() + radius * qCos(angle),
                        center.y() + radius * qSin(angle));
    }
    return poly;
}

void drawArrowHead(QPainter& p, const QPointF& tip, const QPointF& from, double size = 3.4)
{
    const QLineF line(tip, from);
    const double angle = std::atan2(line.dy(), line.dx());
    QPolygonF head;
    head << tip
         << tip + QPointF(size * qCos(angle + 0.55), size * qSin(angle + 0.55))
         << tip + QPointF(size * qCos(angle - 0.55), size * qSin(angle - 0.55));
    p.setBrush(p.pen().color());
    p.drawPolygon(head);
    p.setBrush(Qt::NoBrush);
}

} // namespace

void draw(QPainter& painter, const QString& name, const QRectF& rect,
          const QColor& color, double strokeWidth)
{
    painter.save();
    const double side = qMin(rect.width(), rect.height());
    painter.translate(rect.center());
    painter.translate(-side / 2.0, -side / 2.0);
    painter.scale(side / 24.0, side / 24.0);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QColor fill = color;
    fill.setAlpha(32);

    if (name == QLatin1String("select")) {
        QPainterPath path;
        path.moveTo(5.5, 3.5);
        path.lineTo(18.5, 12.0);
        path.lineTo(12.3, 13.2);
        path.lineTo(15.2, 20.2);
        path.lineTo(12.0, 21.6);
        path.lineTo(9.2, 14.5);
        path.lineTo(5.5, 17.3);
        path.closeSubpath();
        painter.setBrush(fill);
        painter.drawPath(path);
    } else if (name == QLatin1String("rect")) {
        painter.drawRoundedRect(QRectF(4, 5, 16, 14), 3, 3);
    } else if (name == QLatin1String("ellipse")) {
        painter.drawEllipse(QRectF(4, 4, 16, 16));
    } else if (name == QLatin1String("line")) {
        painter.drawLine(QPointF(5.5, 18.5), QPointF(18.5, 5.5));
        painter.setBrush(color);
        painter.drawEllipse(QPointF(5.0, 19.0), 1.8, 1.8);
        painter.drawEllipse(QPointF(19.0, 5.0), 1.8, 1.8);
    } else if (name == QLatin1String("polygon")) {
        painter.drawPolygon(regularPolygon(QPointF(12, 12), 8.5, 6));
    } else if (name == QLatin1String("star")) {
        painter.drawPolygon(starPolygon(QPointF(12, 12.2), 9.2, 4.0, 5));
    } else if (name == QLatin1String("pen")) {
        QPainterPath path;
        path.moveTo(12, 3);
        path.lineTo(18.8, 9.2);
        path.lineTo(12, 20.8);
        path.lineTo(5.2, 9.2);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawEllipse(QPointF(12, 9.3), 1.8, 1.8);
    } else if (name == QLatin1String("text")) {
        painter.drawLine(QPointF(5, 5.5), QPointF(19, 5.5));
        painter.drawLine(QPointF(12, 5.5), QPointF(12, 19.5));
        painter.drawLine(QPointF(8.5, 19.5), QPointF(15.5, 19.5));
        painter.drawLine(QPointF(7.5, 5.5), QPointF(7.5, 8));
        painter.drawLine(QPointF(16.5, 5.5), QPointF(16.5, 8));
    } else if (name == QLatin1String("hand")) {
        QPainterPath path;
        path.moveTo(7.5, 12.5);
        path.lineTo(7.5, 7.2);
        path.cubicTo(7.5, 6.1, 9.2, 6.1, 9.2, 7.2);
        path.lineTo(9.2, 11.2);
        path.lineTo(9.2, 5.3);
        path.cubicTo(9.2, 4.2, 10.9, 4.2, 10.9, 5.3);
        path.lineTo(10.9, 11.0);
        path.lineTo(10.9, 4.9);
        path.cubicTo(10.9, 3.8, 12.6, 3.8, 12.6, 4.9);
        path.lineTo(12.6, 11.2);
        path.lineTo(12.6, 6.2);
        path.cubicTo(12.6, 5.1, 14.3, 5.1, 14.3, 6.2);
        path.lineTo(14.3, 13.6);
        path.lineTo(15.8, 11.5);
        path.cubicTo(16.5, 10.4, 18.2, 11.4, 17.6, 12.7);
        path.lineTo(14.8, 18.2);
        path.cubicTo(14.1, 19.7, 13.0, 20.5, 11.2, 20.5);
        path.cubicTo(8.2, 20.5, 7.2, 18.5, 6.4, 16.2);
        path.lineTo(5.3, 13.2);
        path.cubicTo(4.9, 12.0, 6.7, 11.3, 7.5, 12.5);
        painter.drawPath(path);
    } else if (name == QLatin1String("zoom")) {
        painter.drawEllipse(QPointF(10.5, 10.5), 6.2, 6.2);
        painter.drawLine(QPointF(15.0, 15.0), QPointF(20.0, 20.0));
        painter.drawLine(QPointF(7.8, 10.5), QPointF(13.2, 10.5));
        painter.drawLine(QPointF(10.5, 7.8), QPointF(10.5, 13.2));
    } else if (name == QLatin1String("undo") || name == QLatin1String("redo")) {
        painter.save();
        if (name == QLatin1String("redo")) {
            painter.translate(24, 0);
            painter.scale(-1, 1);
        }
        QPainterPath path;
        path.moveTo(19.5, 17.5);
        path.cubicTo(19.5, 10.5, 13.5, 6.5, 7.0, 9.5);
        painter.drawPath(path);
        drawArrowHead(painter, QPointF(6.5, 9.6), QPointF(8.0, 9.4), 3.8);
        painter.restore();
    } else if (name == QLatin1String("save")) {
        painter.drawRoundedRect(QRectF(4, 4, 16, 16), 2.2, 2.2);
        painter.drawRect(QRectF(8, 4.6, 8, 5.2));
        painter.drawRoundedRect(QRectF(7, 13, 10, 6.4), 1.0, 1.0);
        painter.drawLine(QPointF(10, 16), QPointF(14, 16));
    } else if (name == QLatin1String("open")) {
        QPainterPath path;
        path.moveTo(3.2, 7.2);
        path.lineTo(9.2, 7.2);
        path.lineTo(11.2, 9.4);
        path.lineTo(20.8, 9.4);
        path.lineTo(20.8, 19.0);
        path.lineTo(3.2, 19.0);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(QPointF(3.2, 10.2), QPointF(20.8, 10.2));
    } else if (name == QLatin1String("new")) {
        QPainterPath path;
        path.moveTo(6, 3.5);
        path.lineTo(14.5, 3.5);
        path.lineTo(19, 8.0);
        path.lineTo(19, 20.5);
        path.lineTo(6, 20.5);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(QPointF(14.5, 3.5), QPointF(14.5, 8.0));
        painter.drawLine(QPointF(14.5, 8.0), QPointF(19, 8.0));
    } else if (name == QLatin1String("plus")) {
        painter.drawLine(QPointF(12, 5), QPointF(12, 19));
        painter.drawLine(QPointF(5, 12), QPointF(19, 12));
    } else if (name == QLatin1String("minus")) {
        painter.drawLine(QPointF(5, 12), QPointF(19, 12));
    } else if (name == QLatin1String("close")) {
        painter.drawLine(QPointF(6.5, 6.5), QPointF(17.5, 17.5));
        painter.drawLine(QPointF(17.5, 6.5), QPointF(6.5, 17.5));
    } else if (name == QLatin1String("trash")) {
        painter.drawLine(QPointF(4.5, 7), QPointF(19.5, 7));
        painter.drawLine(QPointF(9.5, 4.5), QPointF(14.5, 4.5));
        painter.drawLine(QPointF(6.5, 7), QPointF(7.4, 20));
        painter.drawLine(QPointF(17.5, 7), QPointF(16.6, 20));
        painter.drawLine(QPointF(7.4, 20), QPointF(16.6, 20));
        painter.drawLine(QPointF(10.2, 10), QPointF(10.5, 17));
        painter.drawLine(QPointF(13.8, 10), QPointF(13.5, 17));
    } else if (name == QLatin1String("duplicate")) {
        painter.drawRoundedRect(QRectF(4, 4, 12.5, 12.5), 2, 2);
        painter.drawRoundedRect(QRectF(7.5, 7.5, 12.5, 12.5), 2, 2);
    } else if (name == QLatin1String("layers")) {
        QPainterPath path;
        path.moveTo(12, 3.5);
        path.lineTo(21, 8.0);
        path.lineTo(12, 12.5);
        path.lineTo(3, 8.0);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawPolyline(QPolygonF() << QPointF(3.4, 12.3) << QPointF(12, 16.5) << QPointF(20.6, 12.3));
        painter.drawPolyline(QPolygonF() << QPointF(3.4, 16.3) << QPointF(12, 20.5) << QPointF(20.6, 16.3));
    } else if (name == QLatin1String("eye") || name == QLatin1String("eyeOff")) {
        QPainterPath path;
        path.moveTo(2.8, 12);
        path.cubicTo(6.2, 6.0, 17.8, 6.0, 21.2, 12);
        path.cubicTo(17.8, 18.0, 6.2, 18.0, 2.8, 12);
        painter.drawPath(path);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(12, 12), 2.7, 2.7);
        painter.setBrush(Qt::NoBrush);
        if (name == QLatin1String("eyeOff")) {
            painter.setPen(QPen(Theme::panel(), 3.4, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(4.5, 4.5), QPointF(19.5, 19.5));
            painter.setPen(pen);
            painter.drawLine(QPointF(4.5, 4.5), QPointF(19.5, 19.5));
        }
    } else if (name == QLatin1String("lock") || name == QLatin1String("unlock")) {
        painter.drawRoundedRect(QRectF(5, 10.5, 14, 9.5), 2, 2);
        QPainterPath shackle;
        if (name == QLatin1String("lock")) {
            shackle.moveTo(8.2, 10.5);
            shackle.lineTo(8.2, 7.3);
            shackle.cubicTo(8.2, 3.3, 15.8, 3.3, 15.8, 7.3);
            shackle.lineTo(15.8, 10.5);
        } else {
            shackle.moveTo(8.2, 10.5);
            shackle.lineTo(8.2, 7.3);
            shackle.cubicTo(8.2, 3.3, 15.8, 3.3, 15.8, 7.3);
            shackle.lineTo(15.8, 8.7);
        }
        painter.drawPath(shackle);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(12, 15.2), 1.2, 1.2);
    } else if (name == QLatin1String("grid")) {
        painter.drawRoundedRect(QRectF(3.5, 3.5, 17, 17), 2, 2);
        painter.drawLine(QPointF(12, 3.5), QPointF(12, 20.5));
        painter.drawLine(QPointF(3.5, 12), QPointF(20.5, 12));
    } else if (name == QLatin1String("play")) {
        QPainterPath path;
        path.moveTo(8, 4.8);
        path.lineTo(19, 12);
        path.lineTo(8, 19.2);
        path.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(path);
    } else if (name == QLatin1String("pause")) {
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(6.5, 5, 4, 14), 1, 1);
        painter.drawRoundedRect(QRectF(13.5, 5, 4, 14), 1, 1);
    } else if (name == QLatin1String("stop")) {
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(6, 6, 12, 12), 2, 2);
    } else if (name == QLatin1String("preview")) {
        painter.drawRoundedRect(QRectF(3, 4.5, 18, 15), 3, 3);
        QPainterPath path;
        path.moveTo(10, 8.8);
        path.lineTo(15.2, 12);
        path.lineTo(10, 15.2);
        path.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(path);
    } else if (name == QLatin1String("export")) {
        painter.drawPolyline(QPolygonF() << QPointF(4, 14) << QPointF(4, 20) << QPointF(20, 20) << QPointF(20, 14));
        painter.drawLine(QPointF(12, 15), QPointF(12, 4));
        painter.drawPolyline(QPolygonF() << QPointF(7.5, 8) << QPointF(12, 4) << QPointF(16.5, 8));
    } else if (name == QLatin1String("fit")) {
        painter.drawPolyline(QPolygonF() << QPointF(4, 9) << QPointF(4, 4) << QPointF(9, 4));
        painter.drawPolyline(QPolygonF() << QPointF(15, 4) << QPointF(20, 4) << QPointF(20, 9));
        painter.drawPolyline(QPolygonF() << QPointF(20, 15) << QPointF(20, 20) << QPointF(15, 20));
        painter.drawPolyline(QPolygonF() << QPointF(9, 20) << QPointF(4, 20) << QPointF(4, 15));
    } else if (name == QLatin1String("chevronUp")) {
        painter.drawPolyline(QPolygonF() << QPointF(6.5, 14.5) << QPointF(12, 9) << QPointF(17.5, 14.5));
    } else if (name == QLatin1String("chevronDown")) {
        painter.drawPolyline(QPolygonF() << QPointF(6.5, 9.5) << QPointF(12, 15) << QPointF(17.5, 9.5));
    } else if (name == QLatin1String("chevronLeft")) {
        painter.drawPolyline(QPolygonF() << QPointF(14.5, 6.5) << QPointF(9, 12) << QPointF(14.5, 17.5));
    } else if (name == QLatin1String("chevronRight")) {
        painter.drawPolyline(QPolygonF() << QPointF(9.5, 6.5) << QPointF(15, 12) << QPointF(9.5, 17.5));
    } else if (name == QLatin1String("sparkle")) {
        QPainterPath path;
        path.moveTo(12, 2.5);
        path.cubicTo(12.8, 8.0, 16.0, 11.2, 21.5, 12);
        path.cubicTo(16.0, 12.8, 12.8, 16.0, 12, 21.5);
        path.cubicTo(11.2, 16.0, 8.0, 12.8, 2.5, 12);
        path.cubicTo(8.0, 11.2, 11.2, 8.0, 12, 2.5);
        painter.setBrush(color);
        painter.drawPath(path);
    } else if (name == QLatin1String("more")) {
        painter.setBrush(color);
        painter.drawEllipse(QPointF(5.5, 12), 1.5, 1.5);
        painter.drawEllipse(QPointF(12, 12), 1.5, 1.5);
        painter.drawEllipse(QPointF(18.5, 12), 1.5, 1.5);
    } else if (name == QLatin1String("help")) {
        painter.drawEllipse(QPointF(12, 12), 8.8, 8.8);
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(11);
        painter.setFont(font);
        painter.drawText(QRectF(4, 4, 16, 16), Qt::AlignCenter, QStringLiteral("?"));
    } else if (name == QLatin1String("record")) {
        painter.setBrush(color);
        painter.drawEllipse(QPointF(12, 12), 4.6, 4.6);
    } else if (name == QLatin1String("animation")) {
        painter.drawEllipse(QPointF(12, 12), 8.5, 8.5);
        QPainterPath path;
        path.moveTo(10, 8.5);
        path.lineTo(15.5, 12);
        path.lineTo(10, 15.5);
        path.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(path);
    } else if (name == QLatin1String("check")) {
        painter.drawPolyline(QPolygonF() << QPointF(4.5, 12.5) << QPointF(9.5, 17.5) << QPointF(19.5, 6.5));
    } else if (name == QLatin1String("alignLeft")) {
        painter.drawLine(QPointF(4, 3.5), QPointF(4, 20.5));
        painter.setBrush(fill);
        painter.drawRect(QRectF(7, 5, 11, 5));
        painter.drawRect(QRectF(7, 14, 7, 5));
    } else if (name == QLatin1String("alignCenter")) {
        painter.drawLine(QPointF(12, 3.5), QPointF(12, 20.5));
        painter.setBrush(fill);
        painter.drawRect(QRectF(4.5, 5, 15, 5));
        painter.drawRect(QRectF(7, 14, 10, 5));
    } else if (name == QLatin1String("alignRight")) {
        painter.drawLine(QPointF(20, 3.5), QPointF(20, 20.5));
        painter.setBrush(fill);
        painter.drawRect(QRectF(6, 5, 11, 5));
        painter.drawRect(QRectF(10, 14, 7, 5));
    } else if (name == QLatin1String("eye")) {
        // handled above
    } else if (name == QLatin1String("refresh")) {
        painter.drawArc(QRectF(4, 4, 16, 16), 40 * 16, 280 * 16);
        drawArrowHead(painter, QPointF(19.4, 7.6), QPointF(18.0, 7.0), 3.6);
    }
    painter.restore();
}

QPixmap pixmap(const QString& name, int size, const QColor& color, double dpr)
{
    QPixmap pm(qRound(size * dpr), qRound(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    draw(painter, name, QRectF(0, 0, size, size), color, 1.7);
    painter.end();
    return pm;
}

QIcon icon(const QString& name, const QColor& color, int size)
{
    return QIcon(pixmap(name, size, color));
}

void drawLogo(QPainter& painter, const QRectF& rect, bool withGlow)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (withGlow) {
        QRadialGradient glow(rect.center(), rect.width() * 0.9);
        glow.setColorAt(0.0, QColor(108, 140, 255, 90));
        glow.setColorAt(1.0, QColor(108, 140, 255, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(rect.adjusted(-rect.width() * 0.35, -rect.height() * 0.35,
                                         rect.width() * 0.35, rect.height() * 0.35));
    }
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0.0, QColor("#7C9AFF"));
    gradient.setColorAt(0.55, QColor("#6C8CFF"));
    gradient.setColorAt(1.0, QColor("#9B6DFF"));
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(rect, rect.width() * 0.28, rect.width() * 0.28);

    QFont font(QStringLiteral("Segoe UI"));
    font.setBold(true);
    font.setPixelSize(qRound(rect.height() * 0.56));
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(rect.adjusted(0, -1, 0, 0), Qt::AlignCenter, QStringLiteral("D"));

    painter.setPen(QPen(QColor(255, 255, 255, 190), 1.2));
    painter.setBrush(Qt::NoBrush);
    const QPointF c = rect.center();
    painter.drawLine(QPointF(c.x() + rect.width() * 0.18, c.y() - rect.height() * 0.22),
                     QPointF(c.x() + rect.width() * 0.30, c.y() - rect.height() * 0.10));
    painter.restore();
}

} // namespace Icons

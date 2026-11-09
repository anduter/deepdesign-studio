#include "Renderer.h"

#include <QFont>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPolygonF>
#include <QRadialGradient>
#include <QTextOption>
#include <QtMath>

namespace Renderer {
namespace {

QPolygonF polygonFor(const Element& element)
{
    const QRectF r = element.boundingRect();
    const QPointF center = r.center();
    const double rx = r.width() / 2.0;
    const double ry = r.height() / 2.0;
    const int sides = qBound(3, element.sides, 60);
    QPolygonF poly;
    if (element.type == ElementType::Polygon) {
        for (int i = 0; i < sides; ++i) {
            const double angle = -M_PI / 2.0 + i * 2.0 * M_PI / sides;
            poly << QPointF(center.x() + rx * qCos(angle), center.y() + ry * qSin(angle));
        }
    } else {
        const int points = sides * 2;
        const double inner = 0.44;
        for (int i = 0; i < points; ++i) {
            const double radius = (i % 2 == 0) ? 1.0 : inner;
            const double angle = -M_PI / 2.0 + i * M_PI / sides;
            poly << QPointF(center.x() + rx * radius * qCos(angle),
                            center.y() + ry * radius * qSin(angle));
        }
    }
    return poly;
}

QPainterPath pathForElement(const Element& element)
{
    const QRectF r = element.boundingRect();
    QPainterPath path;
    switch (element.type) {
    case ElementType::Rectangle: {
        const double radius = qBound(0.0, element.radius, qMin(r.width(), r.height()) / 2.0);
        path.addRoundedRect(r, radius, radius);
        break;
    }
    case ElementType::Ellipse:
        path.addEllipse(r);
        break;
    case ElementType::Line:
        path.moveTo(QPointF(r.x() + element.lineStart.x() * r.width(),
                            r.y() + element.lineStart.y() * r.height()));
        path.lineTo(QPointF(r.x() + element.lineEnd.x() * r.width(),
                            r.y() + element.lineEnd.y() * r.height()));
        break;
    case ElementType::Polygon:
    case ElementType::Star:
        path.addPolygon(polygonFor(element));
        path.closeSubpath();
        break;
    case ElementType::Path: {
        if (element.points.isEmpty()) break;
        for (int i = 0; i < element.points.size(); ++i) {
            const QPointF p(r.x() + element.points[i].x() * r.width(),
                            r.y() + element.points[i].y() * r.height());
            if (i == 0) path.moveTo(p);
            else path.lineTo(p);
        }
        if (element.closed) path.closeSubpath();
        break;
    }
    case ElementType::Text:
        break;
    }
    return path;
}

QBrush fillBrushFor(const Element& element)
{
    if (!element.hasGradient()) return QBrush(element.fill);
    const GradientFill& gradient = element.gradient;
    QGradientStops stops;
    for (const auto& stop : gradient.stops) {
        stops.append(qMakePair(qBound(0.0, stop.position, 1.0), stop.color));
    }
    if (stops.isEmpty()) return QBrush(element.fill);
    if (stops.first().first > 0.0) stops.prepend(qMakePair(0.0, stops.first().second));
    if (stops.last().first < 1.0) stops.append(qMakePair(1.0, stops.last().second));

    const QRectF r = element.boundingRect();
    if (gradient.type == QLatin1String("radial")) {
        QRadialGradient radial(r.center(), qMax(r.width(), r.height()) * 0.62);
        radial.setStops(stops);
        return QBrush(radial);
    }
    const double radians = qDegreesToRadians(gradient.angle);
    const double dx = qCos(radians);
    const double dy = qSin(radians);
    const double half = (qAbs(r.width() * dx) + qAbs(r.height() * dy)) / 2.0;
    QLinearGradient linear(r.center() - QPointF(dx, dy) * half,
                           r.center() + QPointF(dx, dy) * half);
    linear.setStops(stops);
    return QBrush(linear);
}

} // namespace

void applyOverrides(Element& element, const QVariantMap& overrides)
{
    if (overrides.isEmpty()) return;
    QRectF frame = element.frame;
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        const QString& property = it.key();
        const QVariant& value = it.value();
        if (property == QLatin1String("x")) frame.moveLeft(value.toDouble());
        else if (property == QLatin1String("y")) frame.moveTop(value.toDouble());
        else if (property == QLatin1String("width")) frame.setWidth(qMax(0.01, value.toDouble()));
        else if (property == QLatin1String("height")) frame.setHeight(qMax(0.01, value.toDouble()));
        else if (property == QLatin1String("rotation")) element.rotation = value.toDouble();
        else if (property == QLatin1String("opacity")) element.opacity = qBound(0.0, value.toDouble(), 1.0);
        else if (property == QLatin1String("fill")) {
            if (value.canConvert<QColor>()) element.fill = value.value<QColor>();
            else {
                const QColor color(value.toString());
                if (color.isValid()) element.fill = color;
            }
        } else if (property == QLatin1String("strokeWidth")) {
            element.strokeWidth = qMax(0.0, value.toDouble());
        } else if (property == QLatin1String("fontSize")) {
            element.fontSize = qMax(1.0, value.toDouble());
        }
    }
    element.frame = frame;
}

QPainterPath shapePath(const Element& element)
{
    return pathForElement(element);
}

void paintElement(QPainter& painter, const Element& element, const QVariantMap& overrides)
{
    if (!element.visible) return;
    Element e = element;
    applyOverrides(e, overrides);
    if (e.opacity <= 0.001) return;

    painter.save();
    painter.setOpacity(painter.opacity() * e.opacity);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QPointF c = e.center();
    painter.translate(c);
    painter.rotate(e.rotation);
    painter.translate(-c);

    const QRectF r = e.boundingRect();
    if (e.type == ElementType::Text) {
        QFont font(e.fontFamily);
        font.setPixelSize(qMax(1, qRound(e.fontSize)));
        font.setBold(e.bold);
        font.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(font);
        painter.setPen(e.textColor);
        Qt::Alignment alignment = Qt::AlignVCenter;
        if (e.align == QLatin1String("left")) alignment |= Qt::AlignLeft;
        else if (e.align == QLatin1String("right")) alignment |= Qt::AlignRight;
        else alignment |= Qt::AlignHCenter;
        const QRectF textRect = r.adjusted(2, 2, -2, -2);
        painter.drawText(textRect, alignment | Qt::TextWordWrap, e.text);
        painter.restore();
        return;
    }

    if (e.type == ElementType::Line) {
        QColor lineColor = e.stroke;
        if (e.strokeWidth <= 0.0 || !e.stroke.isValid() || e.stroke.alpha() == 0) lineColor = e.fill;
        QPen pen(lineColor, qMax(1.0, e.strokeWidth), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(pathForElement(e));
        painter.restore();
        return;
    }

    QPainterPath path = pathForElement(e);
    painter.setBrush(fillBrushFor(e));
    if (e.strokeWidth > 0.0 && e.stroke.alpha() > 0) {
        painter.setPen(QPen(e.stroke, e.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    } else {
        painter.setPen(Qt::NoPen);
    }
    painter.drawPath(path);
    painter.restore();
}

void paintScene(QPainter& painter, const DesignDocument& document,
                const QHash<QString, QVariantMap>& overrides,
                bool showGrid, double zoom)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(QPointF(0, 0), document.canvasSize), document.background);

    if (showGrid) {
        double step = 20.0;
        if (zoom < 0.3) step = 100.0;
        else if (zoom < 0.6) step = 50.0;
        else if (zoom < 1.4) step = 40.0;
        QPen gridPen(QColor(255, 255, 255, 10), 1.0 / qMax(0.05, zoom));
        painter.setPen(gridPen);
        const int cols = int(document.canvasSize.width() / step) + 1;
        const int rows = int(document.canvasSize.height() / step) + 1;
        for (int i = 1; i < cols; ++i) {
            const double x = i * step;
            painter.drawLine(QPointF(x, 0), QPointF(x, document.canvasSize.height()));
        }
        for (int j = 1; j < rows; ++j) {
            const double y = j * step;
            painter.drawLine(QPointF(0, y), QPointF(document.canvasSize.width(), y));
        }
    }

    for (const auto& element : document.elements) {
        const auto it = overrides.constFind(element.id);
        Renderer::paintElement(painter, element, it == overrides.constEnd() ? QVariantMap() : it.value());
    }
    painter.restore();
}

} // namespace Renderer

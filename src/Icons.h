#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QRectF>
#include <QString>

class QPainter;

namespace Icons {

void draw(QPainter& painter, const QString& name, const QRectF& rect,
          const QColor& color, double strokeWidth = 1.7);
QPixmap pixmap(const QString& name, int size, const QColor& color, double dpr = 2.0);
QIcon icon(const QString& name, const QColor& color = QColor("#C9D2E3"), int size = 20);
void drawLogo(QPainter& painter, const QRectF& rect, bool withGlow = true);

} // namespace Icons

#pragma once

#include "Model.h"

#include <QHash>
#include <QPainterPath>
#include <QVariantMap>

class QPainter;

namespace Renderer {

void applyOverrides(Element& element, const QVariantMap& overrides);
QPainterPath shapePath(const Element& element);
void paintElement(QPainter& painter, const Element& element, const QVariantMap& overrides = {});
void paintScene(QPainter& painter, const DesignDocument& document,
                const QHash<QString, QVariantMap>& overrides,
                bool showGrid, double zoom);

} // namespace Renderer

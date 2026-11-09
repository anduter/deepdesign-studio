#pragma once

#include "Model.h"

#include <QImage>
#include <QString>

namespace ProjectIO {

bool save(const DesignDocument& document, const QString& path, QString* error = nullptr);
bool load(DesignDocument& document, const QString& path, QString* error = nullptr);
QImage renderToImage(const DesignDocument& document, double scale = 1.0, bool grid = false);
QString fileFilter();
QString defaultSuffix();

} // namespace ProjectIO

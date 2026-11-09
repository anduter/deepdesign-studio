#pragma once

#include "Model.h"

#include <QString>

namespace HtmlExporter {

QString generateHtml(const DesignDocument& document);
bool exportPackage(const DesignDocument& document, const QString& directory, QString* error = nullptr);

} // namespace HtmlExporter

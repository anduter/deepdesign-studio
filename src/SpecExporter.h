#pragma once

#include "Model.h"

#include <QString>

namespace SpecExporter {

QString generateUiSpec(const DesignDocument& document);
QString generateAiHandoff(const DesignDocument& document);
QString generatePrompt(const DesignDocument& document);
bool writeFiles(const DesignDocument& document, const QString& directory, QString* error = nullptr);

} // namespace SpecExporter

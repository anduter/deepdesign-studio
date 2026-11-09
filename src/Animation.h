#pragma once

#include <QString>
#include <QStringList>

// Easing curve helpers used by both the editor and the exported HTML.
double applyEasing(const QString& name, double t);
QStringList easingDisplayNames();
QStringList easingIds();
QString easingIdFromDisplay(const QString& display);
QString easingDisplayFromId(const QString& id);
QString easingToCss(const QString& id);

QString trackPropertyDisplayName(const QString& property);
QStringList animatableProperties();

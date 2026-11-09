#pragma once

#include <QColor>
#include <QFont>
#include <QString>

class QApplication;

namespace Theme {

inline QColor bg() { return QColor("#1E1E1E"); }
inline QColor panel() { return QColor("#2B2B2B"); }
inline QColor panelAlt() { return QColor("#303030"); }
inline QColor card() { return QColor("#3A3A3A"); }
inline QColor cardHover() { return QColor("#454545"); }
inline QColor border() { return QColor("#1A1A1A"); }
inline QColor borderLight() { return QColor("#4A4A4A"); }
inline QColor text() { return QColor("#D6D6D6"); }
inline QColor muted() { return QColor("#9A9A9A"); }
inline QColor subtle() { return QColor("#777777"); }
inline QColor accent() { return QColor("#2D8CEB"); }
inline QColor accent2() { return QColor("#5AA9F0"); }
inline QColor accentSoft() { return QColor(45, 140, 235, 45); }
inline QColor success() { return QColor("#3DDC97"); }
inline QColor warning() { return QColor("#FFB454"); }
inline QColor danger() { return QColor("#FF6B6B"); }
inline QColor canvas() { return QColor("#1A1A1A"); }

QString styleSheet();
QFont uiFont(int pointSize = 9, bool bold = false);
void apply(QApplication& app);

} // namespace Theme

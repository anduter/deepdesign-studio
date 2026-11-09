#include "Animation.h"

#include <QHash>
#include <QtMath>

namespace {
struct EasingEntry {
    const char* id;
    const char* display;
    const char* css;
};

const EasingEntry kEasings[] = {
    {"linear", "线性", "linear"},
    {"easeInQuad", "缓入", "cubic-bezier(0.55, 0.085, 0.68, 0.53)"},
    {"easeOutQuad", "缓出", "cubic-bezier(0.25, 0.46, 0.45, 0.94)"},
    {"easeInOutCubic", "缓入缓出", "cubic-bezier(0.645, 0.045, 0.355, 1)"},
    {"easeOutCubic", "平滑减速", "cubic-bezier(0.215, 0.61, 0.355, 1)"},
    {"easeOutBack", "回弹", "cubic-bezier(0.34, 1.56, 0.64, 1)"},
    {"easeOutElastic", "弹性", "cubic-bezier(0.22, 1.28, 0.36, 1)"},
    {"easeOutBounce", "弹跳", "cubic-bezier(0.34, 1.4, 0.64, 1)"},
};

const EasingEntry* entryForId(const QString& id) {
    for (const auto& e : kEasings) {
        if (id == QLatin1String(e.id)) return &e;
    }
    return nullptr;
}
} // namespace

double applyEasing(const QString& name, double t)
{
    t = qBound(0.0, t, 1.0);
    if (name == QLatin1String("easeInQuad")) return t * t;
    if (name == QLatin1String("easeOutQuad")) return 1.0 - (1.0 - t) * (1.0 - t);
    if (name == QLatin1String("easeInOutCubic")) {
        return t < 0.5 ? 4.0 * t * t * t : 1.0 - qPow(-2.0 * t + 2.0, 3.0) / 2.0;
    }
    if (name == QLatin1String("easeOutCubic")) return 1.0 - qPow(1.0 - t, 3.0);
    if (name == QLatin1String("easeOutBack")) {
        const double c1 = 1.70158;
        const double c3 = c1 + 1.0;
        return 1.0 + c3 * qPow(t - 1.0, 3.0) + c1 * qPow(t - 1.0, 2.0);
    }
    if (name == QLatin1String("easeOutElastic")) {
        if (t <= 0.0) return 0.0;
        if (t >= 1.0) return 1.0;
        const double c4 = (2.0 * M_PI) / 3.0;
        return qPow(2.0, -10.0 * t) * qSin((t * 10.0 - 0.75) * c4) + 1.0;
    }
    if (name == QLatin1String("easeOutBounce")) {
        const double n1 = 7.5625;
        const double d1 = 2.75;
        if (t < 1.0 / d1) return n1 * t * t;
        if (t < 2.0 / d1) {
            t -= 1.5 / d1;
            return n1 * t * t + 0.75;
        }
        if (t < 2.5 / d1) {
            t -= 2.25 / d1;
            return n1 * t * t + 0.9375;
        }
        t -= 2.625 / d1;
        return n1 * t * t + 0.984375;
    }
    return t;
}

QStringList easingDisplayNames()
{
    QStringList names;
    for (const auto& e : kEasings) names << QString::fromUtf8(e.display);
    return names;
}

QStringList easingIds()
{
    QStringList ids;
    for (const auto& e : kEasings) ids << QString::fromLatin1(e.id);
    return ids;
}

QString easingIdFromDisplay(const QString& display)
{
    for (const auto& e : kEasings) {
        if (display == QString::fromUtf8(e.display)) return QString::fromLatin1(e.id);
    }
    return QStringLiteral("easeOutCubic");
}

QString easingDisplayFromId(const QString& id)
{
    if (const auto* e = entryForId(id)) return QString::fromUtf8(e->display);
    return QString::fromUtf8(kEasings[4].display);
}

QString easingToCss(const QString& id)
{
    if (const auto* e = entryForId(id)) return QString::fromLatin1(e->css);
    return QString::fromLatin1(kEasings[4].css);
}

QString trackPropertyDisplayName(const QString& property)
{
    static const QHash<QString, QString> map = {
        {"x", QStringLiteral("X 位置")},
        {"y", QStringLiteral("Y 位置")},
        {"width", QStringLiteral("宽度")},
        {"height", QStringLiteral("高度")},
        {"rotation", QStringLiteral("旋转")},
        {"opacity", QStringLiteral("不透明度")},
        {"fill", QStringLiteral("填充颜色")},
        {"strokeWidth", QStringLiteral("描边宽度")},
        {"fontSize", QStringLiteral("字号")},
    };
    return map.value(property, property);
}

QStringList animatableProperties()
{
    return {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("width"),
            QStringLiteral("height"), QStringLiteral("rotation"), QStringLiteral("opacity"),
            QStringLiteral("fill"), QStringLiteral("strokeWidth")};
}

#include "Model.h"
#include "Animation.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSet>
#include <QtMath>
#include <algorithm>

namespace {
QString typePrefix(ElementType type)
{
    switch (type) {
    case ElementType::Rectangle: return QStringLiteral("rect");
    case ElementType::Ellipse: return QStringLiteral("ellipse");
    case ElementType::Line: return QStringLiteral("line");
    case ElementType::Polygon: return QStringLiteral("polygon");
    case ElementType::Star: return QStringLiteral("star");
    case ElementType::Path: return QStringLiteral("path");
    case ElementType::Text: return QStringLiteral("text");
    }
    return QStringLiteral("element");
}

QColor parseColorVariant(const QVariant& value, const QColor& fallback)
{
    if (value.canConvert<QColor>()) {
        const QColor c = value.value<QColor>();
        if (c.isValid()) return c;
    }
    const QColor c(value.toString());
    return c.isValid() ? c : fallback;
}

QJsonArray pointArray(const QPointF& p)
{
    return QJsonArray{roundJson(p.x()), roundJson(p.y())};
}
} // namespace

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------
QString elementTypeToString(ElementType type)
{
    switch (type) {
    case ElementType::Rectangle: return QStringLiteral("rect");
    case ElementType::Ellipse: return QStringLiteral("ellipse");
    case ElementType::Line: return QStringLiteral("line");
    case ElementType::Polygon: return QStringLiteral("polygon");
    case ElementType::Star: return QStringLiteral("star");
    case ElementType::Path: return QStringLiteral("path");
    case ElementType::Text: return QStringLiteral("text");
    }
    return QStringLiteral("rect");
}

ElementType elementTypeFromString(const QString& value)
{
    if (value == QLatin1String("ellipse")) return ElementType::Ellipse;
    if (value == QLatin1String("line")) return ElementType::Line;
    if (value == QLatin1String("polygon")) return ElementType::Polygon;
    if (value == QLatin1String("star")) return ElementType::Star;
    if (value == QLatin1String("path")) return ElementType::Path;
    if (value == QLatin1String("text")) return ElementType::Text;
    return ElementType::Rectangle;
}

QString elementTypeDisplayName(ElementType type)
{
    switch (type) {
    case ElementType::Rectangle: return QStringLiteral("矩形");
    case ElementType::Ellipse: return QStringLiteral("椭圆");
    case ElementType::Line: return QStringLiteral("直线");
    case ElementType::Polygon: return QStringLiteral("多边形");
    case ElementType::Star: return QStringLiteral("星形");
    case ElementType::Path: return QStringLiteral("自由路径");
    case ElementType::Text: return QStringLiteral("文本");
    }
    return QStringLiteral("元素");
}

// ---------------------------------------------------------------------------
// Track
// ---------------------------------------------------------------------------
void Track::sortKeys()
{
    std::sort(keys.begin(), keys.end(), [](const Keyframe& a, const Keyframe& b) {
        return a.time < b.time;
    });
}

QVariant Track::valueAt(double time, const QString& easing) const
{
    if (keys.isEmpty()) return {};
    if (keys.size() == 1) return keys.first().value;
    if (time <= keys.first().time) return keys.first().value;
    if (time >= keys.last().time) return keys.last().value;

    int index = 0;
    for (int i = 0; i + 1 < keys.size(); ++i) {
        if (time >= keys[i].time && time <= keys[i + 1].time) {
            index = i;
            break;
        }
    }
    const Keyframe& a = keys[index];
    const Keyframe& b = keys[index + 1];
    const double span = b.time - a.time;
    double t = span <= 0.000001 ? 1.0 : (time - a.time) / span;
    t = applyEasing(easing, t);

    if (property == QLatin1String("fill")) {
        const QColor ca = parseColorVariant(a.value, QColor("#5B8CFF"));
        const QColor cb = parseColorVariant(b.value, QColor("#5B8CFF"));
        return QColor::fromRgbF(ca.redF() + (cb.redF() - ca.redF()) * t,
                                ca.greenF() + (cb.greenF() - ca.greenF()) * t,
                                ca.blueF() + (cb.blueF() - ca.blueF()) * t,
                                ca.alphaF() + (cb.alphaF() - ca.alphaF()) * t);
    }
    const double na = a.value.toDouble();
    const double nb = b.value.toDouble();
    return na + (nb - na) * t;
}

bool Track::hasKeyAt(double time, double epsilon) const
{
    for (const auto& key : keys) {
        if (qAbs(key.time - time) <= epsilon) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------
Track* Animation::findTrack(const QString& elementId, const QString& property)
{
    for (auto& track : tracks) {
        if (track.elementId == elementId && track.property == property) return &track;
    }
    return nullptr;
}

const Track* Animation::findTrack(const QString& elementId, const QString& property) const
{
    for (const auto& track : tracks) {
        if (track.elementId == elementId && track.property == property) return &track;
    }
    return nullptr;
}

Track& Animation::ensureTrack(const QString& elementId, const QString& property)
{
    if (Track* track = findTrack(elementId, property)) return *track;
    Track track;
    track.elementId = elementId;
    track.property = property;
    tracks.append(track);
    return tracks.last();
}

void Animation::removeTracksFor(const QString& elementId)
{
    tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
                                [&](const Track& t) { return t.elementId == elementId; }),
                 tracks.end());
}

QJsonObject Animation::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["duration"] = roundJson(duration);
    obj["loop"] = loop;
    obj["easing"] = easing;
    QJsonArray trackArray;
    for (const auto& track : tracks) {
        if (track.keys.isEmpty()) continue;
        QJsonObject trackObj;
        trackObj["elementId"] = track.elementId;
        trackObj["property"] = track.property;
        QJsonArray keyArray;
        for (const auto& key : track.keys) {
            QJsonObject keyObj;
            keyObj["time"] = roundJson(key.time);
            if (track.property == QLatin1String("fill")) {
                keyObj["value"] = colorToString(parseColorVariant(key.value, QColor("#5B8CFF")));
            } else {
                keyObj["value"] = roundJson(key.value.toDouble());
            }
            keyArray.append(keyObj);
        }
        trackObj["keyframes"] = keyArray;
        trackArray.append(trackObj);
    }
    obj["tracks"] = trackArray;
    return obj;
}

Animation Animation::fromJson(const QJsonObject& obj)
{
    Animation anim;
    anim.id = obj.value("id").toString();
    anim.name = obj.value("name").toString(QStringLiteral("动画"));
    anim.duration = qMax(0.05, obj.value("duration").toDouble(1.0));
    anim.loop = obj.value("loop").toBool(false);
    anim.easing = obj.value("easing").toString(QStringLiteral("easeOutCubic"));
    for (const auto& value : obj.value("tracks").toArray()) {
        const QJsonObject trackObj = value.toObject();
        Track track;
        track.elementId = trackObj.value("elementId").toString();
        track.property = trackObj.value("property").toString();
        if (track.elementId.isEmpty() || track.property.isEmpty()) continue;
        for (const auto& keyValue : trackObj.value("keyframes").toArray()) {
            const QJsonObject keyObj = keyValue.toObject();
            Keyframe key;
            key.time = qBound(0.0, keyObj.value("time").toDouble(), anim.duration);
            if (track.property == QLatin1String("fill")) {
                key.value = colorFromJson(keyObj.value("value"), QColor("#5B8CFF"));
            } else {
                key.value = keyObj.value("value").toDouble();
            }
            track.keys.append(key);
        }
        if (!track.keys.isEmpty()) {
            track.sortKeys();
            anim.tracks.append(track);
        }
    }
    return anim;
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------
QJsonObject Interaction::toJson() const
{
    QJsonObject obj;
    obj["sourceId"] = sourceId;
    obj["trigger"] = trigger;
    obj["action"] = action;
    if (!animationId.isEmpty()) obj["animationId"] = animationId;
    return obj;
}

Interaction Interaction::fromJson(const QJsonObject& obj)
{
    Interaction interaction;
    interaction.sourceId = obj.value("sourceId").toString();
    interaction.trigger = obj.value("trigger").toString(QStringLiteral("click"));
    interaction.action = obj.value("action").toString(QStringLiteral("playAnimation"));
    interaction.animationId = obj.value("animationId").toString();
    return interaction;
}

QString Interaction::triggerDisplayName() const
{
    if (trigger == QLatin1String("hover")) return QStringLiteral("悬停");
    if (trigger == QLatin1String("leave")) return QStringLiteral("移出");
    if (trigger == QLatin1String("load")) return QStringLiteral("载入时");
    return QStringLiteral("点击");
}

// ---------------------------------------------------------------------------
// Guide
// ---------------------------------------------------------------------------
QJsonObject Guide::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["orientation"] = orientation;
    obj["position"] = roundJson(position);
    return obj;
}

Guide Guide::fromJson(const QJsonObject& obj)
{
    Guide guide;
    guide.id = obj.value("id").toString();
    guide.orientation = obj.value("orientation").toString(QStringLiteral("horizontal"));
    if (guide.orientation != QLatin1String("vertical") &&
        guide.orientation != QLatin1String("horizontal")) {
        guide.orientation = QStringLiteral("horizontal");
    }
    guide.position = obj.value("position").toDouble();
    return guide;
}

// ---------------------------------------------------------------------------
// Gradient
// ---------------------------------------------------------------------------
QJsonObject GradientStop::toJson() const
{
    QJsonObject obj;
    obj["position"] = roundJson(position);
    obj["color"] = colorToString(color);
    return obj;
}

GradientStop GradientStop::fromJson(const QJsonObject& obj)
{
    GradientStop stop;
    stop.position = qBound(0.0, obj.value("position").toDouble(), 1.0);
    stop.color = colorFromJson(obj.value("color"), QColor("#2D8CEB"));
    return stop;
}

bool GradientFill::isValid() const
{
    return stops.size() >= 2;
}

void GradientFill::sortStops()
{
    std::sort(stops.begin(), stops.end(), [](const GradientStop& a, const GradientStop& b) {
        return a.position < b.position;
    });
}

QColor GradientFill::colorAt(double position) const
{
    if (stops.isEmpty()) return QColor("#2D8CEB");
    if (!isValid()) return stops.first().color;
    position = qBound(0.0, position, 1.0);
    if (position <= stops.first().position) return stops.first().color;
    if (position >= stops.last().position) return stops.last().color;
    for (int i = 0; i + 1 < stops.size(); ++i) {
        if (position >= stops[i].position && position <= stops[i + 1].position) {
            const double span = stops[i + 1].position - stops[i].position;
            const double t = span <= 0.000001 ? 1.0 : (position - stops[i].position) / span;
            const QColor a = stops[i].color;
            const QColor b = stops[i + 1].color;
            return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                                    a.greenF() + (b.greenF() - a.greenF()) * t,
                                    a.blueF() + (b.blueF() - a.blueF()) * t,
                                    a.alphaF() + (b.alphaF() - a.alphaF()) * t);
        }
    }
    return stops.last().color;
}

QJsonObject GradientFill::toJson() const
{
    QJsonObject obj;
    obj["type"] = type == QLatin1String("radial") ? QStringLiteral("radial") : QStringLiteral("linear");
    obj["angle"] = roundJson(angle);
    QJsonArray stopArray;
    for (const auto& stop : stops) stopArray.append(stop.toJson());
    obj["stops"] = stopArray;
    return obj;
}

GradientFill GradientFill::fromJson(const QJsonObject& obj)
{
    GradientFill gradient;
    gradient.type = obj.value("type").toString(QStringLiteral("linear")) == QLatin1String("radial")
                        ? QStringLiteral("radial") : QStringLiteral("linear");
    gradient.angle = obj.value("angle").toDouble(135.0);
    for (const auto& value : obj.value("stops").toArray()) {
        gradient.stops.append(GradientStop::fromJson(value.toObject()));
    }
    gradient.sortStops();
    gradient.enabled = gradient.isValid();
    return gradient;
}

// ---------------------------------------------------------------------------
// Element
// ---------------------------------------------------------------------------
QRectF Element::boundingRect() const
{
    if (frame.width() < 0) {
        return QRectF(frame.x() + frame.width(), frame.y(), -frame.width(), frame.height());
    }
    if (frame.height() < 0) {
        return QRectF(frame.x(), frame.y() + frame.height(), frame.width(), -frame.height());
    }
    return frame;
}

QPointF Element::center() const
{
    return boundingRect().center();
}

QPointF Element::localToDocument(const QPointF& local) const
{
    const QPointF c = center();
    const double rad = qDegreesToRadians(rotation);
    const double dx = local.x() - c.x();
    const double dy = local.y() - c.y();
    return QPointF(c.x() + dx * qCos(rad) - dy * qSin(rad),
                   c.y() + dx * qSin(rad) + dy * qCos(rad));
}

QPolygonF Element::rotatedCorners() const
{
    const QRectF r = boundingRect();
    QPolygonF poly;
    poly << localToDocument(r.topLeft()) << localToDocument(r.topRight())
         << localToDocument(r.bottomRight()) << localToDocument(r.bottomLeft());
    return poly;
}

void Element::translateBy(const QPointF& delta)
{
    frame.translate(delta);
}

void Element::setFrameFromPoints(const QPointF& a, const QPointF& b)
{
    frame = QRectF(QPointF(qMin(a.x(), b.x()), qMin(a.y(), b.y())),
                   QPointF(qMax(a.x(), b.x()), qMax(a.y(), b.y())));
    if (frame.width() < 0.01) frame.setWidth(0.01);
    if (frame.height() < 0.01) frame.setHeight(0.01);
}

QJsonObject Element::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = elementTypeToString(type);
    obj["name"] = name;
    obj["x"] = roundJson(frame.x());
    obj["y"] = roundJson(frame.y());
    obj["width"] = roundJson(frame.width());
    obj["height"] = roundJson(frame.height());
    obj["rotation"] = roundJson(rotation);
    obj["opacity"] = roundJson(opacity);
    obj["fill"] = colorToString(fill);
    if (hasGradient()) obj["gradient"] = gradient.toJson();
    obj["stroke"] = colorToString(stroke);
    obj["strokeWidth"] = roundJson(strokeWidth);
    if (type == ElementType::Rectangle) obj["radius"] = roundJson(radius);
    if (type == ElementType::Polygon || type == ElementType::Star) obj["sides"] = sides;
    if (type == ElementType::Line) {
        obj["lineStart"] = pointArray(lineStart);
        obj["lineEnd"] = pointArray(lineEnd);
    }
    if (type == ElementType::Path) {
        obj["closed"] = closed;
        QJsonArray pts;
        for (const auto& p : points) pts.append(pointArray(p));
        obj["points"] = pts;
    }
    if (type == ElementType::Text) {
        obj["text"] = text;
        obj["fontFamily"] = fontFamily;
        obj["fontSize"] = roundJson(fontSize);
        obj["bold"] = bold;
        obj["align"] = align;
        obj["textColor"] = colorToString(textColor);
    }
    obj["visible"] = visible;
    obj["locked"] = locked;
    return obj;
}

Element Element::fromJson(const QJsonObject& obj)
{
    Element e;
    e.id = obj.value("id").toString();
    e.type = elementTypeFromString(obj.value("type").toString(QStringLiteral("rect")));
    e.name = obj.value("name").toString(elementTypeDisplayName(e.type));
    e.frame = QRectF(obj.value("x").toDouble(),
                     obj.value("y").toDouble(),
                     qMax(0.01, obj.value("width").toDouble(100)),
                     qMax(0.01, obj.value("height").toDouble(100)));
    e.rotation = obj.value("rotation").toDouble(0.0);
    e.opacity = qBound(0.0, obj.value("opacity").toDouble(1.0), 1.0);
    e.fill = colorFromJson(obj.value("fill"), QColor("#5B8CFF"));
    if (obj.contains("gradient") && obj.value("gradient").isObject()) {
        e.gradient = GradientFill::fromJson(obj.value("gradient").toObject());
    }
    e.stroke = colorFromJson(obj.value("stroke"), QColor("#101318"));
    e.strokeWidth = qMax(0.0, obj.value("strokeWidth").toDouble(0.0));
    e.radius = qMax(0.0, obj.value("radius").toDouble(0.0));
    e.sides = qBound(3, obj.value("sides").toInt(6), 60);
    e.visible = obj.value("visible").toBool(true);
    e.locked = obj.value("locked").toBool(false);
    e.closed = obj.value("closed").toBool(true);

    if (e.type == ElementType::Line) {
        const QJsonArray start = obj.value("lineStart").toArray();
        const QJsonArray end = obj.value("lineEnd").toArray();
        if (start.size() == 2) e.lineStart = QPointF(start.at(0).toDouble(), start.at(1).toDouble());
        if (end.size() == 2) e.lineEnd = QPointF(end.at(0).toDouble(), end.at(1).toDouble());
    }
    if (e.type == ElementType::Path) {
        for (const auto& value : obj.value("points").toArray()) {
            const QJsonArray p = value.toArray();
            if (p.size() == 2) e.points.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
        }
    }
    if (e.type == ElementType::Text) {
        e.text = obj.value("text").toString();
        e.fontFamily = obj.value("fontFamily").toString(QStringLiteral("Microsoft YaHei UI"));
        e.fontSize = qMax(1.0, obj.value("fontSize").toDouble(24.0));
        e.bold = obj.value("bold").toBool(false);
        e.align = obj.value("align").toString(QStringLiteral("center"));
        e.textColor = colorFromJson(obj.value("textColor"), QColor("#FFFFFF"));
    }
    return e;
}

// ---------------------------------------------------------------------------
// DesignDocument
// ---------------------------------------------------------------------------
DesignDocument::DesignDocument() = default;

void DesignDocument::clear()
{
    title = QStringLiteral("未命名设计");
    canvasSize = QSizeF(1280, 800);
    background = QColor("#0F1424");
    elements.clear();
    animations.clear();
    interactions.clear();
    guides.clear();
    m_nextElementId = 1;
    m_nextAnimationId = 1;
    m_nextGuideId = 1;
}

QJsonObject DesignDocument::toJson() const
{
    QJsonObject root;
    root["format"] = QStringLiteral("DeepDesign");
    root["version"] = 1;
    root["generator"] = QStringLiteral("DeepDesign Studio 1.0");
    root["title"] = title;

    QJsonObject canvas;
    canvas["width"] = roundJson(canvasSize.width());
    canvas["height"] = roundJson(canvasSize.height());
    canvas["background"] = colorToString(background);
    root["canvas"] = canvas;

    QJsonArray elementArray;
    for (const auto& element : elements) elementArray.append(element.toJson());
    root["elements"] = elementArray;

    QJsonArray animationArray;
    for (const auto& animation : animations) animationArray.append(animation.toJson());
    root["animations"] = animationArray;

    QJsonArray interactionArray;
    for (const auto& interaction : interactions) interactionArray.append(interaction.toJson());
    root["interactions"] = interactionArray;

    QJsonArray guideArray;
    for (const auto& guide : guides) guideArray.append(guide.toJson());
    root["guides"] = guideArray;
    return root;
}

bool DesignDocument::fromJson(const QJsonObject& obj, QString* error)
{
    const QString format = obj.value("format").toString();
    if (!format.isEmpty() && format != QLatin1String("DeepDesign")) {
        if (error) *error = QStringLiteral("不是 DeepDesign 设计文件（format = %1）").arg(format);
        return false;
    }
    const int version = obj.value("version").toInt(1);
    if (version > 1) {
        if (error) *error = QStringLiteral("文件版本 %1 高于当前软件支持的版本 1").arg(version);
        return false;
    }

    DesignDocument loaded;
    loaded.title = obj.value("title").toString(QStringLiteral("未命名设计"));
    const QJsonObject canvas = obj.value("canvas").toObject();
    loaded.canvasSize = QSizeF(qMax(100.0, canvas.value("width").toDouble(1280)),
                               qMax(100.0, canvas.value("height").toDouble(800)));
    loaded.background = colorFromJson(canvas.value("background"), QColor("#0F1424"));

    QSet<QString> usedIds;
    for (const auto& value : obj.value("elements").toArray()) {
        Element element = Element::fromJson(value.toObject());
        if (element.id.isEmpty()) element.id = loaded.makeId(typePrefix(element.type));
        while (usedIds.contains(element.id)) element.id = loaded.makeId(typePrefix(element.type));
        usedIds.insert(element.id);
        if (element.name.isEmpty()) element.name = elementTypeDisplayName(element.type);
        loaded.elements.append(element);
    }

    QSet<QString> animationIds;
    for (const auto& value : obj.value("animations").toArray()) {
        Animation animation = Animation::fromJson(value.toObject());
        if (animation.id.isEmpty()) animation.id = loaded.makeAnimationId();
        while (animationIds.contains(animation.id)) animation.id = loaded.makeAnimationId();
        animationIds.insert(animation.id);
        loaded.animations.append(animation);
    }

    for (const auto& value : obj.value("interactions").toArray()) {
        Interaction interaction = Interaction::fromJson(value.toObject());
        if (interaction.sourceId.isEmpty()) continue;
        loaded.interactions.append(interaction);
    }

    QSet<QString> guideIds;
    for (const auto& value : obj.value("guides").toArray()) {
        Guide guide = Guide::fromJson(value.toObject());
        if (guide.id.isEmpty()) guide.id = loaded.makeGuideId();
        while (guideIds.contains(guide.id)) guide.id = loaded.makeGuideId();
        guideIds.insert(guide.id);
        loaded.guides.append(guide);
    }

    *this = loaded;
    if (error) error->clear();
    return true;
}

Element* DesignDocument::findElement(const QString& id)
{
    for (auto& element : elements) {
        if (element.id == id) return &element;
    }
    return nullptr;
}

const Element* DesignDocument::findElement(const QString& id) const
{
    for (const auto& element : elements) {
        if (element.id == id) return &element;
    }
    return nullptr;
}

int DesignDocument::indexOf(const QString& id) const
{
    for (int i = 0; i < elements.size(); ++i) {
        if (elements[i].id == id) return i;
    }
    return -1;
}

QString DesignDocument::makeId(const QString& prefix)
{
    QString id;
    do {
        id = QStringLiteral("%1_%2").arg(prefix).arg(m_nextElementId++);
    } while (findElement(id) != nullptr);
    return id;
}

QString DesignDocument::uniqueName(const QString& base) const
{
    int suffix = 1;
    QString candidate = QStringLiteral("%1 %2").arg(base).arg(suffix);
    auto taken = [&](const QString& name) {
        for (const auto& e : elements) {
            if (e.name == name) return true;
        }
        return false;
    };
    while (taken(candidate)) candidate = QStringLiteral("%1 %2").arg(base).arg(++suffix);
    return candidate;
}

Element& DesignDocument::addElement(const Element& element)
{
    elements.append(element);
    return elements.last();
}

void DesignDocument::removeElement(const QString& id)
{
    removeElements(QStringList{id});
}

void DesignDocument::removeElements(const QStringList& ids)
{
    const QSet<QString> idSet(ids.begin(), ids.end());
    elements.erase(std::remove_if(elements.begin(), elements.end(),
                                  [&](const Element& e) { return idSet.contains(e.id); }),
                   elements.end());
    interactions.erase(std::remove_if(interactions.begin(), interactions.end(),
                                      [&](const Interaction& i) { return idSet.contains(i.sourceId); }),
                       interactions.end());
    for (auto& animation : animations) {
        for (const auto& id : ids) animation.removeTracksFor(id);
    }
}

QString DesignDocument::duplicateElement(const QString& id)
{
    const Element* source = findElement(id);
    if (!source) return {};
    Element copy = *source;
    copy.id = makeId(typePrefix(copy.type));
    copy.name = uniqueName(copy.name + QStringLiteral(" 副本"));
    copy.translateBy(QPointF(18, 18));
    elements.append(copy);
    for (const auto& interaction : interactionsFor(id)) {
        Interaction dup = interaction;
        dup.sourceId = copy.id;
        interactions.append(dup);
    }
    return copy.id;
}

Animation* DesignDocument::findAnimation(const QString& id)
{
    for (auto& animation : animations) {
        if (animation.id == id) return &animation;
    }
    return nullptr;
}

const Animation* DesignDocument::findAnimation(const QString& id) const
{
    for (const auto& animation : animations) {
        if (animation.id == id) return &animation;
    }
    return nullptr;
}

QString DesignDocument::makeAnimationId()
{
    QString id;
    do {
        id = QStringLiteral("anim_%1").arg(m_nextAnimationId++);
    } while (findAnimation(id) != nullptr);
    return id;
}

Animation& DesignDocument::addAnimation(const QString& name)
{
    Animation animation;
    animation.id = makeAnimationId();
    animation.name = name.isEmpty() ? QStringLiteral("动画 %1").arg(animations.size() + 1) : name;
    animations.append(animation);
    return animations.last();
}

void DesignDocument::removeAnimation(const QString& id)
{
    animations.erase(std::remove_if(animations.begin(), animations.end(),
                                    [&](const Animation& a) { return a.id == id; }),
                     animations.end());
    interactions.erase(std::remove_if(interactions.begin(), interactions.end(),
                                      [&](const Interaction& i) { return i.animationId == id; }),
                       interactions.end());
}

QString DesignDocument::makeGuideId()
{
    QString id;
    do {
        id = QStringLiteral("guide_%1").arg(m_nextGuideId++);
    } while (std::any_of(guides.begin(), guides.end(), [&](const Guide& g) { return g.id == id; }));
    return id;
}

int DesignDocument::addGuide(const QString& orientation, double position)
{
    Guide guide;
    guide.id = makeGuideId();
    guide.orientation = orientation == QLatin1String("vertical") ? QStringLiteral("vertical")
                                                                : QStringLiteral("horizontal");
    guide.position = position;
    guides.append(guide);
    return guides.size() - 1;
}

void DesignDocument::removeGuide(int index)
{
    if (index < 0 || index >= guides.size()) return;
    guides.removeAt(index);
}

void DesignDocument::clearGuides()
{
    guides.clear();
}

QVector<Interaction> DesignDocument::interactionsFor(const QString& sourceId) const
{
    QVector<Interaction> result;
    for (const auto& interaction : interactions) {
        if (interaction.sourceId == sourceId) result.append(interaction);
    }
    return result;
}

void DesignDocument::removeInteractionsFor(const QString& sourceId)
{
    interactions.erase(std::remove_if(interactions.begin(), interactions.end(),
                                      [&](const Interaction& i) { return i.sourceId == sourceId; }),
                       interactions.end());
}

void DesignDocument::bringForward(const QStringList& ids)
{
    const QSet<QString> set(ids.begin(), ids.end());
    for (int i = elements.size() - 2; i >= 0; --i) {
        if (set.contains(elements[i].id) && !set.contains(elements[i + 1].id)) {
            elements.swapItemsAt(i, i + 1);
        }
    }
}

void DesignDocument::sendBackward(const QStringList& ids)
{
    const QSet<QString> set(ids.begin(), ids.end());
    for (int i = 1; i < elements.size(); ++i) {
        if (set.contains(elements[i].id) && !set.contains(elements[i - 1].id)) {
            elements.swapItemsAt(i, i - 1);
        }
    }
}

void DesignDocument::bringToFront(const QStringList& ids)
{
    const QSet<QString> set(ids.begin(), ids.end());
    QVector<Element> moving;
    QVector<Element> rest;
    for (const auto& element : elements) {
        (set.contains(element.id) ? moving : rest).append(element);
    }
    elements = rest + moving;
}

void DesignDocument::sendToBack(const QStringList& ids)
{
    const QSet<QString> set(ids.begin(), ids.end());
    QVector<Element> moving;
    QVector<Element> rest;
    for (const auto& element : elements) {
        (set.contains(element.id) ? moving : rest).append(element);
    }
    elements = moving + rest;
}

QHash<QString, QVariantMap> DesignDocument::sampleAnimation(const QString& animationId, double time,
                                                             bool* found) const
{
    QHash<QString, QVariantMap> result;
    const Animation* animation = findAnimation(animationId);
    if (found) *found = animation != nullptr;
    if (!animation) return result;
    for (const auto& track : animation->tracks) {
        if (track.keys.isEmpty()) continue;
        if (!findElement(track.elementId)) continue;
        result[track.elementId][track.property] = track.valueAt(time, animation->easing);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
QString colorToString(const QColor& color)
{
    if (!color.isValid()) return QStringLiteral("#000000");
    if (color.alpha() >= 255) return color.name(QColor::HexRgb).toUpper();
    return QStringLiteral("#%1%2")
        .arg(color.name(QColor::HexRgb).mid(1).toUpper())
        .arg(color.alpha(), 2, 16, QLatin1Char('0'))
        .toUpper();
}

QColor colorFromJson(const QJsonValue& value, const QColor& fallback)
{
    if (value.isString()) {
        const QString text = value.toString().trimmed();
        if (text.startsWith(QLatin1Char('#')) && text.size() == 9) {
            bool ok = false;
            const QColor rgb(QStringLiteral("#") + text.mid(1, 6));
            const int alpha = text.mid(7, 2).toInt(&ok, 16);
            if (rgb.isValid() && ok) return QColor(rgb.red(), rgb.green(), rgb.blue(), alpha);
        }
        const QColor color(text);
        if (color.isValid()) return color;
    }
    return fallback;
}

QJsonValue roundJson(double value)
{
    if (!qIsFinite(value)) return 0;
    return qRound(value * 1000.0) / 1000.0;
}

double clampValue(double value, double minimum, double maximum)
{
    return qBound(minimum, value, maximum);
}

QString formatNumber(double value, int decimals)
{
    return QString::number(value, 'f', decimals);
}

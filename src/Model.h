#pragma once

#include <QColor>
#include <QJsonObject>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QVariant>

// ---------------------------------------------------------------------------
// Element types. The string values are part of the .ddesign file format.
// ---------------------------------------------------------------------------
enum class ElementType {
    Rectangle,
    Ellipse,
    Line,
    Polygon,
    Star,
    Path,
    Text
};

QString elementTypeToString(ElementType type);
ElementType elementTypeFromString(const QString& value);
QString elementTypeDisplayName(ElementType type);

struct Keyframe {
    double time = 0.0;        // seconds from the start of the animation
    QVariant value;           // number or color string, depending on the track
};

struct Track {
    QString elementId;
    QString property;         // x, y, width, height, rotation, opacity, fill, strokeWidth
    QVector<Keyframe> keys;

    void sortKeys();
    QVariant valueAt(double time, const QString& easing) const;
    bool hasKeyAt(double time, double epsilon = 0.001) const;
};

struct Animation {
    QString id;
    QString name = QStringLiteral("新动画");
    double duration = 1.0;    // seconds
    bool loop = false;
    QString easing = QStringLiteral("easeOutCubic");
    QVector<Track> tracks;

    Track* findTrack(const QString& elementId, const QString& property);
    const Track* findTrack(const QString& elementId, const QString& property) const;
    Track& ensureTrack(const QString& elementId, const QString& property);
    void removeTracksFor(const QString& elementId);
    QJsonObject toJson() const;
    static Animation fromJson(const QJsonObject& obj);
};

struct Interaction {
    QString sourceId;
    QString trigger = QStringLiteral("click");     // click, hover, leave, load
    QString action = QStringLiteral("playAnimation"); // playAnimation, stopAll
    QString animationId;

    QJsonObject toJson() const;
    static Interaction fromJson(const QJsonObject& obj);
    QString triggerDisplayName() const;
};

struct Guide {
    QString id;
    QString orientation = QStringLiteral("horizontal"); // horizontal: y = position, vertical: x = position
    double position = 0.0;

    QJsonObject toJson() const;
    static Guide fromJson(const QJsonObject& obj);
};

struct GradientStop {
    double position = 0.0;   // 0..1
    QColor color = QColor("#2D8CEB");

    QJsonObject toJson() const;
    static GradientStop fromJson(const QJsonObject& obj);
};

struct GradientFill {
    bool enabled = false;
    QString type = QStringLiteral("linear"); // linear, radial
    double angle = 135.0;                    // degrees, 0 = left→right, 90 = top→bottom
    QVector<GradientStop> stops;

    bool isValid() const;
    void sortStops();
    QColor colorAt(double position) const;
    QJsonObject toJson() const;
    static GradientFill fromJson(const QJsonObject& obj);
};

struct Element {
    QString id;
    QString name;
    ElementType type = ElementType::Rectangle;

    // Axis-aligned, un-rotated local frame. Rotation is applied around its center.
    QRectF frame = QRectF(0, 0, 100, 100);
    double rotation = 0.0;    // degrees
    double opacity = 1.0;     // 0..1

    QColor fill = QColor("#5B8CFF");
    GradientFill gradient;
    QColor stroke = QColor("#101318");
    double strokeWidth = 0.0;
    double radius = 0.0;
    int sides = 6;

    bool visible = true;
    bool locked = false;
    bool closed = true;

    // Normalised points (0..1 relative to frame) for Path elements.
    QVector<QPointF> points;
    QPointF lineStart = QPointF(0, 0);
    QPointF lineEnd = QPointF(1, 1);

    // Text properties.
    QString text;
    QString fontFamily = QStringLiteral("Microsoft YaHei UI");
    double fontSize = 24.0;
    bool bold = false;
    QString align = QStringLiteral("center"); // left, center, right
    QColor textColor = QColor("#FFFFFF");

    QRectF boundingRect() const;
    QPointF center() const;
    QPolygonF rotatedCorners() const;
    QPointF localToDocument(const QPointF& local) const;
    void translateBy(const QPointF& delta);
    void setFrameFromPoints(const QPointF& a, const QPointF& b);
    bool hasGradient() const { return gradient.enabled && gradient.isValid(); }

    QJsonObject toJson() const;
    static Element fromJson(const QJsonObject& obj);
};

class DesignDocument {
public:
    QString title = QStringLiteral("未命名设计");
    QSizeF canvasSize = QSizeF(1280, 800);
    QColor background = QColor("#0F1424");

    QVector<Element> elements;      // back to front
    QVector<Animation> animations;
    QVector<Interaction> interactions;
    QVector<Guide> guides;

    DesignDocument();

    void clear();
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& obj, QString* error = nullptr);

    Element* findElement(const QString& id);
    const Element* findElement(const QString& id) const;
    int indexOf(const QString& id) const;
    QString makeId(const QString& prefix);
    QString uniqueName(const QString& base) const;

    Element& addElement(const Element& element);
    void removeElement(const QString& id);
    void removeElements(const QStringList& ids);
    QString duplicateElement(const QString& id);

    Animation* findAnimation(const QString& id);
    const Animation* findAnimation(const QString& id) const;
    QString makeAnimationId();
    Animation& addAnimation(const QString& name = QString());
    void removeAnimation(const QString& id);
    QString makeGuideId();
    int addGuide(const QString& orientation, double position);
    void removeGuide(int index);
    void clearGuides();

    QVector<Interaction> interactionsFor(const QString& sourceId) const;
    void removeInteractionsFor(const QString& sourceId);

    void bringForward(const QStringList& ids);
    void sendBackward(const QStringList& ids);
    void bringToFront(const QStringList& ids);
    void sendToBack(const QStringList& ids);

    QHash<QString, QVariantMap> sampleAnimation(const QString& animationId, double time,
                                                 bool* found = nullptr) const;

private:
    int m_nextElementId = 1;
    int m_nextAnimationId = 1;
    int m_nextGuideId = 1;
};

// Serialisation helpers shared by the model, exporter and tests.
QString colorToString(const QColor& color);
QColor colorFromJson(const QJsonValue& value, const QColor& fallback);
QJsonValue roundJson(double value);
double clampValue(double value, double minimum, double maximum);
QString formatNumber(double value, int decimals = 2);

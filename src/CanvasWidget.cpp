#include "CanvasWidget.h"
#include "Animation.h"
#include "Icons.h"
#include "Renderer.h"
#include "Theme.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QRadialGradient>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextOption>
#include <QTimer>
#include <QWheelEvent>
#include <QtMath>

namespace {
constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 8.0;
constexpr double kHandleScreenSize = 8.0;

QPointF rotatePoint(const QPointF& p, const QPointF& center, double degrees)
{
    const double rad = qDegreesToRadians(degrees);
    const double dx = p.x() - center.x();
    const double dy = p.y() - center.y();
    return QPointF(center.x() + dx * qCos(rad) - dy * qSin(rad),
                   center.y() + dx * qSin(rad) + dy * qCos(rad));
}

double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab = b - a;
    const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
    if (len2 < 0.000001) return QLineF(p, a).length();
    double t = ((p.x() - a.x()) * ab.x() + (p.y() - a.y()) * ab.y()) / len2;
    t = qBound(0.0, t, 1.0);
    return QLineF(p, a + ab * t).length();
}

double niceRulerStep(double zoom)
{
    static const double steps[] = {1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500,
                                   1000, 2000, 2500, 5000, 10000};
    for (double step : steps) {
        if (step * zoom >= 64.0) return step;
    }
    return 20000.0;
}
} // namespace

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMinimumSize(360, 260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::ArrowCursor);
    emit statusHint(QStringLiteral("从左侧选择工具，在画布上开始设计"));
}

// ---------------------------------------------------------------------------
// Document and selection
// ---------------------------------------------------------------------------
void CanvasWidget::setDocument(DesignDocument* document)
{
    if (m_textEditor) commitTextEditing();
    m_document = document;
    m_selection.clear();
    m_previewAnimationId.clear();
    m_penPoints.clear();
    m_hoverId.clear();
    m_dragMode = DragMode::None;
    m_activeGuide = -1;
    m_badgeVisible = false;
    m_pendingFit = true;
    emit selectionChanged(m_selection);
    emit selectionInfoChanged();
    if (width() > 10 && height() > 10) {
        fitToView();
    }
    update();
}

Element* CanvasWidget::primaryElement() const
{
    if (!m_document || m_selection.isEmpty()) return nullptr;
    return m_document->findElement(m_selection.first());
}

void CanvasWidget::setSelection(const QStringList& ids, bool notify)
{
    if (m_textEditor && !ids.contains(m_textEditId)) commitTextEditing();

    QStringList filtered;
    if (m_document) {
        for (const auto& id : ids) {
            if (!filtered.contains(id) && m_document->findElement(id)) filtered.append(id);
        }
    }
    if (filtered == m_selection) {
        if (notify) emit selectionInfoChanged();
        return;
    }
    m_selection = filtered;
    update();
    if (notify) {
        emit selectionChanged(m_selection);
        emit selectionInfoChanged();
    }
}

void CanvasWidget::clearSelection()
{
    setSelection({});
}

void CanvasWidget::selectAll()
{
    if (!m_document) return;
    QStringList ids;
    for (const auto& element : m_document->elements) {
        if (element.visible) ids.append(element.id);
    }
    setSelection(ids);
}

void CanvasWidget::pruneSelection()
{
    if (!m_document) {
        clearSelection();
        return;
    }
    QStringList filtered;
    for (const auto& id : m_selection) {
        if (m_document->findElement(id)) filtered.append(id);
    }
    if (filtered != m_selection) setSelection(filtered);
}

void CanvasWidget::setTool(Tool tool)
{
    if (m_tool == Tool::Pen && tool != Tool::Pen) cancelPen();
    if (m_tool == tool) {
        updateCursor();
        return;
    }
    m_tool = tool;
    if (tool != Tool::Select) {
        m_badgeVisible = false;
    }
    if (tool == Tool::Pen) {
        m_penPoints.clear();
        m_penHover = QPointF();
    }
    m_badgeVisible = false;
    updateCursor();
    update();
    QString hint;
    switch (tool) {
    case Tool::Select: hint = QStringLiteral("选择工具：点击选中，拖拽移动，Shift 多选"); break;
    case Tool::Rectangle: hint = QStringLiteral("矩形工具：在画板上拖拽绘制"); break;
    case Tool::Ellipse: hint = QStringLiteral("椭圆工具：在画板上拖拽绘制"); break;
    case Tool::Line: hint = QStringLiteral("直线工具：拖拽绘制，Shift 锁定 45°"); break;
    case Tool::Polygon: hint = QStringLiteral("多边形工具：拖拽绘制"); break;
    case Tool::Star: hint = QStringLiteral("星形工具：拖拽绘制"); break;
    case Tool::Pen: hint = QStringLiteral("钢笔工具：点击添加节点，双击或 Enter 完成，Esc 取消"); break;
    case Tool::Text: hint = QStringLiteral("文本工具：点击画板创建文本"); break;
    case Tool::Hand: hint = QStringLiteral("抓手工具：拖拽平移画布，空格键可临时启用"); break;
    case Tool::Zoom: hint = QStringLiteral("缩放工具：点击放大，Alt+点击缩小"); break;
    }
    emit statusHint(hint);
}

// ---------------------------------------------------------------------------
// Coordinates and geometry
// ---------------------------------------------------------------------------
QPointF CanvasWidget::docToScreen(const QPointF& point) const
{
    return m_origin + point * m_zoom;
}

QPointF CanvasWidget::screenToDoc(const QPointF& point) const
{
    if (m_zoom <= 0.000001) return point;
    return (point - m_origin) / m_zoom;
}

QRectF CanvasWidget::docToScreen(const QRectF& rect) const
{
    return QRectF(docToScreen(rect.topLeft()),
                  QSizeF(rect.width() * m_zoom, rect.height() * m_zoom));
}

QPolygonF CanvasWidget::docToScreen(const QPolygonF& polygon) const
{
    QPolygonF result;
    result.reserve(polygon.size());
    for (const auto& point : polygon) result.append(docToScreen(point));
    return result;
}

QPointF CanvasWidget::contentTopLeft() const
{
    return QPointF(m_showRulers ? m_rulerSize : 0, m_showRulers ? m_rulerSize : 0);
}

QSizeF CanvasWidget::contentSize() const
{
    const double left = m_showRulers ? m_rulerSize : 0;
    return QSizeF(qMax(1.0, width() - left), qMax(1.0, height() - left));
}

QPointF CanvasWidget::localPointForHandle(const Element& element, int index) const
{
    const QRectF r = element.boundingRect();
    const QPointF c = r.center();
    switch (index) {
    case 0: return r.topLeft();
    case 1: return QPointF(c.x(), r.top());
    case 2: return r.topRight();
    case 3: return QPointF(r.right(), c.y());
    case 4: return r.bottomRight();
    case 5: return QPointF(c.x(), r.bottom());
    case 6: return r.bottomLeft();
    case 7: return QPointF(r.left(), c.y());
    default: return c;
    }
}

QPointF CanvasWidget::handlePoint(const Element& element, int index) const
{
    return docToScreen(element.localToDocument(localPointForHandle(element, index)));
}

QPointF CanvasWidget::rotationHandlePoint(const Element& element) const
{
    const QRectF r = element.boundingRect();
    const QPointF local(r.center().x(), r.top() - 26.0 / qMax(0.001, m_zoom));
    return docToScreen(element.localToDocument(local));
}

int CanvasWidget::hitHandle(const QPointF& screenPoint) const
{
    if (m_selection.size() != 1 || m_dragMode == DragMode::Create) return -1;
    const Element* element = primaryElement();
    if (!element || !element->visible || element->locked) return -1;
    for (int i = 0; i < 8; ++i) {
        if (QLineF(screenPoint, handlePoint(*element, i)).length() <= kHandleScreenSize) return i;
    }
    return -1;
}

bool CanvasWidget::hitRotationHandle(const QPointF& screenPoint) const
{
    if (m_selection.size() != 1) return false;
    const Element* element = primaryElement();
    if (!element || !element->visible || element->locked) return false;
    return QLineF(screenPoint, rotationHandlePoint(*element)).length() <= 9.0;
}

bool CanvasWidget::elementHit(const Element& element, const QPointF& documentPoint) const
{
    const QRectF r = element.boundingRect();
    const QPointF center = r.center();
    const QPointF local = rotatePoint(documentPoint, center, -element.rotation);

    if (element.type == ElementType::Line) {
        const QPointF a(r.x() + element.lineStart.x() * r.width(),
                        r.y() + element.lineStart.y() * r.height());
        const QPointF b(r.x() + element.lineEnd.x() * r.width(),
                        r.y() + element.lineEnd.y() * r.height());
        return distanceToSegment(local, a, b) <= qMax(6.0 / m_zoom, element.strokeWidth / 2.0 + 2.0);
    }
    if (element.type == ElementType::Text) {
        return r.adjusted(-2, -2, 2, 2).contains(local);
    }
    const QPainterPath path = Renderer::shapePath(element);
    if (path.contains(local)) return true;
    QPainterPathStroker stroker;
    stroker.setWidth(qMax(element.strokeWidth, 8.0 / m_zoom));
    return stroker.createStroke(path).contains(local);
}

Element* CanvasWidget::topElementAt(const QPointF& screenPoint, bool includeLocked) const
{
    if (!m_document) return nullptr;
    const QPointF documentPoint = screenToDoc(screenPoint);
    for (int i = m_document->elements.size() - 1; i >= 0; --i) {
        Element& element = m_document->elements[i];
        if (!element.visible) continue;
        if (element.locked && !includeLocked) continue;
        if (elementHit(element, documentPoint)) return &element;
    }
    return nullptr;
}

QRectF CanvasWidget::selectionBounds() const
{
    QRectF result;
    if (!m_document) return result;
    for (const auto& id : m_selection) {
        const Element* element = m_document->findElement(id);
        if (!element) continue;
        const QRectF box = element->rotatedCorners().boundingRect();
        result = result.isNull() ? box : result.united(box);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------
void CanvasWidget::setZoom(double zoom)
{
    applyZoom(zoom, rect().center());
}

void CanvasWidget::applyZoom(double zoom, const QPointF& anchorScreen)
{
    const double target = qBound(kMinZoom, zoom, kMaxZoom);
    if (qAbs(target - m_zoom) < 0.00001) return;
    const QPointF anchorDoc = screenToDoc(anchorScreen);
    m_zoom = target;
    m_origin = anchorScreen - anchorDoc * m_zoom;
    updateEditorGeometry();
    update();
    emit zoomChanged(m_zoom);
}

void CanvasWidget::zoomIn()
{
    applyZoom(m_zoom * 1.2, rect().center());
}

void CanvasWidget::zoomOut()
{
    applyZoom(m_zoom / 1.2, rect().center());
}

void CanvasWidget::fitToView()
{
    if (!m_document) return;
    const double left = m_showRulers ? m_rulerSize : 0;
    const double availableW = qMax(80.0, width() - left - 64.0);
    const double availableH = qMax(80.0, height() - left - 64.0);
    const double z = qMin(availableW / m_document->canvasSize.width(),
                          availableH / m_document->canvasSize.height());
    m_zoom = qBound(0.02, z, 1.5);
    const QPointF contentCenter(left + (width() - left) / 2.0, left + (height() - left) / 2.0);
    m_origin = contentCenter - QPointF(m_document->canvasSize.width() * m_zoom / 2.0,
                                       m_document->canvasSize.height() * m_zoom / 2.0);
    m_pendingFit = false;
    updateEditorGeometry();
    update();
    emit zoomChanged(m_zoom);
}

void CanvasWidget::actualSize()
{
    applyZoom(1.0, rect().center());
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------
void CanvasWidget::setGridVisible(bool visible)
{
    if (m_showGrid == visible) return;
    m_showGrid = visible;
    update();
}

void CanvasWidget::setSnapEnabled(bool enabled)
{
    m_snapEnabled = enabled;
}

void CanvasWidget::setRulersVisible(bool visible)
{
    if (m_showRulers == visible) return;
    m_showRulers = visible;
    m_rulerSize = visible ? 22 : 0;
    update();
}

void CanvasWidget::setGuidesVisible(bool visible)
{
    if (m_showGuides == visible) return;
    m_showGuides = visible;
    update();
}

void CanvasWidget::setDefaultFillColor(const QColor& color)
{
    if (!color.isValid()) return;
    m_defaultFillColor = color;
    m_defaultFill = color;
    if (m_defaultStroke == QColor("#5AA9F0") || m_defaultStroke == m_defaultFill.lighter(135)) {
        m_defaultStroke = color.lighter(135);
    }
}

void CanvasWidget::setDefaultShapeStyle(const QColor& fill, const QColor& stroke,
                                        double strokeWidth, double radius, int sides)
{
    if (fill.isValid()) {
        m_defaultFillColor = fill;
        m_defaultFill = fill;
    }
    if (stroke.isValid()) m_defaultStroke = stroke;
    m_defaultStrokeWidth = qMax(0.0, strokeWidth);
    m_defaultRadius = qMax(0.0, radius);
    m_defaultSides = qBound(3, sides, 60);
}

void CanvasWidget::setPreviewAnimation(const QString& animationId, double time)
{
    m_previewAnimationId = animationId;
    m_previewTime = time;
    update();
}

void CanvasWidget::clearPreviewAnimation()
{
    if (m_previewAnimationId.isEmpty()) return;
    m_previewAnimationId.clear();
    update();
}

void CanvasWidget::flushEditing()
{
    if (m_textEditor) commitTextEditing(false);
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------
void CanvasWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    paintWorkspace(painter);
    if (!m_document) return;

    paintArtboard(painter);
    paintEmptyState(painter);
    if (m_previewAnimationId.isEmpty()) paintGuides(painter);

    if (m_badgeVisible && !m_sizeBadge.isEmpty()) {
        paintBadge(painter, m_badgeScreenPoint, m_sizeBadge);
    }
    if (m_previewAnimationId.isEmpty()) {
        paintSelection(painter);
        paintInteractionBadge(painter);
    }
    paintPenPreview(painter);

    if (!m_marqueeRect.isNull()) {
        const QRectF r = docToScreen(m_marqueeRect).normalized();
        painter.setPen(QPen(QColor(108, 140, 255, 220), 1.0, Qt::DashLine));
        painter.setBrush(QColor(108, 140, 255, 28));
        painter.drawRect(r);
        painter.setBrush(Qt::NoBrush);
    }

    paintRulers(painter);
}

void CanvasWidget::paintWorkspace(QPainter& painter)
{
    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0.0, QColor("#0B0E15"));
    gradient.setColorAt(0.55, QColor("#080A0F"));
    gradient.setColorAt(1.0, QColor("#0A0C12"));
    painter.fillRect(rect(), gradient);

    painter.setPen(Qt::NoPen);
    QRadialGradient glow1(QPointF(width() * 0.18, height() * 0.08), width() * 0.6);
    glow1.setColorAt(0.0, QColor(108, 140, 255, 24));
    glow1.setColorAt(1.0, QColor(108, 140, 255, 0));
    painter.setBrush(glow1);
    painter.drawEllipse(QPointF(width() * 0.18, height() * 0.08), width() * 0.6, width() * 0.6);

    QRadialGradient glow2(QPointF(width() * 0.88, height() * 0.9), width() * 0.45);
    glow2.setColorAt(0.0, QColor(164, 123, 255, 18));
    glow2.setColorAt(1.0, QColor(164, 123, 255, 0));
    painter.setBrush(glow2);
    painter.drawEllipse(QPointF(width() * 0.88, height() * 0.9), width() * 0.45, width() * 0.45);

    const double spacing = 32.0;
    QVector<QPointF> dots;
    const double offsetX = std::fmod(m_origin.x(), spacing);
    const double offsetY = std::fmod(m_origin.y(), spacing);
    for (double x = offsetX; x < width(); x += spacing) {
        for (double y = offsetY; y < height(); y += spacing) {
            dots.append(QPointF(x, y));
        }
    }
    painter.setPen(QPen(QColor(255, 255, 255, 12), 1.2));
    if (!dots.isEmpty()) painter.drawPoints(dots.constData(), dots.size());
}

void CanvasWidget::paintArtboard(QPainter& painter)
{
    const QRectF artboard = docToScreen(QRectF(QPointF(0, 0), m_document->canvasSize));
    const double radius = qBound(2.0, 8.0 * m_zoom, 12.0);

    painter.save();
    painter.setPen(Qt::NoPen);
    for (int i = 4; i >= 1; --i) {
        painter.setBrush(QColor(0, 0, 0, 16 + i * 5));
        painter.drawRoundedRect(artboard.translated(0, 10 + i * 2)
                                    .adjusted(-i * 4, -i * 4, i * 4, i * 4),
                                radius + i * 2, radius + i * 2);
    }
    painter.restore();

    painter.save();
    painter.setClipRect(artboard);
    painter.translate(m_origin);
    painter.scale(m_zoom, m_zoom);
    QHash<QString, QVariantMap> overrides;
    if (!m_previewAnimationId.isEmpty()) {
        overrides = m_document->sampleAnimation(m_previewAnimationId, m_previewTime);
    }
    Renderer::paintScene(painter, *m_document, overrides, m_showGrid, m_zoom);
    painter.restore();

    painter.setPen(QPen(QColor(54, 64, 86, 235), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(artboard.adjusted(-0.5, -0.5, 0.5, 0.5));
}

void CanvasWidget::paintEmptyState(QPainter& painter)
{
    if (!m_document || !m_document->elements.isEmpty() || !m_penPoints.isEmpty()) return;
    const QRectF artboard = docToScreen(QRectF(QPointF(0, 0), m_document->canvasSize));
    const QRectF visible = artboard.intersected(rect());
    if (visible.width() < 240 || visible.height() < 160) return;

    const QPointF center = artboard.center();
    const double iconSize = 46;
    painter.save();
    painter.setOpacity(0.92);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(108, 140, 255, 26));
    painter.drawEllipse(center + QPointF(0, -58), iconSize / 1.35, iconSize / 1.35);
    Icons::draw(painter, QStringLiteral("sparkle"),
                QRectF(center + QPointF(-15, -73), QSizeF(30, 30)), QColor("#8CA5FF"));

    QFont title = font();
    title.setPixelSize(18);
    title.setBold(true);
    painter.setFont(title);
    painter.setPen(QColor(232, 236, 246));
    painter.drawText(QRectF(center.x() - 320, center.y() + 4, 640, 30),
                     Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("开始你的第一个界面"));

    QFont subtitle = font();
    subtitle.setPixelSize(12);
    painter.setFont(subtitle);
    painter.setPen(QColor(141, 150, 170));
    painter.drawText(QRectF(center.x() - 320, center.y() + 38, 640, 24),
                     Qt::AlignHCenter | Qt::AlignTop,
                     QStringLiteral("从左侧选择矩形、椭圆、直线、多边形或文字工具，在画板上拖拽绘制"));

    painter.setPen(QPen(QColor(255, 255, 255, 18), 1.0));
    painter.drawLine(QPointF(center.x() - 80, center.y() + 82),
                     QPointF(center.x() + 80, center.y() + 82));
    painter.setPen(QColor(104, 114, 136));
    QFont hint = font();
    hint.setPixelSize(11);
    painter.setFont(hint);
    painter.drawText(QRectF(center.x() - 320, center.y() + 92, 640, 22),
                     Qt::AlignHCenter | Qt::AlignTop,
                     QStringLiteral("提示：V 选择 · R 矩形 · O 椭圆 · P 多边形 · T 文字 · 空格拖动画布"));
    painter.restore();
}

void CanvasWidget::paintSelection(QPainter& painter)
{
    if (!m_document || m_selection.isEmpty()) return;
    painter.save();

    if (m_selection.size() > 1) {
        for (const auto& id : m_selection) {
            const Element* element = m_document->findElement(id);
            if (!element || !element->visible) continue;
            painter.setPen(QPen(QColor(108, 140, 255, 130), 1.0));
            painter.setBrush(Qt::NoBrush);
            painter.drawPolygon(docToScreen(element->rotatedCorners()));
        }
        const QRectF b = docToScreen(selectionBounds());
        painter.setPen(QPen(QColor(108, 140, 255, 210), 1.0, Qt::DashLine));
        painter.drawRect(b);
        painter.restore();
        return;
    }

    const Element* element = primaryElement();
    if (!element || !element->visible) {
        painter.restore();
        return;
    }
    const QColor outline = element->locked ? QColor("#FFB454") : QColor("#6C8CFF");
    painter.setPen(QPen(outline, 1.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(docToScreen(element->rotatedCorners()));

    if (element->locked) {
        painter.restore();
        return;
    }

    if (m_dragMode == DragMode::Create) {
        painter.restore();
        return;
    }

    const QPointF topCenter = handlePoint(*element, 1);
    const QPointF rotatePointScreen = rotationHandlePoint(*element);
    painter.setPen(QPen(QColor(108, 140, 255, 150), 1.0, Qt::DashLine));
    painter.drawLine(topCenter, rotatePointScreen);

    painter.setPen(QPen(QColor("#6C8CFF"), 1.4));
    painter.setBrush(QColor("#0D1119"));
    painter.drawEllipse(rotatePointScreen, 5.2, 5.2);
    painter.setBrush(QColor("#6C8CFF"));
    painter.setPen(Qt::NoPen);
    QPainterPath arrow;
    arrow.moveTo(rotatePointScreen + QPointF(-2.4, 1.6));
    arrow.lineTo(rotatePointScreen + QPointF(0, -2.6));
    arrow.lineTo(rotatePointScreen + QPointF(2.4, 1.6));
    arrow.closeSubpath();
    painter.drawPath(arrow);

    for (int i = 0; i < 8; ++i) {
        const QPointF p = handlePoint(*element, i);
        const QRectF handle(p.x() - kHandleScreenSize / 2, p.y() - kHandleScreenSize / 2,
                            kHandleScreenSize, kHandleScreenSize);
        painter.setPen(QPen(QColor("#6C8CFF"), 1.3));
        painter.setBrush(QColor("#F4F7FF"));
        painter.drawRoundedRect(handle, 2.0, 2.0);
    }
    painter.restore();
}

void CanvasWidget::paintInteractionBadge(QPainter& painter)
{
    if (!m_document) return;
    QSet<QString> sources;
    for (const auto& interaction : m_document->interactions) sources.insert(interaction.sourceId);
    if (sources.isEmpty()) return;

    for (const auto& element : m_document->elements) {
        if (!element.visible || !sources.contains(element.id)) continue;
        if (m_selection.contains(element.id)) continue;
        const QRectF r = element.boundingRect();
        const QPointF corner = docToScreen(element.localToDocument(QPointF(r.right(), r.top())));
        const QPointF p = corner + QPointF(9, -9);
        painter.save();
        painter.setPen(QPen(QColor(108, 140, 255, 180), 1.0));
        painter.setBrush(QColor(14, 18, 28, 230));
        painter.drawEllipse(p, 7.5, 7.5);
        QPainterPath triangle;
        triangle.moveTo(p + QPointF(-2.0, -3.2));
        triangle.lineTo(p + QPointF(3.4, 0));
        triangle.lineTo(p + QPointF(-2.0, 3.2));
        triangle.closeSubpath();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#8CA5FF"));
        painter.drawPath(triangle);
        painter.restore();
    }
}

void CanvasWidget::paintGuides(QPainter& painter)
{
    if (!m_showGuides || !m_document || m_document->guides.isEmpty()) return;
    const QRectF artboard = docToScreen(QRectF(QPointF(0, 0), m_document->canvasSize));
    painter.save();
    for (int i = 0; i < m_document->guides.size(); ++i) {
        const Guide& guide = m_document->guides[i];
        const bool horizontal = guide.orientation == QLatin1String("horizontal");
        const double dimension = horizontal ? m_document->canvasSize.height()
                                            : m_document->canvasSize.width();
        const bool inside = guide.position >= 0.0 && guide.position <= dimension;
        const bool creating = m_dragMode == DragMode::Guide && i == m_activeGuide;
        const QColor color = (creating && !inside) ? QColor(255, 107, 129, 230)
                                                   : QColor(0, 168, 255, 205);
        painter.setPen(QPen(color, 1.0));
        if (horizontal) {
            double y = docToScreen(QPointF(0, guide.position)).y();
            if (inside) {
                if (y >= artboard.top() - 0.5 && y <= artboard.bottom() + 0.5) {
                    painter.drawLine(QPointF(artboard.left(), y), QPointF(artboard.right(), y));
                }
            } else if (creating) {
                y = qBound(0.0, y, double(height()));
                painter.drawLine(QPointF(0, y), QPointF(width(), y));
            }
        } else {
            double x = docToScreen(QPointF(guide.position, 0)).x();
            if (inside) {
                if (x >= artboard.left() - 0.5 && x <= artboard.right() + 0.5) {
                    painter.drawLine(QPointF(x, artboard.top()), QPointF(x, artboard.bottom()));
                }
            } else if (creating) {
                x = qBound(0.0, x, double(width()));
                painter.drawLine(QPointF(x, 0), QPointF(x, height()));
            }
        }
    }
    painter.restore();
}

void CanvasWidget::paintPenPreview(QPainter& painter)
{
    if (m_penPoints.isEmpty()) return;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QVector<QPointF> screenPoints;
    for (const auto& point : m_penPoints) screenPoints.append(docToScreen(point));
    if (m_hasLastMouse && m_tool == Tool::Pen) {
        screenPoints.append(docToScreen(m_penHover));
    }

    QPainterPath path;
    if (!screenPoints.isEmpty()) path.moveTo(screenPoints.first());
    for (int i = 1; i < screenPoints.size(); ++i) path.lineTo(screenPoints[i]);
    painter.setPen(QPen(QColor(108, 140, 255, 220), 1.6));
    painter.setBrush(QColor(108, 140, 255, 26));
    painter.drawPath(path);

    for (int i = 0; i < screenPoints.size(); ++i) {
        const bool first = (i == 0);
        painter.setPen(QPen(first ? QColor("#FFFFFF") : QColor("#6C8CFF"), 1.4));
        painter.setBrush(first ? QColor("#6C8CFF") : QColor("#0D1119"));
        painter.drawEllipse(screenPoints[i], first ? 5.0 : 3.5, first ? 5.0 : 3.5);
    }

    if (m_penNearStart && !m_penPoints.isEmpty()) {
        const QPointF p = docToScreen(m_penPoints.first());
        painter.setPen(QPen(QColor("#3DDC97"), 1.4));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(p, 10, 10);
    }

    if (!m_penPoints.isEmpty()) {
        paintBadge(painter, docToScreen(m_penPoints.last()),
                   m_penNearStart ? QStringLiteral("松开以闭合路径")
                                  : QStringLiteral("双击 / Enter 完成 · Esc 取消"));
    }
    painter.restore();
}

void CanvasWidget::paintBadge(QPainter& painter, const QPointF& screenPoint, const QString& text)
{
    if (text.isEmpty()) return;
    painter.save();
    QFont f = font();
    f.setPixelSize(11);
    painter.setFont(f);
    const QFontMetricsF fm(f);
    const QSizeF size(fm.horizontalAdvance(text) + 18, fm.height() + 10);
    QPointF pos = screenPoint + QPointF(15, -size.height() - 12);
    pos.setX(qBound(6.0, pos.x(), width() - size.width() - 6.0));
    pos.setY(qBound(6.0, pos.y(), height() - size.height() - 6.0));
    const QRectF box(pos, size);
    painter.setPen(QPen(QColor(108, 140, 255, 140), 1.0));
    painter.setBrush(QColor(13, 17, 26, 238));
    painter.drawRoundedRect(box, 5, 5);
    painter.setPen(QColor("#DCE4F5"));
    painter.drawText(box, Qt::AlignCenter, text);
    painter.restore();
}

void CanvasWidget::paintRulers(QPainter& painter)
{
    if (!m_showRulers) return;
    const int r = m_rulerSize;
    painter.save();
    painter.fillRect(QRect(0, 0, width(), r), QColor(12, 15, 22, 248));
    painter.fillRect(QRect(0, 0, r, height()), QColor(12, 15, 22, 248));
    painter.fillRect(QRect(0, 0, r, r), QColor(15, 19, 28, 255));

    painter.setPen(QPen(QColor(37, 44, 59), 1.0));
    painter.drawLine(0, r, width(), r);
    painter.drawLine(r, 0, r, height());
    painter.drawLine(r, r, width(), r);
    painter.drawLine(r, r, r, height());

    painter.setPen(QColor(102, 113, 137));
    QFont tiny = font();
    tiny.setPixelSize(8);
    painter.setFont(tiny);
    painter.drawText(QRect(0, 0, r, r), Qt::AlignCenter, QStringLiteral("px"));

    const double step = niceRulerStep(m_zoom);
    const double minor = step / 5.0;
    const QPointF topLeftDoc = screenToDoc(QPointF(r, r));
    const QPointF bottomRightDoc = screenToDoc(QPointF(width(), height()));

    const double startX = std::floor(topLeftDoc.x() / step) * step;
    const double endX = bottomRightDoc.x();
    for (double x = startX; x <= endX + step; x += step) {
        const double sx = docToScreen(QPointF(x, 0)).x();
        if (sx < r - 1 || sx > width()) continue;
        painter.setPen(QPen(QColor(53, 63, 84), 1.0));
        painter.drawLine(QPointF(sx, r - 7), QPointF(sx, r));
        painter.setPen(QColor(126, 138, 163));
        QString label = QString::number(qRound(x));
        painter.drawText(QRectF(sx + 3, 3, 70, 11), Qt::AlignLeft | Qt::AlignVCenter, label);
        painter.setPen(QPen(QColor(39, 47, 63), 1.0));
        for (int k = 1; k < 5; ++k) {
            const double mx = docToScreen(QPointF(x + minor * k, 0)).x();
            if (mx < r || mx > width()) continue;
            painter.drawLine(QPointF(mx, r - 3), QPointF(mx, r));
        }
    }

    const double startY = std::floor(topLeftDoc.y() / step) * step;
    const double endY = bottomRightDoc.y();
    for (double y = startY; y <= endY + step; y += step) {
        const double sy = docToScreen(QPointF(0, y)).y();
        if (sy < r - 1 || sy > height()) continue;
        painter.setPen(QPen(QColor(53, 63, 84), 1.0));
        painter.drawLine(QPointF(r - 7, sy), QPointF(r, sy));
        painter.setPen(QColor(126, 138, 163));
        painter.drawText(QRectF(2, sy - 6, r - 9, 12), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(qRound(y)));
        painter.setPen(QPen(QColor(39, 47, 63), 1.0));
        for (int k = 1; k < 5; ++k) {
            const double my = docToScreen(QPointF(0, y + minor * k)).y();
            if (my < r || my > height()) continue;
            painter.drawLine(QPointF(r - 3, my), QPointF(r, my));
        }
    }

    if (m_hasLastMouse && m_lastMouse.x() > r && m_lastMouse.y() > r) {
        painter.setPen(QPen(QColor(108, 140, 255, 210), 1.0));
        painter.drawLine(QPointF(m_lastMouse.x(), 0), QPointF(m_lastMouse.x(), r));
        painter.drawLine(QPointF(0, m_lastMouse.y()), QPointF(r, m_lastMouse.y()));
        painter.setBrush(QColor("#6C8CFF"));
        painter.setPen(Qt::NoPen);
        QPolygonF topMarker;
        topMarker << QPointF(m_lastMouse.x() - 4, r - 5)
                  << QPointF(m_lastMouse.x() + 4, r - 5)
                  << QPointF(m_lastMouse.x(), r);
        painter.drawPolygon(topMarker);
        QPolygonF leftMarker;
        leftMarker << QPointF(r - 5, m_lastMouse.y() - 4)
                   << QPointF(r - 5, m_lastMouse.y() + 4)
                   << QPointF(r, m_lastMouse.y());
        painter.drawPolygon(leftMarker);
    }
    painter.restore();
}

// ---------------------------------------------------------------------------
// Element creation
// ---------------------------------------------------------------------------
Element CanvasWidget::makeNewElement(ElementType type, const QRectF& frame) const
{
    Element element;
    element.type = type;
    element.id = m_document->makeId(
        type == ElementType::Rectangle ? QStringLiteral("rect") :
        type == ElementType::Ellipse ? QStringLiteral("ellipse") :
        type == ElementType::Line ? QStringLiteral("line") :
        type == ElementType::Polygon ? QStringLiteral("polygon") :
        type == ElementType::Star ? QStringLiteral("star") :
        type == ElementType::Path ? QStringLiteral("path") : QStringLiteral("text"));
    element.name = m_document->uniqueName(elementTypeDisplayName(type));
    element.frame = frame;

    switch (type) {
    case ElementType::Rectangle:
        element.fill = m_defaultFill;
        element.radius = m_defaultRadius;
        element.stroke = m_defaultStroke;
        element.strokeWidth = m_defaultStrokeWidth;
        break;
    case ElementType::Ellipse:
        element.fill = m_defaultFill;
        element.stroke = m_defaultStroke;
        element.strokeWidth = m_defaultStrokeWidth;
        break;
    case ElementType::Line:
        element.fill = m_defaultStroke;
        element.stroke = m_defaultStroke;
        element.strokeWidth = qMax(2.0, m_defaultStrokeWidth);
        element.lineStart = QPointF(0, 0.5);
        element.lineEnd = QPointF(1, 0.5);
        break;
    case ElementType::Polygon:
        element.fill = m_defaultFill;
        element.stroke = m_defaultStroke;
        element.strokeWidth = m_defaultStrokeWidth;
        element.sides = m_defaultSides;
        break;
    case ElementType::Star:
        element.fill = m_defaultFill;
        element.stroke = m_defaultStroke;
        element.strokeWidth = m_defaultStrokeWidth;
        element.sides = m_defaultSides;
        break;
    case ElementType::Path:
        element.fill = QColor(m_defaultFill.red(), m_defaultFill.green(),
                              m_defaultFill.blue(), 190);
        element.stroke = m_defaultStroke;
        element.strokeWidth = qMax(1.0, m_defaultStrokeWidth);
        break;
    case ElementType::Text:
        element.fill = QColor(0, 0, 0, 0);
        element.stroke = QColor(0, 0, 0, 0);
        element.textColor = QColor("#E8ECF6");
        element.text = QStringLiteral("输入文本");
        element.fontSize = 28;
        break;
    }
    return element;
}

QString CanvasWidget::addElement(Element element)
{
    m_document->addElement(element);
    notifyEdited();
    return element.id;
}

void CanvasWidget::notifyEdited()
{
    update();
    emit documentEdited();
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------
void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    setFocus();
    m_lastMouse = event->position();
    m_hasLastMouse = true;
    if (m_textEditor) commitTextEditing();

    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && (m_spaceDown || m_tool == Tool::Hand))) {
        beginPan(event->position());
        return;
    }
    if (event->button() != Qt::LeftButton) return;

    if (m_showRulers) {
        if (event->position().y() < m_rulerSize && event->position().x() >= m_rulerSize) {
            beginGuideCreation(true, screenToDoc(event->position()));
            return;
        }
        if (event->position().x() < m_rulerSize && event->position().y() >= m_rulerSize) {
            beginGuideCreation(false, screenToDoc(event->position()));
            return;
        }
        if (event->position().x() < m_rulerSize || event->position().y() < m_rulerSize) {
            return;
        }
    }

    if (m_tool == Tool::Zoom) {
        applyZoom(m_zoom * ((event->modifiers() & Qt::AltModifier) ? 0.8 : 1.25),
                  event->position());
        return;
    }

    if (m_tool == Tool::Select) {
        const int guideIndex = guideAt(event->position());
        if (guideIndex >= 0) {
            beginGuideDrag(guideIndex);
            return;
        }
        if (m_selection.size() == 1) {
            if (hitRotationHandle(event->position())) {
                beginRotate();
                return;
            }
            const int handle = hitHandle(event->position());
            if (handle >= 0) {
                beginResize(handle);
                return;
            }
        }

        Element* hit = topElementAt(event->position());
        const bool shift = event->modifiers() & Qt::ShiftModifier;
        if (hit) {
            if (shift && m_selection.contains(hit->id)) {
                QStringList selection = m_selection;
                selection.removeAll(hit->id);
                setSelection(selection);
                update();
                return;
            }
            if (shift) {
                QStringList selection = m_selection;
                selection.append(hit->id);
                setSelection(selection);
            } else if (!m_selection.contains(hit->id)) {
                setSelection(QStringList{hit->id});
            }
            beginMove();
        } else {
            if (!shift) clearSelection();
            m_marqueeBaseSelection = shift ? m_selection : QStringList();
            beginMarquee();
        }
        return;
    }

    if (m_tool == Tool::Text) {
        if (event->position().x() < m_rulerSize || event->position().y() < m_rulerSize) return;
        const QPointF point = screenToDoc(event->position());
        emit editBegan();
        Element element = makeNewElement(ElementType::Text,
                                         QRectF(point.x(), point.y(), 300, 72));
        element.text = QStringLiteral("输入文本");
        m_document->addElement(element);
        setSelection(QStringList{element.id});
        notifyEdited();
        beginTextEditing(element.id, true);
        m_tool = Tool::Select;
        emit toolRequested(Tool::Select);
        return;
    }

    if (m_tool == Tool::Pen) {
        addPenPoint(screenToDoc(event->position()));
        return;
    }

    if (m_tool == Tool::Rectangle || m_tool == Tool::Ellipse || m_tool == Tool::Line ||
        m_tool == Tool::Polygon || m_tool == Tool::Star) {
        beginCreate(screenToDoc(event->position()));
        return;
    }
}

void CanvasWidget::beginPan(const QPointF& screenPoint)
{
    m_dragMode = DragMode::Pan;
    m_dragStartScreen = screenPoint;
    m_panStartOrigin = m_origin;
    setCursor(Qt::ClosedHandCursor);
    emit statusHint(QStringLiteral("平移画布中…"));
}

void CanvasWidget::beginMove()
{
    if (!m_document || m_selection.isEmpty()) return;
    bool hasMovable = false;
    m_startFrames.clear();
    for (const auto& id : m_selection) {
        Element* element = m_document->findElement(id);
        if (!element || element->locked) continue;
        m_startFrames.insert(id, element->frame);
        hasMovable = true;
    }
    if (!hasMovable) return;
    emit editBegan();
    m_dragMode = DragMode::Move;
    m_activeId = m_selection.first();
    m_dragStartScreen = m_lastMouse;
    m_dragStartDoc = screenToDoc(m_lastMouse);
    m_guides.clear();
    m_badgeVisible = false;
}

void CanvasWidget::beginResize(int handle)
{
    if (!m_document || m_selection.size() != 1) return;
    Element* element = primaryElement();
    if (!element || element->locked) return;
    emit editBegan();
    m_dragMode = DragMode::Resize;
    m_activeHandle = handle;
    m_activeId = element->id;
    m_dragStartScreen = m_lastMouse;
    m_dragStartDoc = screenToDoc(m_lastMouse);
    m_startFrames.clear();
    m_startFrames.insert(element->id, element->frame);
    m_startRotations.clear();
    m_startRotations.insert(element->id, element->rotation);
    m_badgeVisible = true;
}

void CanvasWidget::beginRotate()
{
    if (!m_document || m_selection.size() != 1) return;
    Element* element = primaryElement();
    if (!element || element->locked) return;
    emit editBegan();
    m_dragMode = DragMode::Rotate;
    m_activeId = element->id;
    m_dragStartScreen = m_lastMouse;
    m_dragStartDoc = screenToDoc(m_lastMouse);
    m_startRotations.clear();
    m_startRotations.insert(element->id, element->rotation);
    m_startFrames.clear();
    m_startFrames.insert(element->id, element->frame);
    m_badgeVisible = true;
}

void CanvasWidget::beginCreate(const QPointF& documentPoint)
{
    if (!m_document) return;
    ElementType type = ElementType::Rectangle;
    switch (m_tool) {
    case Tool::Rectangle: type = ElementType::Rectangle; break;
    case Tool::Ellipse: type = ElementType::Ellipse; break;
    case Tool::Line: type = ElementType::Line; break;
    case Tool::Polygon: type = ElementType::Polygon; break;
    case Tool::Star: type = ElementType::Star; break;
    default: return;
    }
    emit editBegan();
    m_createStartDoc = documentPoint;
    m_createCurrentDoc = documentPoint;
    Element element = makeNewElement(type, QRectF(documentPoint, QSizeF(1, 1)));
    m_creationId = element.id;
    m_document->addElement(element);
    setSelection(QStringList{element.id});
    m_dragMode = DragMode::Create;
    m_startFrames.clear();
    m_badgeVisible = true;
    m_sizeBadge = QStringLiteral("拖拽绘制");
    m_badgeScreenPoint = m_lastMouse;
    notifyEdited();
}

void CanvasWidget::beginMarquee()
{
    m_dragMode = DragMode::Marquee;
    m_dragStartScreen = m_lastMouse;
    m_dragStartDoc = screenToDoc(m_lastMouse);
    m_marqueeRect = QRectF(m_dragStartDoc, QSizeF(0, 0));
    m_guides.clear();
}

// ---------------------------------------------------------------------------
// Guides
// ---------------------------------------------------------------------------
int CanvasWidget::guideAt(const QPointF& screenPoint) const
{
    if (!m_showGuides || !m_document || m_document->guides.isEmpty()) return -1;
    const QRectF artboard = docToScreen(QRectF(QPointF(0, 0), m_document->canvasSize));
    for (int i = m_document->guides.size() - 1; i >= 0; --i) {
        const Guide& guide = m_document->guides[i];
        const bool horizontal = guide.orientation == QLatin1String("horizontal");
        const double dimension = horizontal ? m_document->canvasSize.height()
                                            : m_document->canvasSize.width();
        if (guide.position < 0.0 || guide.position > dimension) continue;
        if (horizontal) {
            const double y = docToScreen(QPointF(0, guide.position)).y();
            if (qAbs(screenPoint.y() - y) <= 5.0 &&
                screenPoint.x() >= artboard.left() && screenPoint.x() <= artboard.right()) {
                return i;
            }
        } else {
            const double x = docToScreen(QPointF(guide.position, 0)).x();
            if (qAbs(screenPoint.x() - x) <= 5.0 &&
                screenPoint.y() >= artboard.top() && screenPoint.y() <= artboard.bottom()) {
                return i;
            }
        }
    }
    return -1;
}

void CanvasWidget::beginGuideCreation(bool horizontal, const QPointF& documentPoint)
{
    if (!m_document) return;
    emit editBegan();
    const double position = horizontal ? documentPoint.y() : documentPoint.x();
    m_activeGuide = m_document->addGuide(horizontal ? QStringLiteral("horizontal")
                                                   : QStringLiteral("vertical"),
                                          position);
    m_guideIsNew = true;
    m_guideStartPosition = position;
    m_dragMode = DragMode::Guide;
    m_badgeVisible = true;
    m_badgeScreenPoint = m_lastMouse;
    m_sizeBadge = horizontal ? QStringLiteral("Y %1 px").arg(qRound(position))
                             : QStringLiteral("X %1 px").arg(qRound(position));
    setCursor(horizontal ? Qt::SizeVerCursor : Qt::SizeHorCursor);
    notifyEdited();
    emit statusHint(horizontal ? QStringLiteral("横向参考线：拖动定位，拖出画板或按 Esc 删除")
                               : QStringLiteral("纵向参考线：拖动定位，拖出画板或按 Esc 删除"));
}

void CanvasWidget::beginGuideDrag(int index)
{
    if (!m_document || index < 0 || index >= m_document->guides.size()) return;
    emit editBegan();
    m_activeGuide = index;
    m_guideIsNew = false;
    m_guideStartPosition = m_document->guides[index].position;
    m_dragMode = DragMode::Guide;
    m_badgeVisible = true;
    m_badgeScreenPoint = m_lastMouse;
    const bool horizontal = m_document->guides[index].orientation == QLatin1String("horizontal");
    m_sizeBadge = horizontal ? QStringLiteral("Y %1 px").arg(qRound(m_guideStartPosition))
                             : QStringLiteral("X %1 px").arg(qRound(m_guideStartPosition));
    setCursor(horizontal ? Qt::SizeVerCursor : Qt::SizeHorCursor);
    emit statusHint(QStringLiteral("拖动参考线调整位置，拖出画板可删除"));
}

void CanvasWidget::updateGuide(const QPointF& documentPoint)
{
    if (!m_document || m_activeGuide < 0 || m_activeGuide >= m_document->guides.size()) return;
    Guide& guide = m_document->guides[m_activeGuide];
    const bool horizontal = guide.orientation == QLatin1String("horizontal");
    double value = horizontal ? documentPoint.y() : documentPoint.x();
    const double dimension = horizontal ? m_document->canvasSize.height()
                                        : m_document->canvasSize.width();

    if (m_snapEnabled) {
        const double threshold = 6.0 / m_zoom;
        QVector<double> candidates{0.0, dimension / 2.0, dimension};
        for (const auto& element : m_document->elements) {
            if (!element.visible) continue;
            const QRectF frame = element.rotatedCorners().boundingRect();
            if (horizontal) {
                candidates << frame.top() << frame.center().y() << frame.bottom();
            } else {
                candidates << frame.left() << frame.center().x() << frame.right();
            }
        }
        double best = threshold + 1.0;
        double snapped = value;
        for (double candidate : candidates) {
            const double diff = qAbs(candidate - value);
            if (diff < best) {
                best = diff;
                snapped = candidate;
            }
        }
        if (best <= threshold) value = snapped;
        else value = qRound(value);
    } else {
        value = qRound(value * 10.0) / 10.0;
    }

    guide.position = value;
    const bool inside = value >= 0.0 && value <= dimension;
    m_sizeBadge = (horizontal ? QStringLiteral("Y %1 px") : QStringLiteral("X %1 px"))
                      .arg(QString::number(value, 'f', 1));
    if (!inside) m_sizeBadge += QStringLiteral(" · 松开删除");
    m_badgeScreenPoint = m_lastMouse;
    m_badgeVisible = true;
    notifyEdited();
}

void CanvasWidget::finishGuide(bool cancel)
{
    if (!m_document || m_activeGuide < 0 || m_activeGuide >= m_document->guides.size()) {
        m_dragMode = DragMode::None;
        m_activeGuide = -1;
        m_badgeVisible = false;
        update();
        return;
    }
    const Guide guide = m_document->guides[m_activeGuide];
    const bool horizontal = guide.orientation == QLatin1String("horizontal");
    const double dimension = horizontal ? m_document->canvasSize.height()
                                        : m_document->canvasSize.width();
    const bool inside = guide.position >= 0.0 && guide.position <= dimension;

    if (cancel) {
        if (m_guideIsNew) m_document->removeGuide(m_activeGuide);
        else m_document->guides[m_activeGuide].position = m_guideStartPosition;
        notifyEdited();
    } else if (!inside) {
        m_document->removeGuide(m_activeGuide);
        notifyEdited();
        emit statusHint(QStringLiteral("参考线已删除"));
    } else {
        emit statusHint(horizontal ? QStringLiteral("已创建横向参考线 Y = %1 px").arg(qRound(guide.position))
                                   : QStringLiteral("已创建纵向参考线 X = %1 px").arg(qRound(guide.position)));
    }

    m_dragMode = DragMode::None;
    m_activeGuide = -1;
    m_guideIsNew = false;
    m_badgeVisible = false;
    m_sizeBadge.clear();
    update();
    emit editEnded();
}

void CanvasWidget::updateMove(const QPointF& documentPoint, bool)
{
    if (!m_document || m_dragMode != DragMode::Move) return;
    QPointF delta = documentPoint - m_dragStartDoc;
    QRectF proposed = m_startFrames.value(m_activeId);
    QRectF unionRect;
    for (auto it = m_startFrames.constBegin(); it != m_startFrames.constEnd(); ++it) {
        const QRectF r = it.value().translated(delta);
        unionRect = unionRect.isNull() ? r : unionRect.united(r);
    }
    if (!unionRect.isNull()) {
        const SnapResult snap = snapRect(unionRect, QSet<QString>(m_selection.begin(), m_selection.end()));
        delta += snap.offset;
        m_guides = snap.guides;
    }
    for (auto it = m_startFrames.constBegin(); it != m_startFrames.constEnd(); ++it) {
        Element* element = m_document->findElement(it.key());
        if (!element) continue;
        element->frame = it.value().translated(delta);
    }
    m_sizeBadge = QStringLiteral("X %1   Y %2")
                      .arg(qRound(proposed.translated(delta).x()))
                      .arg(qRound(proposed.translated(delta).y()));
    m_badgeScreenPoint = m_lastMouse;
    notifyEdited();
}

void CanvasWidget::updateResize(const QPointF& documentPoint, bool shift)
{
    if (!m_document || m_activeId.isEmpty()) return;
    Element* element = m_document->findElement(m_activeId);
    if (!element || !m_startFrames.contains(m_activeId)) return;
    const QRectF start = m_startFrames.value(m_activeId);
    const double rotation = m_startRotations.value(m_activeId);

    QPointF anchorLocal;
    switch (m_activeHandle) {
    case 0: anchorLocal = start.bottomRight(); break;
    case 1: anchorLocal = QPointF(start.center().x(), start.bottom()); break;
    case 2: anchorLocal = start.bottomLeft(); break;
    case 3: anchorLocal = QPointF(start.left(), start.center().y()); break;
    case 4: anchorLocal = start.topLeft(); break;
    case 5: anchorLocal = QPointF(start.center().x(), start.top()); break;
    case 6: anchorLocal = start.topRight(); break;
    case 7: anchorLocal = QPointF(start.right(), start.center().y()); break;
    default: return;
    }

    const QPointF startCenter = start.center();
    const double rad = qDegreesToRadians(rotation);
    const QPointF anchorWorld(
        startCenter.x() + (anchorLocal.x() - startCenter.x()) * qCos(rad) -
            (anchorLocal.y() - startCenter.y()) * qSin(rad),
        startCenter.y() + (anchorLocal.x() - startCenter.x()) * qSin(rad) +
            (anchorLocal.y() - startCenter.y()) * qCos(rad));

    QPointF mouseLocal = rotatePoint(documentPoint, anchorWorld, -rotation) - anchorWorld;
    double width = start.width();
    double height = start.height();
    double signW = 1.0;
    double signH = 1.0;

    switch (m_activeHandle) {
    case 0: signW = -1; signH = -1; break;
    case 1: signH = -1; break;
    case 2: signW = 1; signH = -1; break;
    case 3: signW = 1; break;
    case 4: signW = 1; signH = 1; break;
    case 5: signH = 1; break;
    case 6: signW = -1; signH = 1; break;
    case 7: signW = -1; break;
    default: break;
    }

    if (m_activeHandle == 0 || m_activeHandle == 2 || m_activeHandle == 4 ||
        m_activeHandle == 6) {
        width = qMax(4.0, qAbs(mouseLocal.x()));
        height = qMax(4.0, qAbs(mouseLocal.y()));
        if (shift) {
            const double ratio = start.width() / qMax(1.0, start.height());
            if (width / height > ratio) height = width / ratio;
            else width = height * ratio;
        }
    } else if (m_activeHandle == 1 || m_activeHandle == 5) {
        height = qMax(4.0, qAbs(mouseLocal.y()));
    } else {
        width = qMax(4.0, qAbs(mouseLocal.x()));
    }
    Q_UNUSED(signW);
    Q_UNUSED(signH);

    const QPointF centerLocal(width / 2.0, height / 2.0);
    QPointF anchorInNew;
    switch (m_activeHandle) {
    case 0: anchorInNew = QPointF(width, height); break;
    case 1: anchorInNew = QPointF(width / 2.0, height); break;
    case 2: anchorInNew = QPointF(0, height); break;
    case 3: anchorInNew = QPointF(0, height / 2.0); break;
    case 4: anchorInNew = QPointF(0, 0); break;
    case 5: anchorInNew = QPointF(width / 2.0, 0); break;
    case 6: anchorInNew = QPointF(width, 0); break;
    case 7: anchorInNew = QPointF(width, height / 2.0); break;
    default: return;
    }

    const QPointF localDelta = centerLocal - anchorInNew;
    const QPointF centerWorld(
        anchorWorld.x() + localDelta.x() * qCos(rad) - localDelta.y() * qSin(rad),
        anchorWorld.y() + localDelta.x() * qSin(rad) + localDelta.y() * qCos(rad));

    element->frame = QRectF(centerWorld.x() - width / 2.0, centerWorld.y() - height / 2.0,
                            width, height);
    m_sizeBadge = QStringLiteral("%1 × %2").arg(qRound(width)).arg(qRound(height));
    m_badgeScreenPoint = m_lastMouse;
    notifyEdited();
}

void CanvasWidget::updateRotate(const QPointF& documentPoint, bool shift)
{
    if (!m_document || m_activeId.isEmpty()) return;
    Element* element = m_document->findElement(m_activeId);
    if (!element) return;
    const QPointF center = element->center();
    const double startAngle = qRadiansToDegrees(
        std::atan2(m_dragStartDoc.y() - center.y(), m_dragStartDoc.x() - center.x()));
    const double currentAngle = qRadiansToDegrees(
        std::atan2(documentPoint.y() - center.y(), documentPoint.x() - center.x()));
    double rotation = m_startRotations.value(m_activeId) + currentAngle - startAngle;
    if (shift) rotation = qRound(rotation / 15.0) * 15.0;
    while (rotation > 180.0) rotation -= 360.0;
    while (rotation <= -180.0) rotation += 360.0;
    element->rotation = rotation;
    m_sizeBadge = QStringLiteral("%1°").arg(qRound(rotation));
    m_badgeScreenPoint = m_lastMouse;
    notifyEdited();
}

void CanvasWidget::updateMarquee()
{
    if (!m_document) return;
    QRectF rect = QRectF(m_dragStartDoc, screenToDoc(m_lastMouse)).normalized();
    m_marqueeRect = rect;
    QStringList result = m_marqueeBaseSelection;
    for (const auto& element : m_document->elements) {
        if (!element.visible || element.locked) continue;
        const QRectF bounds = element.rotatedCorners().boundingRect();
        if (rect.intersects(bounds) && !result.contains(element.id)) result.append(element.id);
    }
    setSelection(result);
    update();
}

CanvasWidget::SnapResult CanvasWidget::snapRect(QRectF rect, const QSet<QString>& ignore) const
{
    SnapResult result;
    if (!m_snapEnabled || !m_document) return result;
    const double threshold = 6.0 / m_zoom;
    QVector<double> xs{0.0, m_document->canvasSize.width() / 2.0, m_document->canvasSize.width()};
    QVector<double> ys{0.0, m_document->canvasSize.height() / 2.0, m_document->canvasSize.height()};
    for (const auto& element : m_document->elements) {
        if (!element.visible || ignore.contains(element.id)) continue;
        const QRectF b = element.rotatedCorners().boundingRect();
        xs << b.left() << b.center().x() << b.right();
        ys << b.top() << b.center().y() << b.bottom();
    }

    double bestDiffX = threshold + 1.0;
    double bestAdjustX = 0.0;
    double guideX = 0.0;
    const double rectXs[] = {rect.left(), rect.center().x(), rect.right()};
    for (double candidate : xs) {
        for (double edge : rectXs) {
            const double diff = candidate - edge;
            if (qAbs(diff) < qAbs(bestDiffX) && qAbs(diff) <= threshold) {
                bestDiffX = diff;
                bestAdjustX = diff;
                guideX = candidate;
            }
        }
    }

    double bestDiffY = threshold + 1.0;
    double bestAdjustY = 0.0;
    double guideY = 0.0;
    const double rectYs[] = {rect.top(), rect.center().y(), rect.bottom()};
    for (double candidate : ys) {
        for (double edge : rectYs) {
            const double diff = candidate - edge;
            if (qAbs(diff) < qAbs(bestDiffY) && qAbs(diff) <= threshold) {
                bestDiffY = diff;
                bestAdjustY = diff;
                guideY = candidate;
            }
        }
    }

    if (bestDiffX > threshold) {
        const double grid = 10.0;
        const double snapped = qRound(rect.left() / grid) * grid;
        bestAdjustX = snapped - rect.left();
        if (qAbs(bestAdjustX) <= threshold) guideX = snapped;
        else bestAdjustX = 0.0;
    }
    if (bestDiffY > threshold) {
        const double grid = 10.0;
        const double snapped = qRound(rect.top() / grid) * grid;
        bestAdjustY = snapped - rect.top();
        if (qAbs(bestAdjustY) <= threshold) guideY = snapped;
        else bestAdjustY = 0.0;
    }

    result.offset = QPointF(bestAdjustX, bestAdjustY);
    if (qAbs(bestAdjustX) > 0.0001) {
        result.guides.append(QLineF(guideX, 0, guideX, m_document->canvasSize.height()));
    }
    if (qAbs(bestAdjustY) > 0.0001) {
        result.guides.append(QLineF(0, guideY, m_document->canvasSize.width(), guideY));
    }
    return result;
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    m_lastMouse = event->position();
    m_hasLastMouse = true;
    emit cursorMoved(screenToDoc(m_lastMouse));

    switch (m_dragMode) {
    case DragMode::Pan:
        m_origin = m_panStartOrigin + (m_lastMouse - m_dragStartScreen);
        update();
        return;
    case DragMode::Move:
        updateMove(screenToDoc(m_lastMouse), event->modifiers() & Qt::ShiftModifier);
        return;
    case DragMode::Resize:
        updateResize(screenToDoc(m_lastMouse), event->modifiers() & Qt::ShiftModifier);
        return;
    case DragMode::Rotate:
        updateRotate(screenToDoc(m_lastMouse), event->modifiers() & Qt::ShiftModifier);
        return;
    case DragMode::Marquee:
        updateMarquee();
        return;
    case DragMode::Create:
        updateCreate(screenToDoc(m_lastMouse), event->modifiers() & Qt::ShiftModifier);
        return;
    case DragMode::Guide:
        updateGuide(screenToDoc(m_lastMouse));
        return;
    default:
        break;
    }

    if (m_tool == Tool::Pen) {
        m_penHover = screenToDoc(m_lastMouse);
        m_penNearStart = m_penPoints.size() >= 3 &&
                          QLineF(m_penHover, m_penPoints.first()).length() <= 9.0 / m_zoom;
        update();
    }

    updateHover(m_lastMouse);
    updateCursor();
    update();
}

void CanvasWidget::updateHover(const QPointF& screenPoint)
{
    Element* hit = topElementAt(screenPoint);
    const QString id = hit ? hit->id : QString();
    if (id != m_hoverId) {
        m_hoverId = id;
        update();
    }
}

void CanvasWidget::updateCreate(const QPointF& documentPoint, bool shift)
{
    if (!m_document || m_creationId.isEmpty()) return;
    Element* element = m_document->findElement(m_creationId);
    if (!element) return;
    m_createCurrentDoc = documentPoint;

    if (element->type == ElementType::Line) {
        QPointF end = documentPoint;
        if (shift) {
            const QPointF delta = end - m_createStartDoc;
            const double length = std::hypot(delta.x(), delta.y());
            double angle = std::atan2(delta.y(), delta.x());
            angle = qRound(angle / (M_PI / 4.0)) * (M_PI / 4.0);
            end = m_createStartDoc + QPointF(length * qCos(angle), length * qSin(angle));
            m_createCurrentDoc = end;
        }
        element->setFrameFromPoints(m_createStartDoc, end);
        const QRectF frame = element->frame;
        const double w = qMax(0.01, frame.width());
        const double h = qMax(0.01, frame.height());
        element->lineStart = QPointF((m_createStartDoc.x() - frame.x()) / w,
                                     (m_createStartDoc.y() - frame.y()) / h);
        element->lineEnd = QPointF((end.x() - frame.x()) / w, (end.y() - frame.y()) / h);
        if (frame.height() < 2.0) {
            element->frame.setHeight(2.0);
            element->frame.moveTop(m_createStartDoc.y() - 1.0);
            element->lineStart.setY(0.5);
            element->lineEnd.setY(0.5);
        }
        m_sizeBadge = QStringLiteral("%1 px · %2°")
                          .arg(qRound(QLineF(m_createStartDoc, end).length()))
                          .arg(qRound(qRadiansToDegrees(std::atan2(end.y() - m_createStartDoc.y(),
                                                                    end.x() - m_createStartDoc.x()))));
    } else {
        QPointF end = documentPoint;
        if (shift) {
            const QPointF delta = end - m_createStartDoc;
            const double side = qMax(qAbs(delta.x()), qAbs(delta.y()));
            end = m_createStartDoc + QPointF(delta.x() < 0 ? -side : side,
                                             delta.y() < 0 ? -side : side);
        }
        element->setFrameFromPoints(m_createStartDoc, end);
        m_sizeBadge = QStringLiteral("%1 × %2")
                          .arg(qRound(element->frame.width()))
                          .arg(qRound(element->frame.height()));
    }
    m_badgeScreenPoint = m_lastMouse;
    notifyEdited();
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_dragMode == DragMode::None) return;
    finishDrag(false);
}

void CanvasWidget::finishDrag(bool cancel)
{
    if (m_dragMode == DragMode::Guide) {
        finishGuide(cancel);
        return;
    }
    const DragMode mode = m_dragMode;
    m_dragMode = DragMode::None;
    m_badgeVisible = false;
    m_guides.clear();

    if (mode == DragMode::Create) {
        Element* element = m_document ? m_document->findElement(m_creationId) : nullptr;
        if (element) {
            const double moved = QLineF(docToScreen(m_createStartDoc),
                                        docToScreen(m_createCurrentDoc)).length();
            if (cancel) {
                m_document->removeElement(m_creationId);
                clearSelection();
                notifyEdited();
                emit editEnded();
            } else if (moved < 4.0) {
                QRectF defaultFrame;
                switch (element->type) {
                case ElementType::Rectangle: defaultFrame = QRectF(m_createStartDoc, QSizeF(180, 110)); break;
                case ElementType::Ellipse: defaultFrame = QRectF(m_createStartDoc, QSizeF(130, 130)); break;
                case ElementType::Polygon: defaultFrame = QRectF(m_createStartDoc, QSizeF(150, 150)); break;
                case ElementType::Star: defaultFrame = QRectF(m_createStartDoc, QSizeF(150, 150)); break;
                case ElementType::Line: defaultFrame = QRectF(m_createStartDoc.x(),
                                                              m_createStartDoc.y() - 1, 200, 2); break;
                default: defaultFrame = QRectF(m_createStartDoc, QSizeF(160, 100)); break;
                }
                element->frame = defaultFrame;
                if (element->type == ElementType::Line) {
                    element->lineStart = QPointF(0, 0.5);
                    element->lineEnd = QPointF(1, 0.5);
                }
                notifyEdited();
                emit editEnded();
            } else {
                notifyEdited();
                emit editEnded();
            }
        } else {
            emit editEnded();
        }
        m_creationId.clear();
        m_startFrames.clear();
        update();
        if (!cancel) {
            setTool(Tool::Select);
            emit toolRequested(Tool::Select);
        }
        return;
    }

    if (mode == DragMode::Move || mode == DragMode::Resize || mode == DragMode::Rotate) {
        if (cancel) {
            if (mode == DragMode::Move) {
                for (auto it = m_startFrames.constBegin(); it != m_startFrames.constEnd(); ++it) {
                    if (Element* element = m_document->findElement(it.key())) element->frame = it.value();
                }
            } else if (mode == DragMode::Resize && !m_activeId.isEmpty()) {
                if (Element* element = m_document->findElement(m_activeId)) {
                    element->frame = m_startFrames.value(m_activeId);
                    element->rotation = m_startRotations.value(m_activeId);
                }
            } else if (mode == DragMode::Rotate && !m_activeId.isEmpty()) {
                if (Element* element = m_document->findElement(m_activeId)) {
                    element->rotation = m_startRotations.value(m_activeId);
                }
            }
            notifyEdited();
        }
        emit editEnded();
        m_startFrames.clear();
        m_startRotations.clear();
        update();
        return;
    }

    if (mode == DragMode::Marquee) {
        m_marqueeRect = QRectF();
        m_marqueeBaseSelection.clear();
        update();
        return;
    }

    if (mode == DragMode::Pan) {
        updateCursor();
        return;
    }

    m_marqueeRect = QRectF();
    update();
}

// ---------------------------------------------------------------------------
// Pen
// ---------------------------------------------------------------------------
void CanvasWidget::addPenPoint(const QPointF& documentPoint)
{
    if (!m_penPoints.isEmpty()) {
        const double distance = QLineF(documentPoint, m_penPoints.first()).length();
        if (m_penPoints.size() >= 3 && distance <= 9.0 / m_zoom) {
            finishPen(false);
            return;
        }
    }
    if (m_penPoints.isEmpty()) emit editBegan();
    m_penPoints.append(documentPoint);
    m_penHover = documentPoint;
    m_penNearStart = false;
    update();
}

void CanvasWidget::finishPen(bool cancel)
{
    if (cancel || m_penPoints.size() < 2) {
        cancelPen();
        if (m_tool != Tool::Select) emit editEnded();
        return;
    }
    QRectF bounds;
    for (const auto& point : m_penPoints) {
        bounds = bounds.isNull() ? QRectF(point, QSizeF(0, 0)) : bounds.united(QRectF(point, QSizeF(0, 0)));
    }
    bounds = bounds.adjusted(-1, -1, 1, 1);
    if (bounds.width() < 1.0) bounds.setWidth(1.0);
    if (bounds.height() < 1.0) bounds.setHeight(1.0);

    Element element = makeNewElement(ElementType::Path, bounds);
    element.closed = true;
    for (const auto& point : m_penPoints) {
        element.points.append(QPointF((point.x() - bounds.x()) / bounds.width(),
                                      (point.y() - bounds.y()) / bounds.height()));
    }
    m_document->addElement(element);
    m_penPoints.clear();
    setSelection(QStringList{element.id});
    notifyEdited();
    emit editEnded();
    setTool(Tool::Select);
    emit toolRequested(Tool::Select);
    update();
}

void CanvasWidget::cancelPen()
{
    if (m_penPoints.isEmpty()) {
        m_penPoints.clear();
        update();
        return;
    }
    m_penPoints.clear();
    m_penHover = QPointF();
    m_penNearStart = false;
    update();
}

// ---------------------------------------------------------------------------
// Double click, wheel, keyboard
// ---------------------------------------------------------------------------
void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_tool == Tool::Pen) {
        finishPen(false);
        return;
    }
    if (m_tool == Tool::Select && event->button() == Qt::LeftButton) {
        Element* hit = topElementAt(event->position());
        if (hit && !hit->locked) {
            setSelection(QStringList{hit->id});
            if (hit->type == ElementType::Text) {
                beginTextEditing(hit->id, false);
            } else {
                emit statusHint(QStringLiteral("双击文本可编辑内容；可在右侧属性面板修改样式"));
            }
        }
    }
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if (m_textEditor) commitTextEditing();
    const double steps = event->angleDelta().y() / 120.0;
    if (qAbs(steps) < 0.001) return;
    const double factor = std::pow(1.18, steps);
    applyZoom(m_zoom * factor, event->position());
    event->accept();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();

    if (key == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceDown = true;
        if (m_dragMode == DragMode::None) setCursor(Qt::OpenHandCursor);
        return;
    }

    if (key == Qt::Key_Escape) {
        if (m_dragMode != DragMode::None) {
            finishDrag(true);
        } else if (!m_penPoints.isEmpty()) {
            cancelPen();
        } else {
            clearSelection();
        }
        return;
    }

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (!m_penPoints.isEmpty()) {
            finishPen(false);
            return;
        }
    }

    if (!m_penPoints.isEmpty() && key == Qt::Key_Backspace) {
        m_penPoints.removeLast();
        update();
        return;
    }

    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        deleteSelection();
        return;
    }

    if (key == Qt::Key_A && (mods & Qt::ControlModifier)) {
        selectAll();
        return;
    }

    if (key == Qt::Key_0 && (mods & Qt::ControlModifier)) {
        fitToView();
        return;
    }

    if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        zoomIn();
        return;
    }
    if (key == Qt::Key_Minus) {
        zoomOut();
        return;
    }

    if (key == Qt::Key_Left || key == Qt::Key_Right ||
        key == Qt::Key_Up || key == Qt::Key_Down) {
        const double step = (mods & Qt::ShiftModifier) ? 10.0 : 1.0;
        QPointF delta;
        if (key == Qt::Key_Left) delta.setX(-step);
        if (key == Qt::Key_Right) delta.setX(step);
        if (key == Qt::Key_Up) delta.setY(-step);
        if (key == Qt::Key_Down) delta.setY(step);
        nudgeSelection(delta);
        return;
    }

    if (!(mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
        switch (key) {
        case Qt::Key_V: emit toolRequested(Tool::Select); return;
        case Qt::Key_R: emit toolRequested(Tool::Rectangle); return;
        case Qt::Key_O: emit toolRequested(Tool::Ellipse); return;
        case Qt::Key_L: emit toolRequested(Tool::Line); return;
        case Qt::Key_P: emit toolRequested(Tool::Polygon); return;
        case Qt::Key_S: emit toolRequested(Tool::Star); return;
        case Qt::Key_N: emit toolRequested(Tool::Pen); return;
        case Qt::Key_T: emit toolRequested(Tool::Text); return;
        case Qt::Key_H: emit toolRequested(Tool::Hand); return;
        case Qt::Key_Z: emit toolRequested(Tool::Zoom); return;
        default: break;
        }
    }

    QWidget::keyPressEvent(event);
}

void CanvasWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceDown = false;
        updateCursor();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

// ---------------------------------------------------------------------------
// Cursor and resize
// ---------------------------------------------------------------------------
void CanvasWidget::updateCursor()
{
    if (m_dragMode == DragMode::Pan) {
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (m_spaceDown || m_tool == Tool::Hand) {
        setCursor(Qt::OpenHandCursor);
        return;
    }
    if (m_showRulers) {
        if (m_lastMouse.y() < m_rulerSize && m_lastMouse.x() >= m_rulerSize) {
            setCursor(Qt::SizeVerCursor);
            return;
        }
        if (m_lastMouse.x() < m_rulerSize && m_lastMouse.y() >= m_rulerSize) {
            setCursor(Qt::SizeHorCursor);
            return;
        }
    }
    if (m_tool == Tool::Select && m_showGuides) {
        const int guide = guideAt(m_lastMouse);
        if (guide >= 0 && m_document && guide < m_document->guides.size()) {
            setCursor(m_document->guides[guide].orientation == QLatin1String("horizontal")
                          ? Qt::SizeVerCursor : Qt::SizeHorCursor);
            return;
        }
    }
    if (m_tool == Tool::Text) {
        setCursor(Qt::IBeamCursor);
        return;
    }
    if (m_tool != Tool::Select) {
        setCursor(Qt::CrossCursor);
        return;
    }
    if (m_dragMode == DragMode::Move) {
        setCursor(Qt::SizeAllCursor);
        return;
    }
    if (m_selection.size() == 1) {
        if (hitRotationHandle(m_lastMouse)) {
            setCursor(Qt::CrossCursor);
            return;
        }
        const int handle = hitHandle(m_lastMouse);
        if (handle >= 0) {
            switch (handle) {
            case 0: case 4: setCursor(Qt::SizeFDiagCursor); break;
            case 2: case 6: setCursor(Qt::SizeBDiagCursor); break;
            case 1: case 5: setCursor(Qt::SizeVerCursor); break;
            default: setCursor(Qt::SizeHorCursor); break;
            }
            return;
        }
    }
    Element* hover = topElementAt(m_lastMouse);
    if (hover) {
        setCursor(hover->locked ? Qt::ArrowCursor : Qt::SizeAllCursor);
        return;
    }
    setCursor(Qt::ArrowCursor);
}

// ---------------------------------------------------------------------------
// Text editing
// ---------------------------------------------------------------------------
void CanvasWidget::beginTextEditing(const QString& id, bool isNew)
{
    if (!m_document) return;
    if (m_textEditor) commitTextEditing();
    Element* element = m_document->findElement(id);
    if (!element) {
        if (isNew) emit editEnded();
        return;
    }
    if (!isNew) emit editBegan();

    m_textEditId = id;
    m_textEditOriginal = element->text;
    m_textEditIsNew = isNew;
    m_textEditCanceled = false;

    m_textEditor = new QTextEdit(this);
    m_textEditor->setObjectName(QStringLiteral("CanvasTextEditor"));
    m_textEditor->setFrameShape(QFrame::NoFrame);
    m_textEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEditor->setAcceptRichText(false);
    m_textEditor->setPlainText(element->text);
    m_textEditor->setTabChangesFocus(false);
    m_textEditor->installEventFilter(this);
    connect(m_textEditor, &QTextEdit::textChanged, this, [this]() {
        if (!m_textEditor || !m_document) return;
        Element* e = m_document->findElement(m_textEditId);
        if (!e) return;
        e->text = m_textEditor->toPlainText();
        notifyEdited();
    });

    QFont editorFont(element->fontFamily);
    editorFont.setPixelSize(qMax(8, qRound(element->fontSize * m_zoom)));
    editorFont.setBold(element->bold);
    m_textEditor->setFont(editorFont);
    m_textEditor->document()->setDefaultFont(editorFont);

    QTextCursor cursor(m_textEditor->document());
    cursor.select(QTextCursor::Document);
    QTextBlockFormat format;
    if (element->align == QLatin1String("left")) format.setAlignment(Qt::AlignLeft);
    else if (element->align == QLatin1String("right")) format.setAlignment(Qt::AlignRight);
    else format.setAlignment(Qt::AlignHCenter);
    cursor.mergeBlockFormat(format);
    cursor.clearSelection();
    m_textEditor->setTextCursor(cursor);

    m_textEditor->setStyleSheet(QStringLiteral(
        "QTextEdit { background: rgba(8, 11, 18, 248); color: %1;"
        " border: 1px solid #6C8CFF; border-radius: 2px; padding: 3px;"
        " selection-background-color: #3B5BDB; }")
        .arg(colorToString(element->textColor)));

    updateEditorGeometry();
    m_textEditor->show();
    m_textEditor->setFocus(Qt::MouseFocusReason);
    m_textEditor->selectAll();
    emit statusHint(QStringLiteral("编辑文本：Esc 取消 · Ctrl+Enter 确认 · 点击空白处完成"));
    update();
}

void CanvasWidget::updateEditorGeometry()
{
    if (!m_textEditor || !m_document) return;
    const Element* element = m_document->findElement(m_textEditId);
    if (!element) return;
    const QRectF screenBox = docToScreen(element->rotatedCorners().boundingRect())
                                 .adjusted(-3, -3, 3, 3);
    QRectF geometry = screenBox;
    geometry.setWidth(qMax(geometry.width(), 60.0));
    geometry.setHeight(qMax(geometry.height(), 30.0));
    m_textEditor->setGeometry(geometry.toRect());
    QFont editorFont = m_textEditor->font();
    editorFont.setPixelSize(qMax(8, qRound(element->fontSize * m_zoom)));
    m_textEditor->setFont(editorFont);
    m_textEditor->document()->setDefaultFont(editorFont);
}

void CanvasWidget::commitTextEditing(bool cancel)
{
    if (!m_textEditor || m_commitInProgress) return;
    m_commitInProgress = true;
    QTextEdit* editor = m_textEditor;
    const QString id = m_textEditId;
    const bool isNew = m_textEditIsNew;
    const QString original = m_textEditOriginal;
    const QString text = editor->toPlainText();

    m_textEditor = nullptr;
    m_textEditId.clear();
    m_textEditIsNew = false;
    m_textEditCanceled = false;
    m_commitInProgress = false;
    editor->removeEventFilter(this);
    editor->hide();
    editor->deleteLater();

    Element* element = m_document ? m_document->findElement(id) : nullptr;
    if (element) {
        if (cancel) {
            if (isNew) {
                m_document->removeElement(id);
                if (m_selection.contains(id)) {
                    QStringList selection = m_selection;
                    selection.removeAll(id);
                    setSelection(selection);
                }
            } else {
                element->text = original;
            }
        } else {
            if (isNew && text.trimmed().isEmpty()) {
                m_document->removeElement(id);
                if (m_selection.contains(id)) {
                    QStringList selection = m_selection;
                    selection.removeAll(id);
                    setSelection(selection);
                }
            } else {
                element->text = text;
            }
        }
    }
    notifyEdited();
    emit editEnded();
    update();
}

bool CanvasWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_textEditor) {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                commitTextEditing(true);
                return true;
            }
            if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) &&
                (keyEvent->modifiers() & Qt::ControlModifier)) {
                commitTextEditing(false);
                return true;
            }
        } else if (event->type() == QEvent::FocusOut) {
            QTimer::singleShot(0, this, [this]() {
                if (m_textEditor) commitTextEditing(false);
            });
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------
// Commands from the main window
// ---------------------------------------------------------------------------
void CanvasWidget::deleteSelection()
{
    if (!m_document || m_selection.isEmpty()) return;
    emit editBegan();
    const QStringList removed = m_selection;
    m_document->removeElements(removed);
    clearSelection();
    notifyEdited();
    emit editEnded();
    update();
}

void CanvasWidget::nudgeSelection(const QPointF& delta)
{
    if (!m_document || m_selection.isEmpty()) return;
    emit editBegan();
    bool changed = false;
    for (const auto& id : m_selection) {
        Element* element = m_document->findElement(id);
        if (!element || element->locked) continue;
        element->translateBy(delta);
        changed = true;
    }
    if (!changed) {
        emit editEnded();
        return;
    }
    notifyEdited();
    emit editEnded();
}

// ---------------------------------------------------------------------------
// Resize / leave
// ---------------------------------------------------------------------------
void CanvasWidget::resizeEvent(QResizeEvent* event)
{
    if (m_pendingFit || m_lastSize.isEmpty()) {
        m_pendingFit = true;
        m_lastSize = event->size();
        fitToView();
    } else {
        const QPointF shift((event->size().width() - m_lastSize.width()) / 2.0,
                            (event->size().height() - m_lastSize.height()) / 2.0);
        m_origin += shift;
        m_lastSize = event->size();
        updateEditorGeometry();
    }
    QWidget::resizeEvent(event);
}

void CanvasWidget::leaveEvent(QEvent* event)
{
    m_hoverId.clear();
    m_hasLastMouse = false;
    update();
    QWidget::leaveEvent(event);
}

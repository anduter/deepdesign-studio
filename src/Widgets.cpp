#include "Widgets.h"
#include "Theme.h"

#include <QColorDialog>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QtMath>
#include <algorithm>

ColorButton::ColorButton(QWidget* parent)
    : QToolButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setToolTip(QStringLiteral("点击选择颜色"));
    setAutoRaise(true);
    connect(this, &QToolButton::clicked, this, &ColorButton::chooseColor);
}

void ColorButton::setColor(const QColor& color)
{
    if (!color.isValid() || color == m_color) return;
    m_color = color;
    setToolTip(QStringLiteral("颜色：%1").arg(color.name(QColor::HexRgb).toUpper()));
    update();
}

void ColorButton::setCompact(bool compact)
{
    m_compact = compact;
    updateGeometry();
    update();
}

void ColorButton::setTextVisible(bool visible)
{
    m_textVisible = visible;
    updateGeometry();
    update();
}

QSize ColorButton::sizeHint() const
{
    if (m_compact) return QSize(28, 22);
    return QSize(m_textVisible ? 96 : 28, 24);
}

void ColorButton::chooseColor()
{
    const QColor selected = QColorDialog::getColor(m_color, this, QStringLiteral("选择颜色"),
                                                   QColorDialog::DontUseNativeDialog);
    if (!selected.isValid()) return;
    if (selected == m_color) return;
    setColor(selected);
    ColorSystem::addRecentColor(selected);
    emit colorChanged(m_color);
}

void ColorButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QRectF swatch = rect().adjusted(1.5, 1.5, -1.5, -1.5);
    QColor border = underMouse() ? QColor("#8A8A8A") : QColor("#5A5A5A");
    p.setPen(QPen(border, 1.0));
    p.setBrush(m_color);
    p.drawRoundedRect(swatch, 3, 3);

    if (!m_compact && m_textVisible && width() > 64) {
        const QString hex = m_color.name(QColor::HexRgb).toUpper();
        const int luminance = (m_color.red() * 299 + m_color.green() * 587 + m_color.blue() * 114) / 1000;
        QRectF textRect(swatch.left() + 22, swatch.top(), swatch.width() - 24, swatch.height());
        p.setPen(luminance > 140 ? QColor("#1E1E1E") : QColor("#F0F0F0"));
        QFont font = p.font();
        font.setPointSize(8);
        p.setFont(font);
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, hex);
    }
}

// ---------------------------------------------------------------------------
// Color system
// ---------------------------------------------------------------------------
namespace {
QVector<QColor>& recentColorStore()
{
    static QVector<QColor> colors;
    return colors;
}
} // namespace

QVector<QColor> ColorSystem::presetColors()
{
    return {
        QColor("#FFFFFF"), QColor("#D6D6D6"), QColor("#9A9A9A"), QColor("#5A5A5A"),
        QColor("#1E1E1E"), QColor("#0A0F1A"), QColor("#0F172A"), QColor("#1E293B"),
        QColor("#2D8CEB"), QColor("#5AA9F0"), QColor("#3DDC97"), QColor("#06D6A0"),
        QColor("#A47BFF"), QColor("#C77DFF"), QColor("#FF6B81"), QColor("#EF476F"),
        QColor("#FFB454"), QColor("#FFD166"), QColor("#F4A261"), QColor("#8D99AE"),
    };
}

QVector<QColor> ColorSystem::recentColors()
{
    return recentColorStore();
}

void ColorSystem::addRecentColor(const QColor& color)
{
    if (!color.isValid()) return;
    QVector<QColor>& recent = recentColorStore();
    recent.removeAll(color);
    recent.prepend(color);
    while (recent.size() > 10) recent.removeLast();
}

ColorPaletteWidget::ColorPaletteWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(68);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(QStringLiteral("调色板：点击应用到当前填充 / 渐变停止点"));
}

void ColorPaletteWidget::addRecent(const QColor& color)
{
    ColorSystem::addRecentColor(color);
    update();
}

QVector<QColor> ColorPaletteWidget::allColors() const
{
    QVector<QColor> colors = ColorSystem::presetColors();
    const QVector<QColor> recent = ColorSystem::recentColors();
    for (const auto& color : recent) {
        if (!colors.contains(color)) colors.append(color);
    }
    return colors;
}

int ColorPaletteWidget::indexAt(const QPoint& point) const
{
    constexpr int columns = 10;
    constexpr int cellW = 22;
    constexpr int cellH = 21;
    const int column = point.x() / cellW;
    const int row = point.y() / cellH;
    if (column < 0 || column >= columns || row < 0) return -1;
    const int index = row * columns + column;
    return index < allColors().size() ? index : -1;
}

void ColorPaletteWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QVector<QColor> colors = allColors();
    constexpr int columns = 10;
    constexpr int cellW = 22;
    constexpr int cellH = 21;
    for (int i = 0; i < colors.size(); ++i) {
        const int row = i / columns;
        const int column = i % columns;
        const QRectF swatch(column * cellW + 2, row * cellH + 2, 18, 16);
        if (swatch.bottom() > height()) break;
        p.setPen(QPen(i == m_hover ? QColor("#FFFFFF") : QColor(255, 255, 255, 32),
                      i == m_hover ? 1.5 : 1.0));
        p.setBrush(colors[i]);
        p.drawRoundedRect(swatch, 3, 3);
    }
}

void ColorPaletteWidget::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (index != m_hover) {
        m_hover = index;
        update();
    }
}

void ColorPaletteWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const int index = indexAt(event->position().toPoint());
    if (index < 0) return;
    const QColor color = allColors().value(index);
    ColorSystem::addRecentColor(color);
    if (colorPicked) colorPicked(color);
    update();
}

void ColorPaletteWidget::leaveEvent(QEvent* event)
{
    m_hover = -1;
    update();
    QWidget::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// GradientEditor
// ---------------------------------------------------------------------------
GradientEditor::GradientEditor(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(QStringLiteral("拖动色标调整位置，双击色标改色，双击色条添加色标"));
    m_gradient.enabled = false;
    m_gradient.stops = {{0.0, QColor("#2D8CEB")}, {1.0, QColor("#A47BFF")}};
}

void GradientEditor::setGradient(const GradientFill& gradient)
{
    m_gradient = gradient;
    m_gradient.sortStops();
    if (m_gradient.stops.isEmpty()) {
        m_gradient.stops = {{0.0, QColor("#2D8CEB")}, {1.0, QColor("#A47BFF")}};
    }
    m_selectedStop = qBound(0, m_selectedStop, m_gradient.stops.size() - 1);
    update();
}

void GradientEditor::setSelectedStop(int index)
{
    if (m_gradient.stops.isEmpty()) return;
    const int clamped = qBound(0, index, m_gradient.stops.size() - 1);
    if (clamped == m_selectedStop) return;
    m_selectedStop = clamped;
    update();
    emit selectedStopChanged(m_selectedStop);
}

void GradientEditor::addStop()
{
    if (m_gradient.stops.isEmpty()) return;
    QVector<GradientStop> sorted = m_gradient.stops;
    std::sort(sorted.begin(), sorted.end(), [](const GradientStop& a, const GradientStop& b) {
        return a.position < b.position;
    });
    double bestGap = -1.0;
    double bestPosition = 0.5;
    double previous = 0.0;
    for (const auto& stop : sorted) {
        if (stop.position - previous > bestGap) {
            bestGap = stop.position - previous;
            bestPosition = (previous + stop.position) / 2.0;
        }
        previous = stop.position;
    }
    if (1.0 - previous > bestGap) {
        bestPosition = (previous + 1.0) / 2.0;
    }
    GradientStop stop;
    stop.position = qBound(0.0, bestPosition, 1.0);
    stop.color = m_gradient.colorAt(stop.position);
    emit editBegan();
    m_gradient.stops.append(stop);
    m_gradient.sortStops();
    for (int i = 0; i < m_gradient.stops.size(); ++i) {
        if (qAbs(m_gradient.stops[i].position - stop.position) < 0.0001) {
            m_selectedStop = i;
            break;
        }
    }
    update();
    emit gradientChanged(m_gradient);
    emit selectedStopChanged(m_selectedStop);
    emit editEnded();
}

void GradientEditor::removeStop()
{
    if (m_gradient.stops.size() <= 2) return;
    if (m_selectedStop < 0 || m_selectedStop >= m_gradient.stops.size()) return;
    emit editBegan();
    m_gradient.stops.removeAt(m_selectedStop);
    m_gradient.sortStops();
    m_selectedStop = qBound(0, m_selectedStop, m_gradient.stops.size() - 1);
    update();
    emit gradientChanged(m_gradient);
    emit selectedStopChanged(m_selectedStop);
    emit editEnded();
}

int GradientEditor::stopAt(const QPoint& point) const
{
    for (int i = 0; i < m_gradient.stops.size(); ++i) {
        const double x = xForPosition(m_gradient.stops[i].position);
        if (qAbs(point.x() - x) <= 9.0 && qAbs(point.y() - markerY()) <= 12.0) return i;
    }
    return -1;
}

double GradientEditor::positionForX(double x) const
{
    const double left = 8.0;
    const double right = width() - 8.0;
    if (right <= left) return 0.0;
    return qBound(0.0, (x - left) / (right - left), 1.0);
}

double GradientEditor::xForPosition(double position) const
{
    return 8.0 + qBound(0.0, position, 1.0) * (width() - 16.0);
}

void GradientEditor::openColorDialog(int index)
{
    if (index < 0 || index >= m_gradient.stops.size()) return;
    const QColor selected = QColorDialog::getColor(m_gradient.stops[index].color, this,
                                                   QStringLiteral("渐变色标颜色"),
                                                   QColorDialog::DontUseNativeDialog);
    if (!selected.isValid()) return;
    emit editBegan();
    m_gradient.stops[index].color = selected;
    ColorSystem::addRecentColor(selected);
    update();
    emit gradientChanged(m_gradient);
    emit editEnded();
}

void GradientEditor::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF bar(8, barTop(), width() - 16, barHeight());
    QVector<GradientStop> sorted = m_gradient.stops;
    std::sort(sorted.begin(), sorted.end(), [](const GradientStop& a, const GradientStop& b) {
        return a.position < b.position;
    });
    QLinearGradient linear(bar.topLeft(), bar.topRight());
    if (sorted.size() >= 2) {
        for (const auto& stop : sorted) {
            linear.setColorAt(qBound(0.0, stop.position, 1.0), stop.color);
        }
    } else {
        linear.setColorAt(0.0, QColor("#333333"));
        linear.setColorAt(1.0, QColor("#333333"));
    }
    p.setPen(QPen(QColor("#5A5A5A"), 1.0));
    p.setBrush(linear);
    p.drawRoundedRect(bar, 5, 5);

    p.setPen(QPen(QColor("#2A2A2A"), 1.0));
    for (int i = 0; i < m_gradient.stops.size(); ++i) {
        const QPointF center(xForPosition(m_gradient.stops[i].position), markerY());
        p.setPen(QPen(i == m_selectedStop ? QColor("#FFFFFF") : QColor("#6A6A6A"),
                      i == m_selectedStop ? 2.0 : 1.2));
        p.setBrush(m_gradient.stops[i].color);
        p.drawEllipse(center, 7.0, 7.0);
        p.setPen(QPen(i == m_selectedStop ? QColor("#FFFFFF") : QColor("#555555"), 1.0));
        p.drawLine(QPointF(center.x(), markerY() - 8), QPointF(center.x(), bar.bottom() + 1));
    }

    p.setPen(QColor("#7E7E7E"));
    QFont small = p.font();
    small.setPixelSize(9);
    p.setFont(small);
    p.drawText(QRectF(8, 53, 30, 12), Qt::AlignLeft, QStringLiteral("0%"));
    p.drawText(QRectF(width() - 38, 53, 30, 12), Qt::AlignRight, QStringLiteral("100%"));
}

void GradientEditor::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const int index = stopAt(event->position().toPoint());
    if (index < 0) return;
    m_selectedStop = index;
    m_dragStop = index;
    m_dragging = true;
    emit selectedStopChanged(m_selectedStop);
    emit editBegan();
    setCursor(Qt::SizeHorCursor);
    update();
}

void GradientEditor::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_dragStop >= 0 && m_dragStop < m_gradient.stops.size()) {
        const double position = positionForX(event->position().x());
        if (qAbs(position - m_gradient.stops[m_dragStop].position) > 0.0001) {
            m_gradient.stops[m_dragStop].position = position;
            emit gradientChanged(m_gradient);
            update();
        }
        return;
    }
    const int hover = stopAt(event->position().toPoint());
    if (hover != m_hoverStop) {
        m_hoverStop = hover;
        update();
    }
}

void GradientEditor::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (!m_dragging) return;
    m_dragging = false;
    m_dragStop = -1;
    setCursor(Qt::ArrowCursor);
    m_gradient.sortStops();
    m_selectedStop = qBound(0, m_selectedStop, m_gradient.stops.size() - 1);
    update();
    emit gradientChanged(m_gradient);
    emit selectedStopChanged(m_selectedStop);
    emit editEnded();
}

void GradientEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const int index = stopAt(event->position().toPoint());
    if (index >= 0) {
        openColorDialog(index);
        return;
    }
    if (event->position().y() >= barTop() && event->position().y() <= barTop() + barHeight()) {
        const double position = positionForX(event->position().x());
        GradientStop stop;
        stop.position = position;
        stop.color = m_gradient.colorAt(position);
        emit editBegan();
        m_gradient.stops.append(stop);
        m_gradient.sortStops();
        for (int i = 0; i < m_gradient.stops.size(); ++i) {
            if (qAbs(m_gradient.stops[i].position - position) < 0.0001) {
                m_selectedStop = i;
                break;
            }
        }
        update();
        emit gradientChanged(m_gradient);
        emit selectedStopChanged(m_selectedStop);
        emit editEnded();
    }
}

void GradientEditor::leaveEvent(QEvent* event)
{
    m_hoverStop = -1;
    update();
    QWidget::leaveEvent(event);
}


#include "ToolPalette.h"
#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QMouseEvent>
#include <QLabel>
#include <QPainter>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

namespace {

class ColorPairWidget : public QWidget {
public:
    explicit ColorPairWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setFixedSize(46, 46);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("默认填充颜色 / 点击切换颜色"));
    }

    void setForeground(const QColor& color)
    {
        m_foreground = color;
        update();
    }

    std::function<void(const QColor&)> foregroundChanged;

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        Q_UNUSED(event);
        const QColor selected = QColorDialog::getColor(m_foreground, this,
                                                       QStringLiteral("默认填充颜色"),
                                                       QColorDialog::DontUseNativeDialog);
        if (!selected.isValid() || selected == m_foreground) return;
        m_foreground = selected;
        update();
        if (foregroundChanged) foregroundChanged(m_foreground);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRectF back(16, 16, 25, 25);
        QRectF front(4, 4, 25, 25);
        p.setPen(QPen(QColor("#6A6A6A"), 1.0));
        p.setBrush(QColor("#FFFFFF"));
        p.drawRoundedRect(back, 2, 2);
        p.setPen(QPen(QColor("#9A9A9A"), 1.0));
        p.setBrush(m_foreground);
        p.drawRoundedRect(front, 2, 2);

        p.setPen(QPen(QColor("#B0B0B0"), 1.0));
        p.drawLine(QPointF(36, 8), QPointF(41, 8));
        p.drawLine(QPointF(41, 8), QPointF(38, 5));
        p.drawLine(QPointF(41, 8), QPointF(38, 11));
    }

private:
    QColor m_foreground = QColor("#2D8CEB");
};

} // namespace

ToolPalette::ToolPalette(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(54);
    setStyleSheet(QStringLiteral(
        "QToolButton { border: none; border-radius: 3px; background: transparent; }"
        "QToolButton:hover { background: #3A3A3A; }"
        "QToolButton:checked { background: #454545; border-left: 2px solid #2D8CEB; }"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 8, 5, 8);
    layout->setSpacing(3);

    auto* logo = new QLabel(this);
    logo->setFixedSize(44, 44);
    logo->setAlignment(Qt::AlignCenter);
    QPixmap logoPixmap(88, 88);
    logoPixmap.setDevicePixelRatio(2.0);
    logoPixmap.fill(Qt::transparent);
    {
        QPainter painter(&logoPixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        Icons::drawLogo(painter, QRectF(4, 4, 36, 36), false);
    }
    logo->setPixmap(logoPixmap);
    logo->setToolTip(QStringLiteral("灵绘 DeepDesign Studio"));
    layout->addWidget(logo, 0, Qt::AlignHCenter);

    auto addSeparator = [&]() {
        auto* line = new QWidget(this);
        line->setFixedHeight(1);
        line->setStyleSheet(QStringLiteral("background: #3C3C3C; margin: 3px 5px;"));
        layout->addWidget(line);
    };

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto addTool = [this, layout](Tool tool, const QString& iconName,
                                 const QString& label, const QString& shortcut) {
        auto* button = createToolButton(tool, iconName, label, shortcut);
        layout->addWidget(button);
        m_buttons.insert(tool, button);
    };

    addTool(Tool::Select, QStringLiteral("select"), QStringLiteral("选择工具"), QStringLiteral("V"));
    addSeparator();
    addTool(Tool::Rectangle, QStringLiteral("rect"), QStringLiteral("矩形工具"), QStringLiteral("R"));
    addTool(Tool::Ellipse, QStringLiteral("ellipse"), QStringLiteral("椭圆工具"), QStringLiteral("O"));
    addTool(Tool::Line, QStringLiteral("line"), QStringLiteral("直线工具"), QStringLiteral("L"));
    addTool(Tool::Polygon, QStringLiteral("polygon"), QStringLiteral("多边形工具"), QStringLiteral("P"));
    addTool(Tool::Star, QStringLiteral("star"), QStringLiteral("星形工具"), QStringLiteral("S"));
    addTool(Tool::Pen, QStringLiteral("pen"), QStringLiteral("钢笔 / 自由路径"), QStringLiteral("N"));
    addSeparator();
    addTool(Tool::Text, QStringLiteral("text"), QStringLiteral("文字工具"), QStringLiteral("T"));
    addSeparator();
    addTool(Tool::Hand, QStringLiteral("hand"), QStringLiteral("抓手工具"), QStringLiteral("H"));
    addTool(Tool::Zoom, QStringLiteral("zoom"), QStringLiteral("缩放工具"), QStringLiteral("Z"));

    layout->addStretch(1);

    auto* colors = new ColorPairWidget(this);
    colors->setForeground(m_defaultFill);
    colors->foregroundChanged = [this](const QColor& color) {
        m_defaultFill = color;
        emit defaultFillChanged(color);
    };
    m_colorSwatch = colors;
    layout->addWidget(colors, 0, Qt::AlignHCenter);

    setTool(Tool::Select, false);
    updateButtonIcons();
}

QToolButton* ToolPalette::createToolButton(Tool tool, const QString& iconName,
                                           const QString& label, const QString& shortcut)
{
    auto* button = new QToolButton(this);
    button->setCheckable(true);
    button->setFixedSize(44, 40);
    button->setIconSize(QSize(21, 21));
    button->setToolTip(QStringLiteral("%1  (%2)").arg(label, shortcut));
    button->setProperty("iconName", iconName);
    button->setProperty("toolId", static_cast<int>(tool));
    m_group->addButton(button);
    connect(button, &QToolButton::clicked, this, [this, tool]() {
        setTool(tool, true);
    });
    return button;
}

void ToolPalette::setTool(Tool tool, bool emitSignal)
{
    if (!m_buttons.contains(tool)) return;
    m_currentTool = tool;
    QAbstractButton* button = m_buttons.value(tool);
    if (button && !button->isChecked()) {
        const QSignalBlocker blocker(button);
        button->setChecked(true);
    }
    updateButtonIcons();
    if (emitSignal) emit toolChanged(tool);
}

void ToolPalette::setDefaultFillColor(const QColor& color)
{
    if (!color.isValid()) return;
    m_defaultFill = color;
    auto* swatch = static_cast<ColorPairWidget*>(m_colorSwatch);
    if (swatch) swatch->setForeground(color);
}

void ToolPalette::updateButtonIcons()
{
    for (auto it = m_buttons.constBegin(); it != m_buttons.constEnd(); ++it) {
        QToolButton* button = it.value();
        if (!button) continue;
        const QString iconName = button->property("iconName").toString();
        const bool checked = (it.key() == m_currentTool);
        const QColor color = checked ? QColor("#FFFFFF") : QColor("#C3C3C3");
        button->setIcon(Icons::icon(iconName, color, 21));
    }
}

#include "OptionsBar.h"
#include "Animation.h"
#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QtMath>

namespace {
const char* toolIconName(Tool tool)
{
    switch (tool) {
    case Tool::Select: return "select";
    case Tool::Rectangle: return "rect";
    case Tool::Ellipse: return "ellipse";
    case Tool::Line: return "line";
    case Tool::Polygon: return "polygon";
    case Tool::Star: return "star";
    case Tool::Pen: return "pen";
    case Tool::Text: return "text";
    case Tool::Hand: return "hand";
    case Tool::Zoom: return "zoom";
    }
    return "select";
}

QString toolDisplayName(Tool tool)
{
    switch (tool) {
    case Tool::Select: return QStringLiteral("移动 / 选择");
    case Tool::Rectangle: return QStringLiteral("矩形");
    case Tool::Ellipse: return QStringLiteral("椭圆");
    case Tool::Line: return QStringLiteral("直线");
    case Tool::Polygon: return QStringLiteral("多边形");
    case Tool::Star: return QStringLiteral("星形");
    case Tool::Pen: return QStringLiteral("钢笔");
    case Tool::Text: return QStringLiteral("文字");
    case Tool::Hand: return QStringLiteral("抓手");
    case Tool::Zoom: return QStringLiteral("缩放");
    }
    return QStringLiteral("工具");
}

bool isShapeTool(Tool tool)
{
    return tool == Tool::Rectangle || tool == Tool::Ellipse || tool == Tool::Line ||
           tool == Tool::Polygon || tool == Tool::Star;
}

QLabel* fieldLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("FieldLabel"));
    return label;
}

QFrame* separator(QWidget* parent)
{
    auto* line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);
    line->setStyleSheet(QStringLiteral("background: #4A4A4A; max-width: 1px; min-width: 1px;"));
    line->setFixedHeight(22);
    return line;
}

QToolButton* toolButton(const QString& icon, const QString& tooltip, QWidget* parent)
{
    auto* button = new QToolButton(parent);
    button->setIcon(Icons::icon(icon, QColor("#C8C8C8"), 18));
    button->setIconSize(QSize(18, 18));
    button->setFixedSize(26, 24);
    button->setToolTip(tooltip);
    button->setCheckable(false);
    return button;
}
} // namespace

OptionsBar::OptionsBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("OptionsBar"));
    setFixedHeight(44);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_editTimer = new QTimer(this);
    m_editTimer->setSingleShot(true);
    connect(m_editTimer, &QTimer::timeout, this, &OptionsBar::finishPendingEditNow);

    buildUi();
    setTool(Tool::Select);
}

void OptionsBar::buildUi()
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(10, 6, 10, 6);
    root->setSpacing(9);

    m_toolIcon = new QLabel(this);
    m_toolIcon->setFixedSize(22, 22);
    m_toolIcon->setAlignment(Qt::AlignCenter);
    root->addWidget(m_toolIcon);

    m_toolName = new QLabel(this);
    m_toolName->setMinimumWidth(74);
    QFont boldFont = m_toolName->font();
    boldFont.setBold(true);
    m_toolName->setFont(boldFont);
    root->addWidget(m_toolName);

    root->addWidget(separator(this));

    // ---- Selection / alignment group -------------------------------------
    m_selectGroup = new QWidget(this);
    {
        auto* layout = new QHBoxLayout(m_selectGroup);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        layout->addWidget(fieldLabel(QStringLiteral("对齐"), m_selectGroup));
        const QList<QPair<QString, QString>> alignments = {
            {"alignLeft", QStringLiteral("左对齐")},
            {"alignCenter", QStringLiteral("水平居中对齐")},
            {"alignRight", QStringLiteral("右对齐")},
            {"alignLeft", QStringLiteral("顶对齐")},
            {"alignCenter", QStringLiteral("垂直居中对齐")},
            {"alignRight", QStringLiteral("底对齐")},
        };
        const QStringList modes = {"left", "hcenter", "right", "top", "vcenter", "bottom"};
        for (int i = 0; i < modes.size(); ++i) {
            auto* button = toolButton(alignments[i].first, alignments[i].second, m_selectGroup);
            if (i >= 3) button->setIcon(Icons::icon(QStringLiteral("alignLeft"),
                                                    QColor("#C8C8C8"), 18));
            connect(button, &QToolButton::clicked, this, [this, mode = modes[i]]() {
                emit alignRequested(mode);
            });
            layout->addWidget(button);
        }
        layout->addSpacing(4);
        auto* distribute = toolButton(QStringLiteral("alignCenter"),
                                      QStringLiteral("水平等距分布"), m_selectGroup);
        connect(distribute, &QToolButton::clicked, this, [this]() {
            emit alignRequested(QStringLiteral("distributeH"));
        });
        layout->addWidget(distribute);
        auto* distributeV = toolButton(QStringLiteral("alignCenter"),
                                       QStringLiteral("垂直等距分布"), m_selectGroup);
        connect(distributeV, &QToolButton::clicked, this, [this]() {
            emit alignRequested(QStringLiteral("distributeV"));
        });
        layout->addWidget(distributeV);
    }
    root->addWidget(m_selectGroup);

    // ---- Shape group -----------------------------------------------------
    m_shapeGroup = new QWidget(this);
    {
        auto* layout = new QHBoxLayout(m_shapeGroup);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);

        m_fillLabel = fieldLabel(QStringLiteral("填充"), m_shapeGroup);
        layout->addWidget(m_fillLabel);
        m_fillButton = new ColorButton(m_shapeGroup);
        m_fillButton->setCompact(true);
        m_fillButton->setFixedSize(28, 22);
        layout->addWidget(m_fillButton);

        m_strokeLabel = fieldLabel(QStringLiteral("描边"), m_shapeGroup);
        layout->addWidget(m_strokeLabel);
        m_strokeButton = new ColorButton(m_shapeGroup);
        m_strokeButton->setCompact(true);
        m_strokeButton->setFixedSize(28, 22);
        layout->addWidget(m_strokeButton);

        m_strokeWidthLabel = fieldLabel(QStringLiteral("粗细"), m_shapeGroup);
        layout->addWidget(m_strokeWidthLabel);
        m_strokeWidthSpin = new QDoubleSpinBox(m_shapeGroup);
        m_strokeWidthSpin->setRange(0, 100);
        m_strokeWidthSpin->setDecimals(1);
        m_strokeWidthSpin->setSingleStep(0.5);
        m_strokeWidthSpin->setFixedWidth(62);
        layout->addWidget(m_strokeWidthSpin);

        m_radiusLabel = fieldLabel(QStringLiteral("圆角"), m_shapeGroup);
        layout->addWidget(m_radiusLabel);
        m_radiusSpin = new QDoubleSpinBox(m_shapeGroup);
        m_radiusSpin->setRange(0, 500);
        m_radiusSpin->setDecimals(1);
        m_radiusSpin->setFixedWidth(62);
        layout->addWidget(m_radiusSpin);

        m_sidesLabel = fieldLabel(QStringLiteral("边数"), m_shapeGroup);
        layout->addWidget(m_sidesLabel);
        m_sidesSpin = new QDoubleSpinBox(m_shapeGroup);
        m_sidesSpin->setRange(3, 60);
        m_sidesSpin->setDecimals(0);
        m_sidesSpin->setFixedWidth(56);
        layout->addWidget(m_sidesSpin);
    }
    root->addWidget(m_shapeGroup);

    // ---- Text group ------------------------------------------------------
    m_textGroup = new QWidget(this);
    {
        auto* layout = new QHBoxLayout(m_textGroup);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        m_fontLabel = fieldLabel(QStringLiteral("字体"), m_textGroup);
        layout->addWidget(m_fontLabel);
        m_fontCombo = new QFontComboBox(m_textGroup);
        m_fontCombo->setMinimumWidth(150);
        m_fontCombo->setMaximumWidth(180);
        layout->addWidget(m_fontCombo);

        m_fontSizeLabel = fieldLabel(QStringLiteral("字号"), m_textGroup);
        layout->addWidget(m_fontSizeLabel);
        m_fontSizeSpin = new QDoubleSpinBox(m_textGroup);
        m_fontSizeSpin->setRange(4, 400);
        m_fontSizeSpin->setDecimals(0);
        m_fontSizeSpin->setFixedWidth(58);
        m_fontSizeSpin->setSuffix(QStringLiteral(" px"));
        layout->addWidget(m_fontSizeSpin);

        m_boldButton = new QToolButton(m_textGroup);
        m_boldButton->setText(QStringLiteral("B"));
        m_boldButton->setCheckable(true);
        QFont bf = m_boldButton->font();
        bf.setBold(true);
        m_boldButton->setFont(bf);
        m_boldButton->setFixedSize(26, 24);
        m_boldButton->setToolTip(QStringLiteral("粗体"));
        layout->addWidget(m_boldButton);

        m_textColorButton = new ColorButton(m_textGroup);
        m_textColorButton->setCompact(true);
        m_textColorButton->setFixedSize(28, 22);
        layout->addWidget(m_textColorButton);

        QButtonGroup* alignGroup = new QButtonGroup(m_textGroup);
        alignGroup->setExclusive(true);
        const QList<QPair<QString, QString>> textAligns = {
            {"alignLeft", "left"}, {"alignCenter", "center"}, {"alignRight", "right"}};
        for (const auto& entry : textAligns) {
            auto* button = toolButton(entry.first, QStringLiteral("文本对齐"), m_textGroup);
            button->setCheckable(true);
            connect(button, &QToolButton::clicked, this, [this, align = entry.second]() {
                applyToPrimary([align](Element& element) { element.align = align; }, true);
                refresh();
            });
            alignGroup->addButton(button);
            layout->addWidget(button);
        }
    }
    root->addWidget(m_textGroup);

    // ---- Path group ------------------------------------------------------
    m_pathGroup = new QWidget(this);
    {
        auto* layout = new QHBoxLayout(m_pathGroup);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
        layout->addWidget(fieldLabel(QStringLiteral("自由路径"), m_pathGroup));
        m_closedCheck = new QCheckBox(QStringLiteral("闭合路径"), m_pathGroup);
        layout->addWidget(m_closedCheck);
        auto* hint = new QLabel(QStringLiteral("双击画布或按 Enter 完成路径"), m_pathGroup);
        hint->setObjectName(QStringLiteral("FieldLabel"));
        layout->addWidget(hint);
    }
    root->addWidget(m_pathGroup);

    // ---- Zoom group ------------------------------------------------------
    m_zoomGroup = new QWidget(this);
    {
        auto* layout = new QHBoxLayout(m_zoomGroup);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(5);
        auto* out = toolButton(QStringLiteral("minus"), QStringLiteral("缩小"), m_zoomGroup);
        connect(out, &QToolButton::clicked, this, [this]() {
            if (m_canvas) m_canvas->zoomOut();
        });
        layout->addWidget(out);
        m_zoomLabel = new QLabel(QStringLiteral("100%"), m_zoomGroup);
        m_zoomLabel->setAlignment(Qt::AlignCenter);
        m_zoomLabel->setFixedWidth(48);
        layout->addWidget(m_zoomLabel);
        auto* in = toolButton(QStringLiteral("plus"), QStringLiteral("放大"), m_zoomGroup);
        connect(in, &QToolButton::clicked, this, [this]() {
            if (m_canvas) m_canvas->zoomIn();
        });
        layout->addWidget(in);
        auto* fit = new QPushButton(QStringLiteral("适合屏幕"), m_zoomGroup);
        fit->setObjectName(QStringLiteral("GhostButton"));
        connect(fit, &QPushButton::clicked, this, [this]() {
            if (m_canvas) m_canvas->fitToView();
        });
        layout->addWidget(fit);
        auto* actual = new QPushButton(QStringLiteral("100%"), m_zoomGroup);
        actual->setObjectName(QStringLiteral("GhostButton"));
        connect(actual, &QPushButton::clicked, this, [this]() {
            if (m_canvas) m_canvas->actualSize();
        });
        layout->addWidget(actual);
    }
    root->addWidget(m_zoomGroup);

    // ---- Hint group ------------------------------------------------------
    m_hintGroup = new QWidget(this);
    {
        auto* layout = new QHBoxLayout(m_hintGroup);
        layout->setContentsMargins(0, 0, 0, 0);
        m_hintLabel = new QLabel(m_hintGroup);
        m_hintLabel->setObjectName(QStringLiteral("FieldLabel"));
        layout->addWidget(m_hintLabel);
    }
    root->addWidget(m_hintGroup);

    root->addStretch(1);

    m_docInfo = new QLabel(this);
    m_docInfo->setObjectName(QStringLiteral("FieldLabel"));
    root->addWidget(m_docInfo);
    root->addWidget(separator(this));

    auto* preview = new QPushButton(QStringLiteral("预览"), this);
    preview->setObjectName(QStringLiteral("PrimaryButton"));
    preview->setIcon(Icons::icon(QStringLiteral("play"), QColor("#FFFFFF"), 14));
    preview->setToolTip(QStringLiteral("运行交互预览 (F5)"));
    connect(preview, &QPushButton::clicked, this, &OptionsBar::previewRequested);
    root->addWidget(preview);

    auto* exportButton = new QPushButton(QStringLiteral("导出"), this);
    exportButton->setObjectName(QStringLiteral("GhostButton"));
    exportButton->setIcon(Icons::icon(QStringLiteral("export"), QColor("#C8C8C8"), 16));
    exportButton->setToolTip(QStringLiteral("导出 HTML / AI 交付包"));
    connect(exportButton, &QPushButton::clicked, this, &OptionsBar::exportRequested);
    root->addWidget(exportButton);

    // Connections ----------------------------------------------------------
    connect(m_fillButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        if (primaryElement()) {
            applyToPrimary([color](Element& e) {
                e.fill = color;
                if (e.strokeWidth <= 0.0) e.strokeWidth = 1.0;
            }, true);
        } else if (m_canvas) {
            const QColor oldFill = m_canvas->defaultFillColor();
            QColor stroke = m_canvas->defaultStrokeColor();
            if (stroke == oldFill.lighter(135)) stroke = color.lighter(135);
            m_canvas->setDefaultShapeStyle(color, stroke, m_canvas->defaultStrokeWidth(),
                                           m_canvas->defaultRadius(), m_canvas->defaultSides());
            emit defaultFillChanged(color);
        }
    });
    connect(m_strokeButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        if (primaryElement()) {
            applyToPrimary([color](Element& e) {
                e.stroke = color;
                if (e.strokeWidth <= 0.0) e.strokeWidth = 1.0;
            }, true);
        } else if (m_canvas) {
            m_canvas->setDefaultShapeStyle(m_canvas->defaultFillColor(), color,
                                           m_canvas->defaultStrokeWidth(),
                                           m_canvas->defaultRadius(), m_canvas->defaultSides());
        }
    });
    connect(m_textColorButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        applyToPrimary([color](Element& e) { e.textColor = color; }, true);
    });
    connect(m_strokeWidthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_updating) return;
                if (primaryElement()) {
                    applyToPrimary([value](Element& e) { e.strokeWidth = value; });
                } else if (m_canvas) {
                    m_canvas->setDefaultShapeStyle(m_canvas->defaultFillColor(),
                                                   m_canvas->defaultStrokeColor(), value,
                                                   m_canvas->defaultRadius(), m_canvas->defaultSides());
                }
            });
    connect(m_radiusSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_updating) return;
                if (primaryElement()) {
                    applyToPrimary([value](Element& e) { e.radius = value; });
                } else if (m_canvas) {
                    m_canvas->setDefaultShapeStyle(m_canvas->defaultFillColor(),
                                                   m_canvas->defaultStrokeColor(),
                                                   m_canvas->defaultStrokeWidth(), value,
                                                   m_canvas->defaultSides());
                }
            });
    connect(m_sidesSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_updating) return;
                if (primaryElement()) {
                    applyToPrimary([value](Element& e) { e.sides = qRound(value); });
                } else if (m_canvas) {
                    m_canvas->setDefaultShapeStyle(m_canvas->defaultFillColor(),
                                                   m_canvas->defaultStrokeColor(),
                                                   m_canvas->defaultStrokeWidth(),
                                                   m_canvas->defaultRadius(), qRound(value));
                }
            });
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        if (m_updating) return;
        applyToPrimary([font](Element& e) { e.fontFamily = font.family(); });
    });
    connect(m_fontSizeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_updating) return;
                applyToPrimary([value](Element& e) { e.fontSize = value; });
            });
    connect(m_boldButton, &QToolButton::toggled, this, [this](bool checked) {
        if (m_updating) return;
        applyToPrimary([checked](Element& e) { e.bold = checked; }, true);
    });
    connect(m_closedCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updating) return;
        applyToPrimary([checked](Element& e) { e.closed = checked; }, true);
    });
}

void OptionsBar::setContext(DesignDocument* document, CanvasWidget* canvas)
{
    m_document = document;
    m_canvas = canvas;
    if (m_canvas) {
        connect(m_canvas, &CanvasWidget::zoomChanged, this, [this](double zoom) {
            if (m_zoomLabel) m_zoomLabel->setText(QStringLiteral("%1%").arg(qRound(zoom * 100)));
        }, Qt::UniqueConnection);
    }
    refresh();
}

void OptionsBar::setTool(Tool tool)
{
    finishPendingEditNow();
    m_tool = tool;
    updateToolPresentation();
    refresh();
}

void OptionsBar::flushEditing()
{
    finishPendingEditNow();
}

void OptionsBar::updateToolPresentation()
{
    m_toolName->setText(toolDisplayName(m_tool));
    m_toolIcon->setPixmap(Icons::pixmap(toolIconName(m_tool), 22, QColor("#D6D6D6"), 2.0));
}

void OptionsBar::setGroupVisible(QWidget* group)
{
    m_selectGroup->hide();
    m_shapeGroup->hide();
    m_textGroup->hide();
    m_pathGroup->hide();
    m_zoomGroup->hide();
    m_hintGroup->hide();
    if (group) group->show();
}

Element* OptionsBar::primaryElement() const
{
    return m_canvas ? m_canvas->primaryElement() : nullptr;
}

void OptionsBar::refresh()
{
    if (!m_document || !m_canvas) return;
    m_updating = true;

    m_docInfo->setText(QStringLiteral("%1 × %2 px")
                           .arg(qRound(m_document->canvasSize.width()))
                           .arg(qRound(m_document->canvasSize.height())));
    if (m_zoomLabel) m_zoomLabel->setText(QStringLiteral("%1%").arg(qRound(m_canvas->zoom() * 100)));

    Element* element = primaryElement();
    const bool hasElement = element != nullptr;

    if (isShapeTool(m_tool)) {
        setGroupVisible(m_shapeGroup);
        m_shapeGroup->setEnabled(true);
        const bool showFill = m_tool != Tool::Line;
        m_fillLabel->setVisible(showFill);
        m_fillButton->setVisible(showFill);
        m_strokeLabel->setVisible(true);
        m_strokeButton->setVisible(true);
        m_strokeWidthLabel->setVisible(true);
        m_strokeWidthSpin->setVisible(true);
        m_radiusLabel->setVisible(m_tool == Tool::Rectangle);
        m_radiusSpin->setVisible(m_tool == Tool::Rectangle);
        m_sidesLabel->setVisible(m_tool == Tool::Polygon || m_tool == Tool::Star);
        m_sidesSpin->setVisible(m_tool == Tool::Polygon || m_tool == Tool::Star);

        if (element) {
            m_fillButton->setColor(element->fill);
            m_strokeButton->setColor(element->stroke);
            m_strokeWidthSpin->setValue(element->strokeWidth);
            m_radiusSpin->setValue(element->radius);
            m_sidesSpin->setValue(element->sides);
        } else {
            m_fillButton->setColor(m_canvas->defaultFillColor());
            m_strokeButton->setColor(m_canvas->defaultStrokeColor());
            m_strokeWidthSpin->setValue(m_canvas->defaultStrokeWidth());
            m_radiusSpin->setValue(m_canvas->defaultRadius());
            m_sidesSpin->setValue(m_canvas->defaultSides());
        }
        m_hintLabel->setText(QStringLiteral("在画布上拖拽创建 · 未选择对象时修改的是新对象默认样式"));
        m_hintGroup->hide();
    } else if (m_tool == Tool::Text) {
        setGroupVisible(m_textGroup);
        const bool isText = hasElement && element->type == ElementType::Text;
        m_textGroup->setEnabled(isText);
        if (isText) {
            m_fontCombo->setCurrentFont(QFont(element->fontFamily));
            m_fontSizeSpin->setValue(element->fontSize);
            m_boldButton->setChecked(element->bold);
            m_textColorButton->setColor(element->textColor);
        }
        if (!isText) {
            m_hintLabel->setText(QStringLiteral("在画布上点击创建文字，或选中文字对象后编辑字体样式"));
            m_hintGroup->show();
        } else {
            m_hintGroup->hide();
        }
    } else if (m_tool == Tool::Pen) {
        const bool isPath = hasElement && element->type == ElementType::Path;
        if (isPath) {
            setGroupVisible(m_pathGroup);
            m_pathGroup->setEnabled(true);
            m_closedCheck->setChecked(element->closed);
            m_hintGroup->hide();
        } else {
            setGroupVisible(m_hintGroup);
            m_hintLabel->setText(QStringLiteral("点击添加节点，双击或 Enter 完成路径，Esc 取消；也可选中已有路径修改闭合状态"));
            m_hintGroup->show();
        }
    } else if (m_tool == Tool::Zoom) {
        setGroupVisible(m_zoomGroup);
    } else if (m_tool == Tool::Hand) {
        setGroupVisible(m_hintGroup);
        m_hintLabel->setText(QStringLiteral("拖拽平移画布；按住空格键可临时切换到抓手"));
        m_hintGroup->show();
    } else {
        setGroupVisible(m_selectGroup);
        m_selectGroup->setEnabled(true);
        for (auto* button : m_selectGroup->findChildren<QToolButton*>()) {
            button->setEnabled(!m_canvas->selection().isEmpty());
        }
    }

    m_updating = false;
}

void OptionsBar::applyToPrimary(const std::function<void(Element&)>& mutate, bool immediate)
{
    Element* element = primaryElement();
    if (!element) return;
    if (immediate) {
        finishPendingEditNow();
        emit editBegan();
    } else {
        beginPendingEdit();
    }
    mutate(*element);
    emit documentEdited();
    if (immediate) emit editEnded();
}

void OptionsBar::beginPendingEdit()
{
    if (!m_pendingEdit) {
        m_pendingEdit = true;
        emit editBegan();
    }
    m_editTimer->start(450);
}

void OptionsBar::finishPendingEditNow()
{
    m_editTimer->stop();
    if (!m_pendingEdit) return;
    m_pendingEdit = false;
    emit editEnded();
}

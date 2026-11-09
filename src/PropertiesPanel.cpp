#include "PropertiesPanel.h"
#include "Animation.h"
#include "CanvasWidget.h"
#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QListWidget>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {
QString typeIconName(ElementType type)
{
    switch (type) {
    case ElementType::Rectangle: return QStringLiteral("rect");
    case ElementType::Ellipse: return QStringLiteral("ellipse");
    case ElementType::Line: return QStringLiteral("line");
    case ElementType::Polygon: return QStringLiteral("polygon");
    case ElementType::Star: return QStringLiteral("star");
    case ElementType::Path: return QStringLiteral("pen");
    case ElementType::Text: return QStringLiteral("text");
    }
    return QStringLiteral("rect");
}

QLabel* fieldLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("FieldLabel"));
    return label;
}

QDoubleSpinBox* makeSpin(QWidget* parent, double minimum, double maximum, int decimals = 1)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(minimum, maximum);
    spin->setDecimals(decimals);
    spin->setKeyboardTracking(true);
    spin->setFixedHeight(25);
    spin->setMinimumWidth(62);
    return spin;
}
} // namespace

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    m_editTimer = new QTimer(this);
    m_editTimer->setSingleShot(true);
    connect(m_editTimer, &QTimer::timeout, this, &PropertiesPanel::finishPendingEditNow);
}

QWidget* PropertiesPanel::createSection(const QString& title, QWidget* parent, QWidget*& body)
{
    auto* section = new QWidget(parent);
    auto* layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QFrame(section);
    header->setObjectName(QStringLiteral("SectionHeader"));
    header->setFixedHeight(25);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(9, 0, 8, 0);
    auto* titleLabel = new QLabel(title, header);
    titleLabel->setObjectName(QStringLiteral("PanelTitle"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);
    layout->addWidget(header);

    body = new QWidget(section);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(9, 8, 9, 10);
    bodyLayout->setSpacing(7);
    layout->addWidget(body);
    return section;
}

void PropertiesPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QFrame(this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    header->setFixedHeight(30);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 0, 6, 0);
    auto* title = new QLabel(QStringLiteral("属性"), header);
    title->setObjectName(QStringLiteral("PanelTitle"));
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    auto* resetIcon = new QLabel(header);
    resetIcon->setPixmap(Icons::pixmap(QStringLiteral("settings"), 15, QColor("#A0A0A0"), 2.0));
    resetIcon->setToolTip(QStringLiteral("属性面板"));
    headerLayout->addWidget(resetIcon);
    root->addWidget(header);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: #2B2B2B; }"));

    auto* content = new QWidget(scroll);
    content->setStyleSheet(QStringLiteral("background: #2B2B2B;"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(1);

    auto* identity = new QWidget(content);
    identity->setStyleSheet(QStringLiteral("background: #2B2B2B;"));
    auto* identityLayout = new QHBoxLayout(identity);
    identityLayout->setContentsMargins(10, 10, 10, 9);
    identityLayout->setSpacing(9);
    m_typeIcon = new QLabel(identity);
    m_typeIcon->setFixedSize(34, 34);
    m_typeIcon->setAlignment(Qt::AlignCenter);
    m_typeIcon->setStyleSheet(QStringLiteral("background: #333333; border: 1px solid #454545; border-radius: 3px;"));
    identityLayout->addWidget(m_typeIcon);
    auto* identityText = new QVBoxLayout();
    identityText->setContentsMargins(0, 0, 0, 0);
    identityText->setSpacing(2);
    m_nameEdit = new QLineEdit(identity);
    m_nameEdit->setPlaceholderText(QStringLiteral("对象名称"));
    m_nameEdit->setStyleSheet(QStringLiteral("QLineEdit { background: transparent; border: 1px solid transparent; padding: 1px 3px; font-weight: 600; }"
                                             "QLineEdit:hover { border-color: #4A4A4A; background: #262626; }"
                                             "QLineEdit:focus { border-color: #2D8CEB; background: #1E1E1E; }"));
    identityText->addWidget(m_nameEdit);
    m_typeLabel = new QLabel(identity);
    m_typeLabel->setStyleSheet(QStringLiteral("color: #8A8A8A; font-size: 11px;"));
    identityText->addWidget(m_typeLabel);
    identityLayout->addLayout(identityText, 1);
    contentLayout->addWidget(identity);

    m_selectionInfo = new QLabel(content);
    m_selectionInfo->setContentsMargins(10, 0, 10, 6);
    m_selectionInfo->setStyleSheet(QStringLiteral("color: #A0A0A0; font-size: 11px;"));
    contentLayout->addWidget(m_selectionInfo);

    m_elementContainer = new QWidget(content);
    auto* elementLayout = new QVBoxLayout(m_elementContainer);
    elementLayout->setContentsMargins(0, 0, 0, 0);
    elementLayout->setSpacing(1);

    // ---- Transform -------------------------------------------------------
    QWidget* transformBody = nullptr;
    m_transformSection = createSection(QStringLiteral("变换"), m_elementContainer, transformBody);
    {
        auto* bodyLayout = qobject_cast<QVBoxLayout*>(transformBody->layout());
        auto* grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(6);
        grid->setVerticalSpacing(6);
        m_xSpin = makeSpin(transformBody, -100000, 100000);
        m_ySpin = makeSpin(transformBody, -100000, 100000);
        m_wSpin = makeSpin(transformBody, 1, 100000);
        m_hSpin = makeSpin(transformBody, 1, 100000);
        m_xSpin->setSuffix(QStringLiteral(" px"));
        m_ySpin->setSuffix(QStringLiteral(" px"));
        m_wSpin->setSuffix(QStringLiteral(" px"));
        m_hSpin->setSuffix(QStringLiteral(" px"));
        grid->addWidget(fieldLabel(QStringLiteral("X"), transformBody), 0, 0);
        grid->addWidget(m_xSpin, 0, 1);
        grid->addWidget(fieldLabel(QStringLiteral("Y"), transformBody), 0, 2);
        grid->addWidget(m_ySpin, 0, 3);
        grid->addWidget(fieldLabel(QStringLiteral("W"), transformBody), 1, 0);
        grid->addWidget(m_wSpin, 1, 1);
        grid->addWidget(fieldLabel(QStringLiteral("H"), transformBody), 1, 2);
        grid->addWidget(m_hSpin, 1, 3);
        bodyLayout->addLayout(grid);

        auto* grid2 = new QGridLayout();
        grid2->setContentsMargins(0, 0, 0, 0);
        grid2->setHorizontalSpacing(6);
        grid2->setVerticalSpacing(6);
        m_rotationSpin = makeSpin(transformBody, -3600, 3600, 1);
        m_rotationSpin->setSuffix(QStringLiteral("°"));
        m_rotationSpin->setSingleStep(1);
        m_opacitySpin = makeSpin(transformBody, 0, 100, 0);
        m_opacitySpin->setSuffix(QStringLiteral("%"));
        grid2->addWidget(fieldLabel(QStringLiteral("旋转"), transformBody), 0, 0);
        grid2->addWidget(m_rotationSpin, 0, 1);
        grid2->addWidget(fieldLabel(QStringLiteral("不透明度"), transformBody), 0, 2);
        grid2->addWidget(m_opacitySpin, 0, 3);
        bodyLayout->addLayout(grid2);
    }
    elementLayout->addWidget(m_transformSection);

    // ---- Appearance ------------------------------------------------------
    QWidget* appearanceBody = nullptr;
    m_appearanceSection = createSection(QStringLiteral("外观"), m_elementContainer, appearanceBody);
    {
        auto* bodyLayout = qobject_cast<QVBoxLayout*>(appearanceBody->layout());

        m_fillRow = new QWidget(appearanceBody);
        auto* fillLayout = new QHBoxLayout(m_fillRow);
        fillLayout->setContentsMargins(0, 0, 0, 0);
        fillLayout->setSpacing(8);
        fillLayout->addWidget(fieldLabel(QStringLiteral("填充"), m_fillRow));
        m_fillButton = new ColorButton(m_fillRow);
        m_fillButton->setCompact(false);
        m_fillButton->setTextVisible(true);
        m_fillButton->setMinimumWidth(120);
        fillLayout->addWidget(m_fillButton, 1);
        bodyLayout->addWidget(m_fillRow);

        // Fill mode: solid or gradient.
        m_fillModeRow = new QWidget(appearanceBody);
        auto* modeLayout = new QHBoxLayout(m_fillModeRow);
        modeLayout->setContentsMargins(0, 0, 0, 0);
        modeLayout->setSpacing(6);
        modeLayout->addWidget(fieldLabel(QStringLiteral("填充方式"), m_fillModeRow));
        m_solidModeButton = new QToolButton(m_fillModeRow);
        m_solidModeButton->setText(QStringLiteral("纯色"));
        m_solidModeButton->setCheckable(true);
        m_solidModeButton->setFixedHeight(24);
        m_gradientModeButton = new QToolButton(m_fillModeRow);
        m_gradientModeButton->setText(QStringLiteral("渐变"));
        m_gradientModeButton->setCheckable(true);
        m_gradientModeButton->setFixedHeight(24);
        const QString modeStyle = QStringLiteral(
            "QToolButton { background: #2B2B2B; border: 1px solid #4A4A4A; border-radius: 3px;"
            " padding: 2px 12px; color: #BEBEBE; }"
            "QToolButton:hover { background: #3A3A3A; }"
            "QToolButton:checked { background: #2D8CEB; border-color: #2D8CEB; color: #FFFFFF; }");
        m_solidModeButton->setStyleSheet(modeStyle);
        m_gradientModeButton->setStyleSheet(modeStyle);
        auto* modeGroup = new QButtonGroup(m_fillModeRow);
        modeGroup->setExclusive(true);
        modeGroup->addButton(m_solidModeButton);
        modeGroup->addButton(m_gradientModeButton);
        modeLayout->addWidget(m_solidModeButton);
        modeLayout->addWidget(m_gradientModeButton);
        modeLayout->addStretch(1);
        bodyLayout->addWidget(m_fillModeRow);

        // Gradient editor and options.
        m_gradientControls = new QWidget(appearanceBody);
        auto* gradientLayout = new QVBoxLayout(m_gradientControls);
        gradientLayout->setContentsMargins(0, 2, 0, 2);
        gradientLayout->setSpacing(6);
        m_gradientEditor = new GradientEditor(m_gradientControls);
        gradientLayout->addWidget(m_gradientEditor);

        auto* typeRow = new QHBoxLayout();
        typeRow->setContentsMargins(0, 0, 0, 0);
        typeRow->setSpacing(6);
        typeRow->addWidget(fieldLabel(QStringLiteral("类型"), m_gradientControls));
        m_gradientTypeCombo = new QComboBox(m_gradientControls);
        m_gradientTypeCombo->addItem(QStringLiteral("线性"), QStringLiteral("linear"));
        m_gradientTypeCombo->addItem(QStringLiteral("径向"), QStringLiteral("radial"));
        m_gradientTypeCombo->setFixedWidth(74);
        typeRow->addWidget(m_gradientTypeCombo);
        m_gradientAngleLabel = fieldLabel(QStringLiteral("角度"), m_gradientControls);
        typeRow->addWidget(m_gradientAngleLabel);
        m_gradientAngleSpin = makeSpin(m_gradientControls, 0, 360, 0);
        m_gradientAngleSpin->setSingleStep(5);
        m_gradientAngleSpin->setSuffix(QStringLiteral("°"));
        m_gradientAngleSpin->setFixedWidth(64);
        m_gradientAngleSpin->setToolTip(QStringLiteral("0° = 从左到右，90° = 从上到下"));
        typeRow->addWidget(m_gradientAngleSpin);
        typeRow->addStretch(1);
        gradientLayout->addLayout(typeRow);

        auto* stopRow = new QHBoxLayout();
        stopRow->setContentsMargins(0, 0, 0, 0);
        stopRow->setSpacing(6);
        stopRow->addWidget(fieldLabel(QStringLiteral("色标"), m_gradientControls));
        m_stopColorButton = new ColorButton(m_gradientControls);
        m_stopColorButton->setCompact(false);
        m_stopColorButton->setMinimumWidth(110);
        stopRow->addWidget(m_stopColorButton, 1);
        m_addStopButton = new QToolButton(m_gradientControls);
        m_addStopButton->setIcon(Icons::icon(QStringLiteral("plus"), QColor("#C8C8C8"), 15));
        m_addStopButton->setFixedSize(26, 24);
        m_addStopButton->setToolTip(QStringLiteral("添加色标"));
        stopRow->addWidget(m_addStopButton);
        m_removeStopButton = new QToolButton(m_gradientControls);
        m_removeStopButton->setIcon(Icons::icon(QStringLiteral("minus"), QColor("#C8C8C8"), 15));
        m_removeStopButton->setFixedSize(26, 24);
        m_removeStopButton->setToolTip(QStringLiteral("删除当前色标"));
        stopRow->addWidget(m_removeStopButton);
        gradientLayout->addLayout(stopRow);

        auto* presetRow = new QHBoxLayout();
        presetRow->setContentsMargins(0, 0, 0, 0);
        presetRow->setSpacing(5);
        presetRow->addWidget(fieldLabel(QStringLiteral("预设"), m_gradientControls));
        const QVector<QPair<QString, GradientFill>> presets = {
            {QStringLiteral("极光"), [] {
                 GradientFill g; g.enabled = true; g.type = "linear"; g.angle = 135;
                 g.stops = {{0.0, QColor("#2D8CEB")}, {1.0, QColor("#A47BFF")}}; return g; }()},
            {QStringLiteral("日落"), [] {
                 GradientFill g; g.enabled = true; g.type = "linear"; g.angle = 90;
                 g.stops = {{0.0, QColor("#FF7A59")}, {0.55, QColor("#FFB454")},
                            {1.0, QColor("#FF4D8D")}}; return g; }()},
            {QStringLiteral("薄荷"), [] {
                 GradientFill g; g.enabled = true; g.type = "linear"; g.angle = 45;
                 g.stops = {{0.0, QColor("#3DDC97")}, {1.0, QColor("#2D8CEB")}}; return g; }()},
            {QStringLiteral("深空"), [] {
                 GradientFill g; g.enabled = true; g.type = "radial"; g.angle = 135;
                 g.stops = {{0.0, QColor("#2D8CEB")}, {1.0, QColor("#0F172A")}}; return g; }()},
        };
        for (const auto& preset : presets) {
            auto* button = new QToolButton(m_gradientControls);
            button->setFixedSize(52, 24);
            button->setIconSize(QSize(48, 18));
            button->setToolTip(preset.first);
            QPixmap pixmap(96, 36);
            pixmap.setDevicePixelRatio(2.0);
            pixmap.fill(Qt::transparent);
            {
                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing, true);
                QLinearGradient preview(0, 0, 48, 0);
                for (const auto& stop : preset.second.stops) {
                    preview.setColorAt(stop.position, stop.color);
                }
                painter.setPen(QPen(QColor("#5A5A5A"), 1.0));
                painter.setBrush(preview);
                painter.drawRoundedRect(QRectF(1, 1, 46, 16), 3, 3);
            }
            button->setIcon(QIcon(pixmap));
            connect(button, &QToolButton::clicked, this, [this, preset]() {
                if (!primaryElement() || m_updating) return;
                GradientFill gradient = preset.second;
                gradient.enabled = true;
                m_gradientModeButton->setChecked(true);
                m_gradientEditor->setGradient(gradient);
                applyToPrimary([gradient](Element& element) { element.gradient = gradient; }, true);
                refresh();
            });
            presetRow->addWidget(button);
        }
        presetRow->addStretch(1);
        gradientLayout->addLayout(presetRow);
        bodyLayout->addWidget(m_gradientControls);

        m_paletteRow = new QWidget(appearanceBody);
        auto* paletteLayout = new QVBoxLayout(m_paletteRow);
        paletteLayout->setContentsMargins(0, 4, 0, 0);
        paletteLayout->setSpacing(3);
        paletteLayout->addWidget(fieldLabel(QStringLiteral("调色板"), m_paletteRow));
        m_colorPalette = new ColorPaletteWidget(m_paletteRow);
        paletteLayout->addWidget(m_colorPalette);
        bodyLayout->addWidget(m_paletteRow);

        m_strokeRow = new QWidget(appearanceBody);
        auto* strokeLayout = new QVBoxLayout(m_strokeRow);
        strokeLayout->setContentsMargins(0, 0, 0, 0);
        strokeLayout->setSpacing(5);
        auto* strokeColorRow = new QHBoxLayout();
        strokeColorRow->setContentsMargins(0, 0, 0, 0);
        strokeColorRow->setSpacing(8);
        strokeColorRow->addWidget(fieldLabel(QStringLiteral("描边"), m_strokeRow));
        m_strokeButton = new ColorButton(m_strokeRow);
        m_strokeButton->setCompact(false);
        m_strokeButton->setMinimumWidth(120);
        strokeColorRow->addWidget(m_strokeButton, 1);
        strokeLayout->addLayout(strokeColorRow);
        auto* strokeWidthRow = new QHBoxLayout();
        strokeWidthRow->setContentsMargins(0, 0, 0, 0);
        strokeWidthRow->setSpacing(8);
        strokeWidthRow->addWidget(fieldLabel(QStringLiteral("粗细"), m_strokeRow));
        m_strokeWidthSpin = makeSpin(m_strokeRow, 0, 200, 1);
        m_strokeWidthSpin->setSuffix(QStringLiteral(" px"));
        strokeWidthRow->addWidget(m_strokeWidthSpin);
        strokeWidthRow->addStretch(1);
        strokeLayout->addLayout(strokeWidthRow);
        bodyLayout->addWidget(m_strokeRow);

        m_radiusRow = new QWidget(appearanceBody);
        auto* radiusLayout = new QHBoxLayout(m_radiusRow);
        radiusLayout->setContentsMargins(0, 0, 0, 0);
        radiusLayout->setSpacing(8);
        radiusLayout->addWidget(fieldLabel(QStringLiteral("圆角"), m_radiusRow));
        m_radiusSpin = makeSpin(m_radiusRow, 0, 500, 1);
        m_radiusSpin->setSuffix(QStringLiteral(" px"));
        radiusLayout->addWidget(m_radiusSpin);
        radiusLayout->addStretch(1);
        bodyLayout->addWidget(m_radiusRow);

        m_sidesRow = new QWidget(appearanceBody);
        auto* sidesLayout = new QHBoxLayout(m_sidesRow);
        sidesLayout->setContentsMargins(0, 0, 0, 0);
        sidesLayout->setSpacing(8);
        sidesLayout->addWidget(fieldLabel(QStringLiteral("边数"), m_sidesRow));
        m_sidesSpin = makeSpin(m_sidesRow, 3, 60, 0);
        sidesLayout->addWidget(m_sidesSpin);
        sidesLayout->addStretch(1);
        bodyLayout->addWidget(m_sidesRow);

        m_closedRow = new QWidget(appearanceBody);
        auto* closedLayout = new QHBoxLayout(m_closedRow);
        closedLayout->setContentsMargins(0, 0, 0, 0);
        m_closedCheck = new QCheckBox(QStringLiteral("闭合路径"), m_closedRow);
        closedLayout->addWidget(m_closedCheck);
        closedLayout->addStretch(1);
        bodyLayout->addWidget(m_closedRow);
    }
    elementLayout->addWidget(m_appearanceSection);

    // ---- Text ------------------------------------------------------------
    QWidget* textBody = nullptr;
    m_textSection = createSection(QStringLiteral("文字"), m_elementContainer, textBody);
    {
        auto* bodyLayout = qobject_cast<QVBoxLayout*>(textBody->layout());
        m_textEdit = new QPlainTextEdit(textBody);
        m_textEdit->setPlaceholderText(QStringLiteral("输入文字内容…"));
        m_textEdit->setFixedHeight(70);
        bodyLayout->addWidget(m_textEdit);

        auto* fontRow = new QHBoxLayout();
        fontRow->setContentsMargins(0, 0, 0, 0);
        fontRow->setSpacing(6);
        m_fontCombo = new QFontComboBox(textBody);
        fontRow->addWidget(m_fontCombo, 1);
        m_fontSizeSpin = makeSpin(textBody, 4, 400, 0);
        m_fontSizeSpin->setSuffix(QStringLiteral(" px"));
        m_fontSizeSpin->setFixedWidth(70);
        fontRow->addWidget(m_fontSizeSpin);
        bodyLayout->addLayout(fontRow);

        auto* styleRow = new QHBoxLayout();
        styleRow->setContentsMargins(0, 0, 0, 0);
        styleRow->setSpacing(6);
        m_boldButton = new QToolButton(textBody);
        m_boldButton->setText(QStringLiteral("B"));
        m_boldButton->setCheckable(true);
        QFont boldFont = m_boldButton->font();
        boldFont.setBold(true);
        m_boldButton->setFont(boldFont);
        m_boldButton->setFixedSize(28, 25);
        m_boldButton->setToolTip(QStringLiteral("粗体"));
        styleRow->addWidget(m_boldButton);
        m_textColorButton = new ColorButton(textBody);
        m_textColorButton->setCompact(true);
        m_textColorButton->setFixedSize(30, 25);
        styleRow->addWidget(m_textColorButton);
        styleRow->addSpacing(6);
        styleRow->addWidget(fieldLabel(QStringLiteral("对齐"), textBody));
        const QList<QPair<QString, QString>> aligns = {
            {"alignLeft", "left"}, {"alignCenter", "center"}, {"alignRight", "right"}};
        for (const auto& entry : aligns) {
            auto* button = new QToolButton(textBody);
            button->setIcon(Icons::icon(entry.first, QColor("#C8C8C8"), 17));
            button->setCheckable(true);
            button->setFixedSize(28, 25);
            button->setToolTip(QStringLiteral("文本对齐"));
            m_textAlignButtons.insert(entry.second, button);
            styleRow->addWidget(button);
        }
        styleRow->addStretch(1);
        bodyLayout->addLayout(styleRow);
    }
    elementLayout->addWidget(m_textSection);

    // ---- Interactions ----------------------------------------------------
    QWidget* interactionBody = nullptr;
    m_interactionSection = createSection(QStringLiteral("交互"), m_elementContainer, interactionBody);
    {
        auto* bodyLayout = qobject_cast<QVBoxLayout*>(interactionBody->layout());
        m_interactionHint = new QLabel(QStringLiteral("在预览模式 (F5) 中触发，可让对象点击或悬停时播放时间轴动画。"), interactionBody);
        m_interactionHint->setWordWrap(true);
        m_interactionHint->setStyleSheet(QStringLiteral("color: #8A8A8A; font-size: 11px;"));
        bodyLayout->addWidget(m_interactionHint);

        m_interactionList = new QListWidget(interactionBody);
        m_interactionList->setFixedHeight(66);
        m_interactionList->setStyleSheet(QStringLiteral(
            "QListWidget { background: #1E1E1E; border: 1px solid #404040; }"
            "QListWidget::item { padding: 3px 5px; color: #C8C8C8; }"
            "QListWidget::item:selected { background: #2D8CEB; color: #FFFFFF; }"));
        bodyLayout->addWidget(m_interactionList);

        auto* row1 = new QHBoxLayout();
        row1->setContentsMargins(0, 0, 0, 0);
        row1->setSpacing(5);
        m_triggerCombo = new QComboBox(interactionBody);
        m_triggerCombo->addItem(QStringLiteral("点击"), QStringLiteral("click"));
        m_triggerCombo->addItem(QStringLiteral("悬停"), QStringLiteral("hover"));
        m_triggerCombo->addItem(QStringLiteral("移出"), QStringLiteral("leave"));
        m_triggerCombo->addItem(QStringLiteral("载入时"), QStringLiteral("load"));
        m_actionCombo = new QComboBox(interactionBody);
        m_actionCombo->addItem(QStringLiteral("播放动画"), QStringLiteral("playAnimation"));
        m_actionCombo->addItem(QStringLiteral("停止全部动画"), QStringLiteral("stopAll"));
        row1->addWidget(m_triggerCombo, 1);
        row1->addWidget(m_actionCombo, 1);
        bodyLayout->addLayout(row1);

        auto* row2 = new QHBoxLayout();
        row2->setContentsMargins(0, 0, 0, 0);
        row2->setSpacing(5);
        m_animationCombo = new QComboBox(interactionBody);
        m_addInteractionButton = new QPushButton(QStringLiteral("添加"), interactionBody);
        m_addInteractionButton->setFixedWidth(48);
        m_removeInteractionButton = new QPushButton(QStringLiteral("删除"), interactionBody);
        m_removeInteractionButton->setObjectName(QStringLiteral("GhostButton"));
        m_removeInteractionButton->setFixedWidth(48);
        row2->addWidget(m_animationCombo, 1);
        row2->addWidget(m_addInteractionButton);
        row2->addWidget(m_removeInteractionButton);
        bodyLayout->addLayout(row2);
    }
    elementLayout->addWidget(m_interactionSection);
    elementLayout->addStretch(1);
    contentLayout->addWidget(m_elementContainer);

    // ---- Document --------------------------------------------------------
    m_documentContainer = new QWidget(content);
    auto* documentLayout = new QVBoxLayout(m_documentContainer);
    documentLayout->setContentsMargins(0, 0, 0, 0);
    documentLayout->setSpacing(1);
    QWidget* docBody = nullptr;
    QWidget* docSection = createSection(QStringLiteral("文档"), m_documentContainer, docBody);
    {
        auto* bodyLayout = qobject_cast<QVBoxLayout*>(docBody->layout());
        bodyLayout->addWidget(fieldLabel(QStringLiteral("标题"), docBody));
        m_docTitleEdit = new QLineEdit(docBody);
        bodyLayout->addWidget(m_docTitleEdit);
        auto* sizeGrid = new QGridLayout();
        sizeGrid->setContentsMargins(0, 0, 0, 0);
        sizeGrid->setHorizontalSpacing(6);
        sizeGrid->setVerticalSpacing(6);
        m_canvasWidthSpin = makeSpin(docBody, 100, 20000, 0);
        m_canvasWidthSpin->setSuffix(QStringLiteral(" px"));
        m_canvasHeightSpin = makeSpin(docBody, 100, 20000, 0);
        m_canvasHeightSpin->setSuffix(QStringLiteral(" px"));
        sizeGrid->addWidget(fieldLabel(QStringLiteral("宽度"), docBody), 0, 0);
        sizeGrid->addWidget(m_canvasWidthSpin, 0, 1);
        sizeGrid->addWidget(fieldLabel(QStringLiteral("高度"), docBody), 0, 2);
        sizeGrid->addWidget(m_canvasHeightSpin, 0, 3);
        bodyLayout->addLayout(sizeGrid);
        auto* bgRow = new QHBoxLayout();
        bgRow->setContentsMargins(0, 0, 0, 0);
        bgRow->setSpacing(8);
        bgRow->addWidget(fieldLabel(QStringLiteral("背景"), docBody));
        m_canvasBgButton = new ColorButton(docBody);
        m_canvasBgButton->setCompact(false);
        m_canvasBgButton->setMinimumWidth(120);
        bgRow->addWidget(m_canvasBgButton, 1);
        bodyLayout->addLayout(bgRow);
    }
    documentLayout->addWidget(docSection);
    documentLayout->addStretch(1);
    contentLayout->addWidget(m_documentContainer);

    m_emptyLabel = new QLabel(QStringLiteral("未选择对象\n在画布中选择对象以编辑属性，或在此调整画布设置。"), content);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #777777; padding: 24px 12px; font-size: 11px;"));
    contentLayout->addWidget(m_emptyLabel);
    contentLayout->addStretch(1);

    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    // ---- Connections -----------------------------------------------------
    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]() {
        if (m_updating) return;
        Element* element = primaryElement();
        if (!element) return;
        const QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty() || name == element->name) return;
        applyToPrimary([name](Element& e) { e.name = name; }, true);
    });
    connect(m_xSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.frame.moveLeft(v); });
    });
    connect(m_ySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.frame.moveTop(v); });
    });
    connect(m_wSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.frame.setWidth(qMax(1.0, v)); });
    });
    connect(m_hSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.frame.setHeight(qMax(1.0, v)); });
    });
    connect(m_rotationSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.rotation = v; });
    });
    connect(m_opacitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.opacity = qBound(0.0, v / 100.0, 1.0); });
    });
    connect(m_radiusSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.radius = v; });
    });
    connect(m_strokeWidthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.strokeWidth = v; });
    });
    connect(m_sidesSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.sides = qRound(v); });
    });
    connect(m_fillButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        applyToPrimary([color](Element& e) {
            if (e.type == ElementType::Text) e.textColor = color;
            else e.fill = color;
            if (e.strokeWidth <= 0.0) e.strokeWidth = 1.0;
        }, true);
    });
    connect(m_strokeButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        applyToPrimary([color](Element& e) {
            e.stroke = color;
            if (e.strokeWidth <= 0.0) e.strokeWidth = 1.0;
        }, true);
    });
    connect(m_textColorButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        applyToPrimary([color](Element& e) { e.textColor = color; }, true);
    });
    connect(m_closedCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updating) return;
        applyToPrimary([checked](Element& e) { e.closed = checked; }, true);
    });

    connect(m_solidModeButton, &QToolButton::clicked, this, [this]() {
        if (!m_updating) setFillMode(false);
    });
    connect(m_gradientModeButton, &QToolButton::clicked, this, [this]() {
        if (!m_updating) setFillMode(true);
    });
    connect(m_gradientEditor, &GradientEditor::editBegan, this, [this]() {
        m_gradientEditing = true;
        emit editBegan();
    });
    connect(m_gradientEditor, &GradientEditor::editEnded, this, [this]() {
        m_gradientEditing = false;
        emit editEnded();
    });
    connect(m_gradientEditor, &GradientEditor::gradientChanged, this,
            [this](const GradientFill& gradient) {
                Element* element = primaryElement();
                if (!element) return;
                element->gradient = gradient;
                element->gradient.enabled = true;
                updateSelectedStopColor();
                emit documentEdited();
            });
    connect(m_gradientEditor, &GradientEditor::selectedStopChanged, this, [this](int) {
        updateSelectedStopColor();
    });
    connect(m_gradientTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) {
                if (m_updating) return;
                GradientFill gradient = m_gradientEditor->gradient();
                gradient.type = m_gradientTypeCombo->currentData().toString();
                gradient.enabled = true;
                m_gradientEditor->setGradient(gradient);
                applyToPrimary([gradient](Element& element) { element.gradient = gradient; }, true);
                m_gradientAngleLabel->setVisible(gradient.type != QLatin1String("radial"));
                m_gradientAngleSpin->setVisible(gradient.type != QLatin1String("radial"));
            });
    connect(m_gradientAngleSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_updating) return;
                GradientFill gradient = m_gradientEditor->gradient();
                gradient.angle = value;
                gradient.enabled = true;
                m_gradientEditor->setGradient(gradient);
                applyToPrimary([gradient](Element& element) { element.gradient = gradient; });
            });
    connect(m_stopColorButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        GradientFill gradient = m_gradientEditor->gradient();
        const int index = m_gradientEditor->selectedStop();
        if (index < 0 || index >= gradient.stops.size()) return;
        gradient.stops[index].color = color;
        m_gradientEditor->setGradient(gradient);
        applyToPrimary([gradient](Element& element) { element.gradient = gradient; }, true);
    });
    connect(m_addStopButton, &QToolButton::clicked, this, [this]() {
        if (!m_updating) m_gradientEditor->addStop();
    });
    connect(m_removeStopButton, &QToolButton::clicked, this, [this]() {
        if (!m_updating) m_gradientEditor->removeStop();
    });
    m_colorPalette->colorPicked = [this](const QColor& color) {
        if (!m_updating) applyPaletteColor(color);
    };
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_updating) return;
        const QString text = m_textEdit->toPlainText();
        applyToPrimary([text](Element& e) { e.text = text; });
    });
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        if (m_updating) return;
        applyToPrimary([font](Element& e) { e.fontFamily = font.family(); });
    });
    connect(m_fontSizeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToPrimary([v](Element& e) { e.fontSize = v; });
    });
    connect(m_boldButton, &QToolButton::toggled, this, [this](bool checked) {
        if (m_updating) return;
        applyToPrimary([checked](Element& e) { e.bold = checked; }, true);
    });
    for (auto it = m_textAlignButtons.constBegin(); it != m_textAlignButtons.constEnd(); ++it) {
        const QString align = it.key();
        connect(it.value(), &QToolButton::clicked, this, [this, align]() {
            if (m_updating) return;
            applyToPrimary([align](Element& e) { e.align = align; }, true);
            updateTextAlignment(primaryElement());
        });
    }

    connect(m_docTitleEdit, &QLineEdit::editingFinished, this, [this]() {
        if (m_updating || !m_document) return;
        const QString title = m_docTitleEdit->text().trimmed();
        if (title.isEmpty() || title == m_document->title) return;
        applyToDocument([title](DesignDocument& doc) { doc.title = title; }, true);
    });
    connect(m_canvasWidthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToDocument([v](DesignDocument& doc) { doc.canvasSize.setWidth(v); });
    });
    connect(m_canvasHeightSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (m_updating) return;
        applyToDocument([v](DesignDocument& doc) { doc.canvasSize.setHeight(v); });
    });
    connect(m_canvasBgButton, &ColorButton::colorChanged, this, [this](const QColor& color) {
        if (m_updating) return;
        applyToDocument([color](DesignDocument& doc) { doc.background = color; }, true);
    });

    connect(m_addInteractionButton, &QPushButton::clicked, this, &PropertiesPanel::addInteraction);
    connect(m_removeInteractionButton, &QPushButton::clicked, this, &PropertiesPanel::removeInteraction);
    connect(m_actionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        const bool play = m_actionCombo->currentData().toString() == QLatin1String("playAnimation");
        m_animationCombo->setEnabled(play);
    });
}

void PropertiesPanel::setContext(DesignDocument* document, CanvasWidget* canvas)
{
    m_document = document;
    m_canvas = canvas;
    m_interactionSignature.clear();
    refresh();
}

void PropertiesPanel::flushEditing()
{
    finishPendingEditNow();
}

Element* PropertiesPanel::primaryElement() const
{
    return m_canvas ? m_canvas->primaryElement() : nullptr;
}

void PropertiesPanel::refresh()
{
    if (!m_document) return;
    m_updating = true;
    Element* element = primaryElement();
    const bool hasSelection = element != nullptr;
    const int selectionCount = m_canvas ? m_canvas->selection().size() : 0;

    m_elementContainer->setVisible(hasSelection);
    m_documentContainer->setVisible(!hasSelection);
    m_emptyLabel->setVisible(!hasSelection);
    m_typeIcon->setVisible(hasSelection);
    m_nameEdit->setVisible(hasSelection);
    m_typeLabel->setVisible(hasSelection);
    m_selectionInfo->setVisible(hasSelection);

    if (hasSelection) {
        m_typeLabel->setText(QStringLiteral("%1 · %2")
                                 .arg(elementTypeDisplayName(element->type),
                                      elementTypeToString(element->type)));
        if (!m_nameEdit->hasFocus()) m_nameEdit->setText(element->name);
        if (selectionCount > 1) {
            m_selectionInfo->setText(QStringLiteral("已选择 %1 个对象 · 正在编辑主选中对象").arg(selectionCount));
        } else {
            m_selectionInfo->setText(QStringLiteral("对象 ID：%1").arg(element->id));
        }
        updateTypeIcon(element);
        updateRowVisibility(element);

        setSpinValue(m_xSpin, element->frame.x());
        setSpinValue(m_ySpin, element->frame.y());
        setSpinValue(m_wSpin, element->frame.width());
        setSpinValue(m_hSpin, element->frame.height());
        setSpinValue(m_rotationSpin, element->rotation);
        setSpinValue(m_opacitySpin, element->opacity * 100.0);
        setSpinValue(m_radiusSpin, element->radius);
        setSpinValue(m_strokeWidthSpin, element->strokeWidth);
        setSpinValue(m_sidesSpin, element->sides);
        setSpinValue(m_fontSizeSpin, element->fontSize);

        m_fillButton->setColor(element->type == ElementType::Text ? element->textColor : element->fill);
        m_strokeButton->setColor(element->stroke);
        m_textColorButton->setColor(element->textColor);
        if (!m_textEdit->hasFocus()) {
            m_textEdit->setPlainText(element->text);
        }
        m_fontCombo->setCurrentFont(QFont(element->fontFamily));
        m_boldButton->setChecked(element->bold);
        m_closedCheck->setChecked(element->closed);
        updateTextAlignment(element);

        m_textSection->setVisible(element->type == ElementType::Text);
        rebuildInteractionList(element->id);
    } else {
        m_docTitleEdit->setText(m_document->title);
        setSpinValue(m_canvasWidthSpin, m_document->canvasSize.width());
        setSpinValue(m_canvasHeightSpin, m_document->canvasSize.height());
        m_canvasBgButton->setColor(m_document->background);
        m_interactionSignature.clear();
    }

    m_updating = false;
}

void PropertiesPanel::updateTypeIcon(Element* element)
{
    if (!element) return;
    m_typeIcon->setPixmap(Icons::pixmap(typeIconName(element->type), 22, QColor("#C8D0DC"), 2.0));
}

QString interactionSignatureFor(const DesignDocument* document, const QString& elementId)
{
    QString signature = elementId;
    for (int i = 0; i < document->interactions.size(); ++i) {
        const Interaction& interaction = document->interactions[i];
        if (interaction.sourceId != elementId) continue;
        QString animationName;
        if (const Animation* animation = document->findAnimation(interaction.animationId)) {
            animationName = animation->name;
        }
        signature += QStringLiteral("%1|%2|%3|%4;")
                         .arg(i)
                         .arg(interaction.trigger, interaction.action, animationName);
    }
    for (const auto& animation : document->animations) {
        signature += QStringLiteral("@%1:%2;").arg(animation.id, animation.name);
    }
    return signature;
}

void PropertiesPanel::rebuildInteractionList(const QString& elementId)
{
    if (!m_document) return;
    const QString signature = interactionSignatureFor(m_document, elementId);
    if (signature == m_interactionSignature) return;
    m_interactionSignature = signature;

    QSignalBlocker blocker(m_interactionList);
    m_interactionList->clear();
    for (int i = 0; i < m_document->interactions.size(); ++i) {
        const Interaction& interaction = m_document->interactions[i];
        if (interaction.sourceId != elementId) continue;
        QString animationName = QStringLiteral("—");
        if (const Animation* animation = m_document->findAnimation(interaction.animationId)) {
            animationName = animation->name;
        }
        QString actionText;
        if (interaction.action == QLatin1String("stopAll")) actionText = QStringLiteral("停止全部动画");
        else actionText = QStringLiteral("播放「%1」").arg(animationName);
        auto* item = new QListWidgetItem(QStringLiteral("%1 → %2")
                                             .arg(interaction.triggerDisplayName(), actionText),
                                         m_interactionList);
        item->setData(Qt::UserRole, i);
    }

    const QString previous = m_animationCombo->currentData().toString();
    QSignalBlocker comboBlocker(m_animationCombo);
    m_animationCombo->clear();
    for (const auto& animation : m_document->animations) {
        m_animationCombo->addItem(animation.name, animation.id);
    }
    if (!previous.isEmpty()) {
        const int index = m_animationCombo->findData(previous);
        if (index >= 0) m_animationCombo->setCurrentIndex(index);
    }
    m_animationCombo->setEnabled(m_actionCombo->currentData().toString() != QLatin1String("stopAll"));
    m_addInteractionButton->setEnabled(!m_document->animations.isEmpty());
}

void PropertiesPanel::addInteraction()
{
    if (!m_document || !primaryElement()) return;
    const QString trigger = m_triggerCombo->currentData().toString();
    const QString action = m_actionCombo->currentData().toString();
    const QString animationId = m_animationCombo->currentData().toString();
    if (action == QLatin1String("playAnimation") && animationId.isEmpty()) return;

    Interaction interaction;
    interaction.sourceId = primaryElement()->id;
    interaction.trigger = trigger;
    interaction.action = action;
    interaction.animationId = action == QLatin1String("playAnimation") ? animationId : QString();

    emit editBegan();
    m_document->interactions.append(interaction);
    emit documentEdited();
    emit editEnded();
    m_interactionSignature.clear();
    refresh();
}

void PropertiesPanel::removeInteraction()
{
    if (!m_document || !m_interactionList->currentItem()) return;
    const int index = m_interactionList->currentItem()->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_document->interactions.size()) return;
    emit editBegan();
    m_document->interactions.removeAt(index);
    emit documentEdited();
    emit editEnded();
    m_interactionSignature.clear();
    refresh();
}

void PropertiesPanel::updateRowVisibility(Element* element)
{
    if (!element) return;
    const ElementType type = element->type;
    const bool isText = type == ElementType::Text;
    m_strokeRow->setVisible(!isText);
    m_radiusRow->setVisible(type == ElementType::Rectangle);
    m_sidesRow->setVisible(type == ElementType::Polygon || type == ElementType::Star);
    m_closedRow->setVisible(type == ElementType::Path);
    m_strokeWidthSpin->setEnabled(true);
    updateGradientUi(element);
}

void PropertiesPanel::updateGradientUi(Element* element)
{
    if (!element || !m_gradientEditor) return;
    const bool supportsFill = element->type != ElementType::Text &&
                              element->type != ElementType::Line;
    const bool gradient = supportsFill && element->hasGradient();
    m_fillModeRow->setVisible(supportsFill);
    m_fillRow->setVisible(supportsFill && !gradient);
    m_gradientControls->setVisible(gradient);
    m_paletteRow->setVisible(true);
    if (!supportsFill) return;

    {
        QSignalBlocker solidBlocker(m_solidModeButton);
        QSignalBlocker gradientBlocker(m_gradientModeButton);
        m_solidModeButton->setChecked(!gradient);
        m_gradientModeButton->setChecked(gradient);
    }

    GradientFill preview = gradient ? element->gradient : makeDefaultGradient(*element);
    if (!m_gradientEditing) m_gradientEditor->setGradient(preview);
    if (!m_gradientTypeCombo->hasFocus()) {
        QSignalBlocker blocker(m_gradientTypeCombo);
        m_gradientTypeCombo->setCurrentIndex(preview.type == QLatin1String("radial") ? 1 : 0);
    }
    if (!m_gradientAngleSpin->hasFocus()) {
        QSignalBlocker blocker(m_gradientAngleSpin);
        m_gradientAngleSpin->setValue(preview.angle);
    }
    const bool showAngle = preview.type != QLatin1String("radial");
    m_gradientAngleLabel->setVisible(showAngle);
    m_gradientAngleSpin->setVisible(showAngle);
    m_addStopButton->setEnabled(true);
    m_removeStopButton->setEnabled(preview.stops.size() > 2);
    updateSelectedStopColor();
    if (m_colorPalette) m_colorPalette->update();
}

GradientFill PropertiesPanel::makeDefaultGradient(const Element& element) const
{
    GradientFill gradient;
    gradient.enabled = true;
    gradient.type = QStringLiteral("linear");
    gradient.angle = 135.0;
    QColor first = element.fill;
    if (!first.isValid() || first.alpha() == 0) first = QColor("#2D8CEB");
    QColor second = first.lightness() < 128 ? first.lighter(170) : first.darker(170);
    if (second == first) second = QColor("#A47BFF");
    gradient.stops = {{0.0, first}, {1.0, second}};
    return gradient;
}

void PropertiesPanel::setFillMode(bool gradient)
{
    Element* element = primaryElement();
    if (!element || element->type == ElementType::Text || element->type == ElementType::Line) {
        return;
    }
    if (gradient) {
        GradientFill fill = element->gradient;
        if (fill.stops.size() < 2) fill = makeDefaultGradient(*element);
        fill.enabled = true;
        m_gradientEditor->setGradient(fill);
        applyToPrimary([fill](Element& e) { e.gradient = fill; }, true);
    } else {
        QColor solid = element->hasGradient() && !element->gradient.stops.isEmpty()
                           ? element->gradient.stops.first().color
                           : element->fill;
        applyToPrimary([solid](Element& e) {
            e.fill = solid;
            e.gradient.enabled = false;
        }, true);
    }
}

void PropertiesPanel::updateSelectedStopColor()
{
    if (!m_gradientEditor || !m_stopColorButton) return;
    const GradientFill gradient = m_gradientEditor->gradient();
    const int index = m_gradientEditor->selectedStop();
    if (index < 0 || index >= gradient.stops.size()) return;
    QSignalBlocker blocker(m_stopColorButton);
    m_stopColorButton->setColor(gradient.stops[index].color);
    m_removeStopButton->setEnabled(gradient.stops.size() > 2);
}

void PropertiesPanel::applyGradient(const GradientFill& gradient, bool immediate)
{
    GradientFill copy = gradient;
    copy.enabled = true;
    m_gradientEditor->setGradient(copy);
    applyToPrimary([copy](Element& element) { element.gradient = copy; }, immediate);
}

void PropertiesPanel::applyPaletteColor(const QColor& color)
{
    Element* element = primaryElement();
    if (!element) return;
    if (element->type == ElementType::Text) {
        applyToPrimary([color](Element& e) { e.textColor = color; }, true);
        return;
    }
    if (element->type == ElementType::Line) {
        applyToPrimary([color](Element& e) { e.stroke = color; }, true);
        return;
    }
    if (element->hasGradient()) {
        GradientFill gradient = m_gradientEditor->gradient();
        const int index = m_gradientEditor->selectedStop();
        if (index < 0 || index >= gradient.stops.size()) return;
        gradient.stops[index].color = color;
        m_gradientEditor->setGradient(gradient);
        applyToPrimary([gradient](Element& e) { e.gradient = gradient; }, true);
        return;
    }
    applyToPrimary([color](Element& e) { e.fill = color; }, true);
}

void PropertiesPanel::updateTextAlignment(Element* element)
{
    if (!element) return;
    for (auto it = m_textAlignButtons.constBegin(); it != m_textAlignButtons.constEnd(); ++it) {
        QSignalBlocker blocker(it.value());
        it.value()->setChecked(it.key() == element->align);
    }
}

void PropertiesPanel::setSpinValue(QDoubleSpinBox* spin, double value)
{
    if (!spin || spin->hasFocus()) return;
    QSignalBlocker blocker(spin);
    spin->setValue(value);
}

void PropertiesPanel::applyToPrimary(const std::function<void(Element&)>& mutate, bool immediate)
{
    Element* element = primaryElement();
    if (!element) return;
    if (immediate) {
        finishPendingEditNow();
        emit editBegan();
        mutate(*element);
        emit documentEdited();
        emit editEnded();
    } else {
        beginPendingEdit();
        mutate(*element);
        emit documentEdited();
    }
}

void PropertiesPanel::applyToDocument(const std::function<void(DesignDocument&)>& mutate, bool immediate)
{
    if (!m_document) return;
    if (immediate) {
        finishPendingEditNow();
        emit editBegan();
        mutate(*m_document);
        emit documentEdited();
        emit editEnded();
    } else {
        beginPendingEdit();
        mutate(*m_document);
        emit documentEdited();
    }
}

void PropertiesPanel::beginPendingEdit()
{
    if (!m_pendingEdit) {
        m_pendingEdit = true;
        emit editBegan();
    }
    m_editTimer->start(450);
}

void PropertiesPanel::finishPendingEditNow()
{
    m_editTimer->stop();
    if (!m_pendingEdit) return;
    m_pendingEdit = false;
    emit editEnded();
}

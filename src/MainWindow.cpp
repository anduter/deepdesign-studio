#include "MainWindow.h"
#include "Animation.h"
#include "CanvasWidget.h"
#include "HtmlExporter.h"
#include "Icons.h"
#include "LayersPanel.h"
#include "OptionsBar.h"
#include "PreviewDialog.h"
#include "ProjectIO.h"
#include "PropertiesPanel.h"
#include "SpecExporter.h"
#include "Theme.h"
#include "TimelinePanel.h"
#include "ToolPalette.h"
#include "Widgets.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QUndoCommand>
#include <QUndoStack>
#include <QUrl>
#include <QVBoxLayout>
#include <QtMath>
#include <algorithm>

namespace {
class SnapshotCommand : public QUndoCommand {
public:
    SnapshotCommand(MainWindow* window, const QJsonObject& before, const QJsonObject& after)
        : QUndoCommand(QStringLiteral("编辑设计"))
        , m_window(window)
        , m_before(before)
        , m_after(after)
    {
    }

    void undo() override { m_window->applySnapshot(m_before); }
    void redo() override { m_window->applySnapshot(m_after); }

private:
    MainWindow* m_window;
    QJsonObject m_before;
    QJsonObject m_after;
};

QString safeFileName(QString name)
{
    static const QRegularExpression invalid(QStringLiteral("[\\\\/:*?\"<>|]"));
    name.replace(invalid, QStringLiteral("_"));
    name = name.trimmed();
    if (name.isEmpty()) name = QStringLiteral("未命名设计");
    return name;
}

QString findResource(const QString& relative)
{
    QStringList bases;
    bases << QCoreApplication::applicationDirPath();
    bases << QDir::currentPath();
    for (const auto& base : bases) {
        for (const QString& prefix : {QString(), QStringLiteral("../"), QStringLiteral("../../")}) {
            const QString candidate = QDir(base).filePath(prefix + relative);
            if (QFileInfo::exists(candidate)) return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return {};
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setMinimumSize(1120, 700);
    const QRect available = screen() ? screen()->availableGeometry() : QRect(0, 0, 1600, 1000);
    resize(qMin(1520, available.width() - 50), qMin(940, available.height() - 50));
    setAcceptDrops(true);
    setWindowIcon(QIcon(Icons::pixmap(QStringLiteral("sparkle"), 48, QColor("#2D8CEB"), 2.0)));

    m_undo = new QUndoStack(this);
    buildUi();
    createActions();
    buildMenus();
    setupConnections();
    initializeDocument();

    QSettings settings(QStringLiteral("DeepDesign"), QStringLiteral("DeepDesignStudio"));
    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (!geometry.isEmpty()) restoreGeometry(geometry);
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_optionsBar = new OptionsBar(central);
    root->addWidget(m_optionsBar);

    m_mainSplitter = new QSplitter(Qt::Vertical, central);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setHandleWidth(3);

    auto* workspace = new QWidget(m_mainSplitter);
    auto* workspaceLayout = new QHBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    m_toolPalette = new ToolPalette(workspace);
    workspaceLayout->addWidget(m_toolPalette);

    m_horizontalSplitter = new QSplitter(Qt::Horizontal, workspace);
    m_horizontalSplitter->setChildrenCollapsible(false);
    m_horizontalSplitter->setHandleWidth(3);

    auto* canvasArea = new QWidget(m_horizontalSplitter);
    auto* canvasLayout = new QVBoxLayout(canvasArea);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);

    auto* documentBar = new QWidget(canvasArea);
    documentBar->setObjectName(QStringLiteral("DocumentBar"));
    documentBar->setFixedHeight(31);
    auto* docLayout = new QHBoxLayout(documentBar);
    docLayout->setContentsMargins(0, 0, 10, 0);
    docLayout->setSpacing(8);

    m_documentTab = new QLabel(QStringLiteral("未命名设计"), documentBar);
    m_documentTab->setObjectName(QStringLiteral("DocumentTab"));
    m_documentTab->setFixedHeight(30);
    m_documentTab->setMinimumWidth(190);
    m_documentTab->setMaximumWidth(360);
    m_documentTab->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    docLayout->addWidget(m_documentTab);
    docLayout->addStretch(1);

    m_docInfoLabel = new QLabel(documentBar);
    m_docInfoLabel->setStyleSheet(QStringLiteral("color: #8A8A8A; font-size: 11px;"));
    docLayout->addWidget(m_docInfoLabel);
    auto* rgbLabel = new QLabel(QStringLiteral("RGB / 8 位"), documentBar);
    rgbLabel->setStyleSheet(QStringLiteral("color: #707070; font-size: 11px; padding-left: 10px;"));
    docLayout->addWidget(rgbLabel);
    canvasLayout->addWidget(documentBar);

    m_canvas = new CanvasWidget(canvasArea);
    canvasLayout->addWidget(m_canvas, 1);

    m_rightSplitter = new QSplitter(Qt::Vertical, m_horizontalSplitter);
    m_rightSplitter->setChildrenCollapsible(false);
    m_rightSplitter->setHandleWidth(3);
    m_layersPanel = new LayersPanel(m_rightSplitter);
    m_propertiesPanel = new PropertiesPanel(m_rightSplitter);
    m_layersPanel->setMinimumHeight(160);
    m_propertiesPanel->setMinimumHeight(320);
    m_rightSplitter->addWidget(m_layersPanel);
    m_rightSplitter->addWidget(m_propertiesPanel);
    m_rightSplitter->setStretchFactor(0, 0);
    m_rightSplitter->setStretchFactor(1, 1);
    m_rightSplitter->setSizes({210, 560});
    m_rightSplitter->setMinimumWidth(280);

    m_horizontalSplitter->addWidget(canvasArea);
    m_horizontalSplitter->addWidget(m_rightSplitter);
    m_horizontalSplitter->setStretchFactor(0, 1);
    m_horizontalSplitter->setStretchFactor(1, 0);
    m_horizontalSplitter->setSizes({1060, 330});

    workspaceLayout->addWidget(m_horizontalSplitter, 1);
    m_mainSplitter->addWidget(workspace);

    m_timeline = new TimelinePanel(m_mainSplitter);
    m_mainSplitter->addWidget(m_timeline);
    m_mainSplitter->setSizes({720, 215});
    root->addWidget(m_mainSplitter, 1);

    setCentralWidget(central);

    // Status bar
    QStatusBar* status = statusBar();
    status->setSizeGripEnabled(false);
    m_hintLabel = new QLabel(QStringLiteral("就绪"), status);
    m_hintLabel->setStyleSheet(QStringLiteral("color: #A0A0A0; padding-left: 6px;"));
    status->addWidget(m_hintLabel, 1);

    m_coordLabel = new QLabel(QStringLiteral("X 0   Y 0"), status);
    m_coordLabel->setFixedWidth(132);
    m_coordLabel->setStyleSheet(QStringLiteral("color: #8A8A8A; font-family: Consolas, monospace;"));
    status->addPermanentWidget(m_coordLabel);

    auto* sep1 = new QFrame(status);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QStringLiteral("background: #4A4A4A; max-width: 1px;"));
    sep1->setFixedHeight(16);
    status->addPermanentWidget(sep1);

    auto* zoomOut = new QToolButton(status);
    zoomOut->setIcon(Icons::icon(QStringLiteral("minus"), QColor("#C8C8C8"), 14));
    zoomOut->setFixedSize(22, 20);
    zoomOut->setToolTip(QStringLiteral("缩小"));
    status->addPermanentWidget(zoomOut);

    m_zoomCombo = new QComboBox(status);
    m_zoomCombo->setEditable(true);
    m_zoomCombo->setInsertPolicy(QComboBox::NoInsert);
    m_zoomCombo->setFixedWidth(75);
    m_zoomCombo->addItems({QStringLiteral("12%"), QStringLiteral("25%"), QStringLiteral("50%"),
                           QStringLiteral("100%"), QStringLiteral("200%"), QStringLiteral("400%")});
    m_zoomCombo->setCurrentText(QStringLiteral("100%"));
    m_zoomCombo->lineEdit()->setAlignment(Qt::AlignCenter);
    m_zoomCombo->setToolTip(QStringLiteral("缩放比例"));
    status->addPermanentWidget(m_zoomCombo);

    auto* zoomIn = new QToolButton(status);
    zoomIn->setIcon(Icons::icon(QStringLiteral("plus"), QColor("#C8C8C8"), 14));
    zoomIn->setFixedSize(22, 20);
    zoomIn->setToolTip(QStringLiteral("放大"));
    status->addPermanentWidget(zoomIn);

    auto* zoomFit = new QToolButton(status);
    zoomFit->setIcon(Icons::icon(QStringLiteral("fit"), QColor("#C8C8C8"), 14));
    zoomFit->setFixedSize(24, 20);
    zoomFit->setToolTip(QStringLiteral("适合屏幕"));
    status->addPermanentWidget(zoomFit);

    m_canvasSizeLabel = new QLabel(status);
    m_canvasSizeLabel->setStyleSheet(QStringLiteral("color: #8A8A8A; padding-left: 10px; padding-right: 6px;"));
    status->addPermanentWidget(m_canvasSizeLabel);

    connect(zoomOut, &QToolButton::clicked, this, [this]() { m_canvas->zoomOut(); });
    connect(zoomIn, &QToolButton::clicked, this, [this]() { m_canvas->zoomIn(); });
    connect(zoomFit, &QToolButton::clicked, this, [this]() { m_canvas->fitToView(); });
    connect(m_zoomCombo, qOverload<int>(&QComboBox::activated), this, [this](int) {
        applyZoomFromCombo();
    });
    connect(m_zoomCombo->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        applyZoomFromCombo();
    });
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------
QAction* MainWindow::addAction(const QString& key, const QString& text,
                               const QKeySequence& shortcut,
                               const std::function<void()>& handler,
                               bool canvasShortcut, const QString& iconName)
{
    auto* action = new QAction(text, this);
    if (!shortcut.isEmpty()) action->setShortcut(shortcut);
    if (!iconName.isEmpty()) action->setIcon(Icons::icon(iconName, QColor("#C8C8C8"), 16));
    connect(action, &QAction::triggered, this, handler);
    if (canvasShortcut) {
        action->setShortcutContext(Qt::WidgetShortcut);
        m_canvas->addAction(action);
    }
    action->setShortcutVisibleInContextMenu(true);
    m_actions.insert(key, action);
    return action;
}

QAction* MainWindow::action(const QString& key) const
{
    return m_actions.value(key, nullptr);
}

void MainWindow::createActions()
{
    addAction(QStringLiteral("new"), QStringLiteral("新建"), QKeySequence(QStringLiteral("Ctrl+N")),
              [this]() { newDocument(); }, false, QStringLiteral("new"));
    addAction(QStringLiteral("open"), QStringLiteral("打开…"), QKeySequence(QStringLiteral("Ctrl+O")),
              [this]() { openDialog(); }, false, QStringLiteral("open"));
    addAction(QStringLiteral("save"), QStringLiteral("保存"), QKeySequence(QStringLiteral("Ctrl+S")),
              [this]() { saveDocument(); }, false, QStringLiteral("save"));
    addAction(QStringLiteral("saveAs"), QStringLiteral("另存为…"),
              QKeySequence(QStringLiteral("Ctrl+Shift+S")),
              [this]() { saveDocumentAs(); }, false);
    addAction(QStringLiteral("exportPackage"), QStringLiteral("导出 HTML / AI 交付包…"),
              QKeySequence(QStringLiteral("Ctrl+E")), [this]() { exportAiPackage(); }, false,
              QStringLiteral("export"));
    addAction(QStringLiteral("exportPng"), QStringLiteral("导出 PNG 预览图…"),
              QKeySequence(QStringLiteral("Ctrl+Shift+E")), [this]() { exportPng(); }, false);
    addAction(QStringLiteral("quit"), QStringLiteral("退出"), QKeySequence(QStringLiteral("Ctrl+Q")),
              [this]() { close(); });

    addAction(QStringLiteral("exampleLogin"), QStringLiteral("登录界面示例"), QKeySequence(),
              [this]() { selectExample(QStringLiteral("login.ddesign")); });
    addAction(QStringLiteral("exampleMusic"), QStringLiteral("音乐播放器示例"), QKeySequence(),
              [this]() { selectExample(QStringLiteral("music-player.ddesign")); });
    addAction(QStringLiteral("exampleGradient"), QStringLiteral("渐变与参考线示例"), QKeySequence(),
              [this]() { selectExample(QStringLiteral("feature-test.ddesign")); });

    addAction(QStringLiteral("undo"), QStringLiteral("撤销"), QKeySequence(QStringLiteral("Ctrl+Z")),
              [this]() { flushEdits(); m_undo->undo(); }, true, QStringLiteral("undo"));
    auto* redo = addAction(QStringLiteral("redo"), QStringLiteral("重做"),
                           QKeySequence(QStringLiteral("Ctrl+Shift+Z")),
                           [this]() { flushEdits(); m_undo->redo(); }, true, QStringLiteral("redo"));
    redo->setShortcuts({QKeySequence(QStringLiteral("Ctrl+Shift+Z")),
                        QKeySequence(QStringLiteral("Ctrl+Y"))});

    addAction(QStringLiteral("cut"), QStringLiteral("剪切"), QKeySequence(QStringLiteral("Ctrl+X")),
              [this]() { cutSelection(); }, true);
    addAction(QStringLiteral("copy"), QStringLiteral("拷贝"), QKeySequence(QStringLiteral("Ctrl+C")),
              [this]() { copySelection(); }, true);
    addAction(QStringLiteral("paste"), QStringLiteral("粘贴"), QKeySequence(QStringLiteral("Ctrl+V")),
              [this]() { pasteSelection(); }, true);
    addAction(QStringLiteral("duplicate"), QStringLiteral("复制图层"),
              QKeySequence(QStringLiteral("Ctrl+J")), [this]() { duplicateSelection(); }, true,
              QStringLiteral("duplicate"));
    addAction(QStringLiteral("delete"), QStringLiteral("删除"), QKeySequence(QKeySequence::Delete),
              [this]() { deleteSelection(); }, true, QStringLiteral("trash"));
    addAction(QStringLiteral("selectAll"), QStringLiteral("全选"),
              QKeySequence(QStringLiteral("Ctrl+A")), [this]() { m_canvas->selectAll(); }, true);
    addAction(QStringLiteral("deselect"), QStringLiteral("取消选择"),
              QKeySequence(QStringLiteral("Ctrl+D")), [this]() { m_canvas->clearSelection(); }, true);
    addAction(QStringLiteral("invert"), QStringLiteral("反向选择"), QKeySequence(),
              [this]() { invertSelection(); }, true);

    addAction(QStringLiteral("canvasSize"), QStringLiteral("画布大小…"), QKeySequence(),
              [this]() { resizeCanvasDialog(); }, false, QStringLiteral("settings"));
    addAction(QStringLiteral("canvasBackground"), QStringLiteral("画布背景颜色…"), QKeySequence(),
              [this]() { chooseCanvasBackground(); }, false);

    addAction(QStringLiteral("orderForward"), QStringLiteral("上移一层"),
              QKeySequence(QStringLiteral("Ctrl+]")),
              [this]() { reorderSelection(0); }, true, QStringLiteral("chevronUp"));
    addAction(QStringLiteral("orderBackward"), QStringLiteral("下移一层"),
              QKeySequence(QStringLiteral("Ctrl+[")),
              [this]() { reorderSelection(1); }, true, QStringLiteral("chevronDown"));
    addAction(QStringLiteral("orderFront"), QStringLiteral("置于顶层"),
              QKeySequence(QStringLiteral("Ctrl+Shift+]")),
              [this]() { reorderSelection(2); }, true);
    addAction(QStringLiteral("orderBack"), QStringLiteral("置于底层"),
              QKeySequence(QStringLiteral("Ctrl+Shift+[")),
              [this]() { reorderSelection(3); }, true);

    addAction(QStringLiteral("alignLeft"), QStringLiteral("左对齐"), QKeySequence(),
              [this]() { alignSelection(QStringLiteral("left")); }, false, QStringLiteral("alignLeft"));
    addAction(QStringLiteral("alignHCenter"), QStringLiteral("水平居中"), QKeySequence(),
              [this]() { alignSelection(QStringLiteral("hcenter")); }, false, QStringLiteral("alignCenter"));
    addAction(QStringLiteral("alignRight"), QStringLiteral("右对齐"), QKeySequence(),
              [this]() { alignSelection(QStringLiteral("right")); }, false, QStringLiteral("alignRight"));
    addAction(QStringLiteral("alignTop"), QStringLiteral("顶对齐"), QKeySequence(),
              [this]() { alignSelection(QStringLiteral("top")); }, false, QStringLiteral("alignLeft"));
    addAction(QStringLiteral("alignVCenter"), QStringLiteral("垂直居中"), QKeySequence(),
              [this]() { alignSelection(QStringLiteral("vcenter")); }, false, QStringLiteral("alignCenter"));
    addAction(QStringLiteral("alignBottom"), QStringLiteral("底对齐"), QKeySequence(),
              [this]() { alignSelection(QStringLiteral("bottom")); }, false, QStringLiteral("alignRight"));
    addAction(QStringLiteral("distributeH"), QStringLiteral("水平等距分布"), QKeySequence(),
              [this]() { distributeSelection(true); }, false);
    addAction(QStringLiteral("distributeV"), QStringLiteral("垂直等距分布"), QKeySequence(),
              [this]() { distributeSelection(false); }, false);

    addAction(QStringLiteral("newAnimation"), QStringLiteral("新建动画"), QKeySequence(),
              [this]() { m_timeline->addAnimationAction(); }, false, QStringLiteral("animation"));
    addAction(QStringLiteral("recordKeyframe"), QStringLiteral("记录关键帧"),
              QKeySequence(QStringLiteral("Ctrl+Alt+K")),
              [this]() { m_timeline->recordKeyframesAction(); }, true);
    addAction(QStringLiteral("playPause"), QStringLiteral("播放 / 暂停时间轴"),
              QKeySequence(QStringLiteral("Ctrl+Space")),
              [this]() { m_timeline->playPause(); }, true);
    addAction(QStringLiteral("stopAnim"), QStringLiteral("停止并回到起点"),
              QKeySequence(QStringLiteral("Ctrl+Shift+Space")),
              [this]() { m_timeline->stopPlayback(); }, true);
    addAction(QStringLiteral("preview"), QStringLiteral("交互预览"), QKeySequence(QStringLiteral("F5")),
              [this]() { showPreview(); }, false, QStringLiteral("preview"));

    addAction(QStringLiteral("zoomIn"), QStringLiteral("放大"), QKeySequence(QStringLiteral("Ctrl++")),
              [this]() { m_canvas->zoomIn(); }, true, QStringLiteral("plus"));
    addAction(QStringLiteral("zoomOut"), QStringLiteral("缩小"), QKeySequence(QStringLiteral("Ctrl+-")),
              [this]() { m_canvas->zoomOut(); }, true, QStringLiteral("minus"));
    addAction(QStringLiteral("zoomFit"), QStringLiteral("适合屏幕"),
              QKeySequence(QStringLiteral("Ctrl+0")), [this]() { m_canvas->fitToView(); }, true,
              QStringLiteral("fit"));
    addAction(QStringLiteral("zoom100"), QStringLiteral("实际像素 (100%)"),
              QKeySequence(QStringLiteral("Ctrl+1")), [this]() { m_canvas->actualSize(); }, true);

    auto* grid = addAction(QStringLiteral("grid"), QStringLiteral("显示网格"),
                           QKeySequence(QStringLiteral("Ctrl+'")), []() {}, true,
                           QStringLiteral("grid"));
    grid->setCheckable(true);
    grid->setChecked(true);
    connect(grid, &QAction::toggled, this, [this](bool checked) {
        m_canvas->setGridVisible(checked);
        m_hintLabel->setText(checked ? QStringLiteral("网格已开启") : QStringLiteral("网格已关闭"));
    });
    auto* snap = addAction(QStringLiteral("snap"), QStringLiteral("吸附与智能参考线"),
                           QKeySequence(QStringLiteral("Ctrl+Shift+'")), []() {}, true);
    snap->setCheckable(true);
    snap->setChecked(true);
    connect(snap, &QAction::toggled, this, [this](bool checked) {
        m_canvas->setSnapEnabled(checked);
        m_hintLabel->setText(checked ? QStringLiteral("吸附已开启") : QStringLiteral("吸附已关闭"));
    });
    auto* rulers = addAction(QStringLiteral("rulers"), QStringLiteral("显示标尺"),
                             QKeySequence(QStringLiteral("Ctrl+R")), []() {}, true);
    rulers->setCheckable(true);
    rulers->setChecked(true);
    connect(rulers, &QAction::toggled, this, [this](bool checked) {
        m_canvas->setRulersVisible(checked);
    });
    auto* guides = addAction(QStringLiteral("guides"), QStringLiteral("显示参考线"),
                             QKeySequence(QStringLiteral("Ctrl+;")), []() {}, true);
    guides->setCheckable(true);
    guides->setChecked(true);
    connect(guides, &QAction::toggled, this, [this](bool checked) {
        m_canvas->setGuidesVisible(checked);
        m_hintLabel->setText(checked ? QStringLiteral("参考线已显示：可从标尺拖出参考线，用于测量与对齐")
                                     : QStringLiteral("参考线已隐藏"));
    });
    addAction(QStringLiteral("newHGuide"), QStringLiteral("新建横向参考线…"), QKeySequence(),
              [this]() { createGuideDialog(true); }, false);
    addAction(QStringLiteral("newVGuide"), QStringLiteral("新建纵向参考线…"), QKeySequence(),
              [this]() { createGuideDialog(false); }, false);
    addAction(QStringLiteral("clearGuides"), QStringLiteral("清除全部参考线"), QKeySequence(),
              [this]() {
                  if (m_document.guides.isEmpty()) return;
                  beginEdit();
                  m_document.clearGuides();
                  onDocumentEdited();
                  endEdit();
                  m_hintLabel->setText(QStringLiteral("已清除全部参考线"));
              }, false);
    auto* timelineAction = addAction(QStringLiteral("timeline"), QStringLiteral("显示时间轴"),
                                     QKeySequence(QStringLiteral("Ctrl+Alt+T")), []() {});
    timelineAction->setCheckable(true);
    timelineAction->setChecked(true);
    connect(timelineAction, &QAction::toggled, this, [this](bool checked) {
        m_timeline->setVisible(checked);
        if (!checked) m_timeline->stopPlayback();
    });
    auto* rightPanel = addAction(QStringLiteral("rightPanel"), QStringLiteral("显示图层面板"),
                                 QKeySequence(QStringLiteral("Ctrl+Alt+P")), []() {});
    rightPanel->setCheckable(true);
    rightPanel->setChecked(true);
    connect(rightPanel, &QAction::toggled, this, [this](bool checked) {
        m_rightSplitter->setVisible(checked);
    });

    addAction(QStringLiteral("helpFormat"), QStringLiteral("设计文件格式说明"), QKeySequence(),
              [this]() { openHelpDocument(QStringLiteral("DESIGN_FILE_FORMAT.md")); }, false,
              QStringLiteral("help"));
    addAction(QStringLiteral("helpAI"), QStringLiteral("AI 交付指南"), QKeySequence(),
              [this]() { openHelpDocument(QStringLiteral("AI_UI_SPEC.md")); }, false);
    addAction(QStringLiteral("helpShortcuts"), QStringLiteral("快捷键一览"), QKeySequence(),
              [this]() {
                  QMessageBox box(this);
                  box.setWindowTitle(QStringLiteral("快捷键"));
                  box.setTextFormat(Qt::RichText);
                  box.setText(QStringLiteral(
                      "<b>工具</b><br>V 选择 · R 矩形 · O 椭圆 · L 直线 · P 多边形 · S 星形 · N 钢笔 · T 文字 · H 抓手 · Z 缩放<br><br>"
                      "<b>画布</b><br>空格 + 拖拽 平移 · 滚轮 缩放 · Ctrl+0 适合屏幕 · Ctrl+1 100%<br>"
                      "方向键 微调 · Shift+方向键 10px · Delete 删除 · Shift 多选 / 等比缩放<br>"
                      "从上方标尺拖出横向参考线，从左侧标尺拖出纵向参考线；Ctrl+; 显示/隐藏参考线<br><br>"
                      "<b>编辑</b><br>Ctrl+Z 撤销 · Ctrl+Shift+Z 重做 · Ctrl+C/V/X 拷贝粘贴剪切<br>"
                      "Ctrl+J 复制图层 · Ctrl+D 取消选择 · Ctrl+A 全选<br><br>"
                      "<b>动效</b><br>Ctrl+Alt+K 记录关键帧 · Ctrl+Space 播放/暂停 · F5 交互预览"));
                  box.exec();
              });
    addAction(QStringLiteral("about"), QStringLiteral("关于灵绘 DeepDesign Studio"),
              QKeySequence(), [this]() {
                  QMessageBox box(this);
                  box.setWindowTitle(QStringLiteral("关于"));
                  box.setTextFormat(Qt::RichText);
                  box.setText(QStringLiteral(
                      "<h3>灵绘 DeepDesign Studio 1.0</h3>"
                      "<p>面向 UI 交互设计的 Qt 6 / C++ 桌面设计工具。</p>"
                      "<p>支持矩形、椭圆、直线、多边形、星形、自由路径、文字，图层管理、"
                      "标尺参考线、智能吸附、时间轴动效、交互触发器，以及 AI 可读的 <code>.ddesign</code> 文件。</p>"
                      "<p>导出交付包包含：<code>index.html</code> 交互预览、<code>design.json</code>、"
                      "<code>UI_SPEC.md</code>、<code>AI_HANDOFF.md</code>。</p>"
                      "<p style='color:#888'>Qt %1 · C++17 · 单文件 JSON 设计格式</p>")
                      .arg(QString::fromLatin1(qVersion())));
                  box.exec();
              });

    connect(m_undo, &QUndoStack::canUndoChanged, this, [this]() { updateActions(); });
    connect(m_undo, &QUndoStack::canRedoChanged, this, [this]() { updateActions(); });
    connect(m_undo, &QUndoStack::cleanChanged, this, [this]() { updateTitle(); });
}

void MainWindow::buildMenus()
{
    QMenuBar* bar = menuBar();
    auto* file = bar->addMenu(QStringLiteral("文件(&F)"));
    file->addAction(action(QStringLiteral("new")));
    file->addAction(action(QStringLiteral("open")));
    file->addAction(action(QStringLiteral("save")));
    file->addAction(action(QStringLiteral("saveAs")));
    file->addSeparator();
    file->addAction(action(QStringLiteral("exportPackage")));
    file->addAction(action(QStringLiteral("exportPng")));
    file->addSeparator();
    QMenu* examples = file->addMenu(QStringLiteral("打开示例"));
    examples->addAction(action(QStringLiteral("exampleLogin")));
    examples->addAction(action(QStringLiteral("exampleMusic")));
    examples->addAction(action(QStringLiteral("exampleGradient")));
    file->addSeparator();
    file->addAction(action(QStringLiteral("quit")));

    auto* edit = bar->addMenu(QStringLiteral("编辑(&E)"));
    edit->addAction(action(QStringLiteral("undo")));
    edit->addAction(action(QStringLiteral("redo")));
    edit->addSeparator();
    edit->addAction(action(QStringLiteral("cut")));
    edit->addAction(action(QStringLiteral("copy")));
    edit->addAction(action(QStringLiteral("paste")));
    edit->addAction(action(QStringLiteral("duplicate")));
    edit->addAction(action(QStringLiteral("delete")));
    edit->addSeparator();
    edit->addAction(action(QStringLiteral("selectAll")));
    edit->addAction(action(QStringLiteral("deselect")));
    edit->addAction(action(QStringLiteral("invert")));

    auto* image = bar->addMenu(QStringLiteral("图像(&I)"));
    image->addAction(action(QStringLiteral("canvasSize")));
    image->addAction(action(QStringLiteral("canvasBackground")));
    image->addSeparator();
    image->addAction(action(QStringLiteral("zoomFit")));
    image->addAction(action(QStringLiteral("zoom100")));

    auto* layer = bar->addMenu(QStringLiteral("图层(&L)"));
    layer->addAction(action(QStringLiteral("duplicate")));
    layer->addAction(action(QStringLiteral("delete")));
    layer->addSeparator();
    layer->addAction(action(QStringLiteral("orderForward")));
    layer->addAction(action(QStringLiteral("orderBackward")));
    layer->addAction(action(QStringLiteral("orderFront")));
    layer->addAction(action(QStringLiteral("orderBack")));

    auto* object = bar->addMenu(QStringLiteral("对象(&O)"));
    QMenu* align = object->addMenu(QStringLiteral("对齐"));
    align->addAction(action(QStringLiteral("alignLeft")));
    align->addAction(action(QStringLiteral("alignHCenter")));
    align->addAction(action(QStringLiteral("alignRight")));
    align->addSeparator();
    align->addAction(action(QStringLiteral("alignTop")));
    align->addAction(action(QStringLiteral("alignVCenter")));
    align->addAction(action(QStringLiteral("alignBottom")));
    QMenu* distribute = object->addMenu(QStringLiteral("分布"));
    distribute->addAction(action(QStringLiteral("distributeH")));
    distribute->addAction(action(QStringLiteral("distributeV")));
    object->addSeparator();
    object->addAction(action(QStringLiteral("orderForward")));
    object->addAction(action(QStringLiteral("orderBackward")));
    object->addAction(action(QStringLiteral("orderFront")));
    object->addAction(action(QStringLiteral("orderBack")));

    auto* selection = bar->addMenu(QStringLiteral("选择(&S)"));
    selection->addAction(action(QStringLiteral("selectAll")));
    selection->addAction(action(QStringLiteral("deselect")));
    selection->addAction(action(QStringLiteral("invert")));

    auto* animation = bar->addMenu(QStringLiteral("动画(&A)"));
    animation->addAction(action(QStringLiteral("newAnimation")));
    animation->addAction(action(QStringLiteral("recordKeyframe")));
    animation->addSeparator();
    animation->addAction(action(QStringLiteral("playPause")));
    animation->addAction(action(QStringLiteral("stopAnim")));
    animation->addSeparator();
    animation->addAction(action(QStringLiteral("preview")));

    auto* view = bar->addMenu(QStringLiteral("视图(&V)"));
    view->addAction(action(QStringLiteral("zoomIn")));
    view->addAction(action(QStringLiteral("zoomOut")));
    view->addAction(action(QStringLiteral("zoomFit")));
    view->addAction(action(QStringLiteral("zoom100")));
    view->addSeparator();
    view->addAction(action(QStringLiteral("grid")));
    view->addAction(action(QStringLiteral("snap")));
    view->addAction(action(QStringLiteral("rulers")));
    view->addAction(action(QStringLiteral("guides")));
    QMenu* guideMenu = view->addMenu(QStringLiteral("参考线"));
    guideMenu->addAction(action(QStringLiteral("newHGuide")));
    guideMenu->addAction(action(QStringLiteral("newVGuide")));
    guideMenu->addSeparator();
    guideMenu->addAction(action(QStringLiteral("clearGuides")));
    view->addSeparator();
    view->addAction(action(QStringLiteral("timeline")));
    view->addAction(action(QStringLiteral("rightPanel")));

    auto* help = bar->addMenu(QStringLiteral("帮助(&H)"));
    help->addAction(action(QStringLiteral("helpFormat")));
    help->addAction(action(QStringLiteral("helpAI")));
    help->addAction(action(QStringLiteral("helpShortcuts")));
    help->addSeparator();
    help->addAction(action(QStringLiteral("about")));
}

// ---------------------------------------------------------------------------
// Signal wiring
// ---------------------------------------------------------------------------
void MainWindow::setupConnections()
{
    connect(m_toolPalette, &ToolPalette::toolChanged, this, [this](Tool tool) {
        m_canvas->setTool(tool);
        m_optionsBar->setTool(tool);
    });
    connect(m_toolPalette, &ToolPalette::defaultFillChanged, this, [this](const QColor& color) {
        m_canvas->setDefaultFillColor(color);
    });

    connect(m_canvas, &CanvasWidget::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_canvas, &CanvasWidget::documentEdited, this, &MainWindow::onDocumentEdited);
    connect(m_canvas, &CanvasWidget::editBegan, this, [this]() {
        m_timeline->stopPlayback();
        m_canvas->clearPreviewAnimation();
        beginEdit();
    });
    connect(m_canvas, &CanvasWidget::editEnded, this, &MainWindow::endEdit);
    connect(m_canvas, &CanvasWidget::zoomChanged, this, &MainWindow::updateZoomUi);
    connect(m_canvas, &CanvasWidget::cursorMoved, this, [this](const QPointF& point) {
        m_coordLabel->setText(QStringLiteral("X %1   Y %2")
                                  .arg(qRound(point.x()))
                                  .arg(qRound(point.y())));
    });
    connect(m_canvas, &CanvasWidget::statusHint, this, [this](const QString& hint) {
        if (!hint.isEmpty()) m_hintLabel->setText(hint);
    });
    connect(m_canvas, &CanvasWidget::toolRequested, this, [this](Tool tool) {
        m_toolPalette->setTool(tool, false);
        m_canvas->setTool(tool);
        m_optionsBar->setTool(tool);
    });

    connect(m_optionsBar, &OptionsBar::alignRequested, this, [this](const QString& mode) {
        if (mode == QLatin1String("distributeH")) distributeSelection(true);
        else if (mode == QLatin1String("distributeV")) distributeSelection(false);
        else alignSelection(mode);
    });
    connect(m_optionsBar, &OptionsBar::previewRequested, this, &MainWindow::showPreview);
    connect(m_optionsBar, &OptionsBar::exportRequested, this, &MainWindow::exportAiPackage);
    connect(m_optionsBar, &OptionsBar::defaultFillChanged, this, [this](const QColor& color) {
        m_toolPalette->setDefaultFillColor(color);
    });

    auto beginPanelEdit = [this]() {
        m_timeline->stopPlayback();
        m_canvas->clearPreviewAnimation();
        beginEdit();
    };
    connect(m_optionsBar, &OptionsBar::editBegan, this, beginPanelEdit);
    connect(m_optionsBar, &OptionsBar::editEnded, this, &MainWindow::endEdit);
    connect(m_optionsBar, &OptionsBar::documentEdited, this, &MainWindow::onDocumentEdited);

    connect(m_layersPanel, &LayersPanel::selectionRequested, this, [this](const QStringList& ids) {        m_canvas->setSelection(ids);
    });
    connect(m_layersPanel, &LayersPanel::editBegan, this, beginPanelEdit);
    connect(m_layersPanel, &LayersPanel::editEnded, this, &MainWindow::endEdit);
    connect(m_layersPanel, &LayersPanel::documentEdited, this, &MainWindow::onDocumentEdited);

    connect(m_propertiesPanel, &PropertiesPanel::editBegan, this, beginPanelEdit);
    connect(m_propertiesPanel, &PropertiesPanel::editEnded, this, &MainWindow::endEdit);
    connect(m_propertiesPanel, &PropertiesPanel::documentEdited, this, &MainWindow::onDocumentEdited);

    connect(m_timeline, &TimelinePanel::editBegan, this, &MainWindow::beginEdit);
    connect(m_timeline, &TimelinePanel::editEnded, this, &MainWindow::endEdit);
    connect(m_timeline, &TimelinePanel::documentEdited, this, &MainWindow::onDocumentEdited);
    connect(m_timeline, &TimelinePanel::previewChanged, this,
            [this](const QString& animationId, double time) {
                if (animationId.isEmpty()) m_canvas->clearPreviewAnimation();
                else m_canvas->setPreviewAnimation(animationId, time);
            });
    connect(m_timeline, &TimelinePanel::statusMessage, this, [this](const QString& message) {
        if (!message.isEmpty()) m_hintLabel->setText(message);
    });
}

void MainWindow::initializeDocument()
{
    m_canvas->setDocument(&m_document);
    m_optionsBar->setContext(&m_document, m_canvas);
    m_layersPanel->setContext(&m_document, m_canvas);
    m_propertiesPanel->setContext(&m_document, m_canvas);
    m_timeline->setContext(&m_document, m_canvas);
    m_undo->clear();
    m_undo->setClean();
    m_canvas->fitToView();
    refreshAll();
    updateTitle();
    updateActions();
    m_hintLabel->setText(QStringLiteral("从左侧工具栏选择形状，在画布上拖拽绘制；F5 交互预览"));
    m_initialized = true;
}

// ---------------------------------------------------------------------------
// Document state
// ---------------------------------------------------------------------------
void MainWindow::beginEdit()
{
    if (!m_editBefore.isEmpty()) return;
    m_editBefore = m_document.toJson();
}

void MainWindow::endEdit()
{
    if (m_editBefore.isEmpty()) return;
    const QJsonObject before = m_editBefore;
    m_editBefore = QJsonObject();
    const QJsonObject after = m_document.toJson();
    if (before == after) return;
    m_undo->push(new SnapshotCommand(this, before, after));
}

void MainWindow::flushEdits()
{
    m_canvas->flushEditing();
    m_propertiesPanel->flushEditing();
    m_optionsBar->flushEditing();
}

void MainWindow::onDocumentEdited()
{
    m_canvas->update();
    m_layersPanel->reload();
    m_propertiesPanel->refresh();
    if (!m_timeline->isInteracting()) m_timeline->refresh();
    m_optionsBar->refresh();
    updateDocumentInfo();
    updateTitle();
    updateActions();
}

void MainWindow::onSelectionChanged(const QStringList& ids)
{
    m_layersPanel->syncSelection(ids);
    m_propertiesPanel->refresh();
    m_optionsBar->refresh();
    m_timeline->setSelection(ids);
    updateActions();
    if (ids.isEmpty()) {
        m_hintLabel->setText(QStringLiteral("未选择对象；可从左侧选择工具继续绘制"));
    } else if (ids.size() == 1) {
        if (const Element* element = m_document.findElement(ids.first())) {
            m_hintLabel->setText(QStringLiteral("已选择：%1 · %2")
                                     .arg(element->name, elementTypeDisplayName(element->type)));
        }
    } else {
        m_hintLabel->setText(QStringLiteral("已选择 %1 个对象").arg(ids.size()));
    }
}

void MainWindow::refreshAll()
{
    m_canvas->update();
    m_layersPanel->reload();
    m_propertiesPanel->refresh();
    m_timeline->refresh();
    m_optionsBar->refresh();
    updateDocumentInfo();
    updateTitle();
    updateActions();
}

void MainWindow::applySnapshot(const QJsonObject& json)
{
    QString error;
    if (!m_document.fromJson(json, &error)) return;
    m_timeline->stopPlayback();
    m_canvas->clearPreviewAnimation();
    m_canvas->pruneSelection();
    refreshAll();
    m_canvas->update();
}

void MainWindow::updateTitle()
{
    const QString name = m_document.title.trimmed().isEmpty()
                             ? QStringLiteral("未命名设计")
                             : m_document.title.trimmed();
    const bool dirty = !m_undo->isClean();
    setWindowTitle(QStringLiteral("%1%2 — 灵绘 DeepDesign Studio")
                       .arg(name, dirty ? QStringLiteral(" *") : QString()));
    if (m_documentTab) {
        m_documentTab->setText(dirty ? QStringLiteral("● %1").arg(name) : name);
        m_documentTab->setToolTip(m_filePath.isEmpty() ? QStringLiteral("未保存的设计文件")
                                                       : m_filePath);
    }
}

void MainWindow::updateDocumentInfo()
{
    const QString size = QStringLiteral("%1 × %2 px")
                             .arg(qRound(m_document.canvasSize.width()))
                             .arg(qRound(m_document.canvasSize.height()));
    if (m_docInfoLabel) {
        m_docInfoLabel->setText(QStringLiteral("%1 · %2 图层 · %3 动画")
                                    .arg(size)
                                    .arg(m_document.elements.size())
                                    .arg(m_document.animations.size()));
    }
    if (m_canvasSizeLabel) m_canvasSizeLabel->setText(size);
}

void MainWindow::updateActions()
{
    if (!m_canvas) return;
    const bool hasSelection = !m_canvas->selection().isEmpty();
    const bool hasElements = !m_document.elements.isEmpty();
    const bool hasAnimations = !m_document.animations.isEmpty();

    auto setEnabled = [this](const QString& key, bool enabled) {
        if (auto* a = action(key)) a->setEnabled(enabled);
    };
    setEnabled(QStringLiteral("undo"), m_undo->canUndo());
    setEnabled(QStringLiteral("redo"), m_undo->canRedo());
    setEnabled(QStringLiteral("cut"), hasSelection);
    setEnabled(QStringLiteral("copy"), hasSelection);
    setEnabled(QStringLiteral("paste"), true);
    setEnabled(QStringLiteral("duplicate"), hasSelection);
    setEnabled(QStringLiteral("delete"), hasSelection);
    setEnabled(QStringLiteral("selectAll"), hasElements);
    setEnabled(QStringLiteral("deselect"), hasSelection);
    setEnabled(QStringLiteral("invert"), hasElements);
    setEnabled(QStringLiteral("exportPackage"), hasElements);
    setEnabled(QStringLiteral("exportPng"), hasElements);
    setEnabled(QStringLiteral("preview"), hasElements);
    setEnabled(QStringLiteral("recordKeyframe"), hasAnimations && hasSelection);
    setEnabled(QStringLiteral("playPause"), hasAnimations);
    setEnabled(QStringLiteral("stopAnim"), hasAnimations);
    for (const QString& key : {QStringLiteral("orderForward"), QStringLiteral("orderBackward"),
                               QStringLiteral("orderFront"), QStringLiteral("orderBack"),
                               QStringLiteral("alignLeft"), QStringLiteral("alignHCenter"),
                               QStringLiteral("alignRight"), QStringLiteral("alignTop"),
                               QStringLiteral("alignVCenter"), QStringLiteral("alignBottom"),
                               QStringLiteral("distributeH"), QStringLiteral("distributeV")}) {
        setEnabled(key, hasSelection);
    }

    auto setChecked = [this](const QString& key, bool checked) {
        if (auto* a = action(key)) {
            QSignalBlocker blocker(a);
            a->setChecked(checked);
        }
    };
    setChecked(QStringLiteral("grid"), m_canvas->gridVisible());
    setChecked(QStringLiteral("snap"), m_canvas->snapEnabled());
    setChecked(QStringLiteral("rulers"), m_canvas->rulersVisible());
    setChecked(QStringLiteral("guides"), m_canvas->guidesVisible());
    setChecked(QStringLiteral("timeline"), !m_timeline->isHidden());
    setChecked(QStringLiteral("rightPanel"), !m_rightSplitter->isHidden());
}

void MainWindow::updateZoomUi(double zoom)
{
    m_updatingZoom = true;
    const QString text = QStringLiteral("%1%").arg(qRound(zoom * 100));
    QSignalBlocker blocker(m_zoomCombo);
    m_zoomCombo->setEditText(text);
    m_updatingZoom = false;
}

void MainWindow::applyZoomFromCombo()
{
    if (m_updatingZoom) return;
    QString text = m_zoomCombo->currentText().trimmed();
    text.remove(QLatin1Char('%'));
    bool ok = false;
    const double percent = text.toDouble(&ok);
    if (!ok || percent < 5.0 || percent > 800.0) {
        updateZoomUi(m_canvas->zoom());
        return;
    }
    m_canvas->setZoom(percent / 100.0);
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------
void MainWindow::newDocument()
{
    flushEdits();
    if (!maybeSave()) return;
    m_document.clear();
    m_filePath.clear();
    m_undo->clear();
    m_undo->setClean();
    m_canvas->setDocument(&m_document);
    m_optionsBar->setTool(m_toolPalette->currentTool());
    m_layersPanel->setContext(&m_document, m_canvas);
    m_propertiesPanel->setContext(&m_document, m_canvas);
    m_timeline->setContext(&m_document, m_canvas);
    m_canvas->fitToView();
    refreshAll();
    m_hintLabel->setText(QStringLiteral("新建空白画布 · %1 × %2 px")
                             .arg(qRound(m_document.canvasSize.width()))
                             .arg(qRound(m_document.canvasSize.height())));
}

void MainWindow::openDialog()
{
    flushEdits();
    if (!maybeSave()) return;
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开 DeepDesign 设计文件"),
        m_filePath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                             : QFileInfo(m_filePath).absolutePath(),
        ProjectIO::fileFilter());
    if (path.isEmpty()) return;
    loadDocumentFile(path);
}

void MainWindow::openDocument(const QString& path)
{
    if (!m_initialized) {
        loadDocumentFile(path, false);
        return;
    }
    flushEdits();
    if (!maybeSave()) return;
    loadDocumentFile(path);
}

bool MainWindow::loadDocumentFile(const QString& path, bool addToRecent)
{
    Q_UNUSED(addToRecent);
    DesignDocument document;
    QString error;
    if (!ProjectIO::load(document, path, &error)) {
        QMessageBox::critical(this, QStringLiteral("打开失败"), error);
        return false;
    }
    m_document = document;
    m_filePath = path;
    m_undo->clear();
    m_undo->setClean();
    m_canvas->setDocument(&m_document);
    m_layersPanel->setContext(&m_document, m_canvas);
    m_propertiesPanel->setContext(&m_document, m_canvas);
    m_timeline->setContext(&m_document, m_canvas);
    m_canvas->fitToView();
    refreshAll();
    m_hintLabel->setText(QStringLiteral("已打开：%1").arg(QFileInfo(path).fileName()));
    return true;
}

bool MainWindow::saveDocument()
{
    if (m_filePath.isEmpty()) return saveDocumentAs();
    return saveToPath(m_filePath);
}

bool MainWindow::saveDocumentAs()
{
    flushEdits();
    QString defaultPath = m_filePath;
    if (defaultPath.isEmpty()) {
        defaultPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                          .filePath(safeFileName(m_document.title) + QStringLiteral(".ddesign"));
    }
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存设计文件"),
                                                       defaultPath, ProjectIO::fileFilter());
    if (path.isEmpty()) return false;
    QString finalPath = path;
    if (QFileInfo(finalPath).suffix().isEmpty()) {
        finalPath += QStringLiteral(".") + ProjectIO::defaultSuffix();
    }
    return saveToPath(finalPath);
}

bool MainWindow::saveToPath(const QString& path)
{
    flushEdits();
    QString error;
    if (!ProjectIO::save(m_document, path, &error)) {
        QMessageBox::critical(this, QStringLiteral("保存失败"), error);
        return false;
    }
    m_filePath = path;
    m_undo->setClean();
    updateTitle();
    m_hintLabel->setText(QStringLiteral("已保存：%1").arg(QFileInfo(path).fileName()));
    return true;
}

bool MainWindow::maybeSave()
{
    flushEdits();
    if (m_undo->isClean()) return true;
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("保存更改"));
    box.setIcon(QMessageBox::Question);
    box.setText(QStringLiteral("是否保存对「%1」的更改？").arg(m_document.title));
    box.setInformativeText(QStringLiteral("如果不保存，最近的修改将会丢失。"));
    QPushButton* saveButton = box.addButton(QStringLiteral("保存"), QMessageBox::AcceptRole);
    box.addButton(QStringLiteral("不保存"), QMessageBox::DestructiveRole);
    QPushButton* cancelButton = box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(saveButton);
    box.exec();
    if (box.clickedButton() == saveButton) return saveDocument();
    if (box.clickedButton() == cancelButton) return false;
    return true;
}

void MainWindow::exportAiPackage()
{
    flushEdits();
    const QString start = m_filePath.isEmpty()
                              ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                              : QFileInfo(m_filePath).absolutePath();
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择导出文件夹（将写入 index.html / design.json / UI_SPEC.md / AI_HANDOFF.md）"),
        start, QFileDialog::ShowDirsOnly);
    if (directory.isEmpty()) return;
    QString error;
    if (!HtmlExporter::exportPackage(m_document, directory, &error)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"), error);
        return;
    }
    m_hintLabel->setText(QStringLiteral("已导出 AI 交付包：%1").arg(directory));
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("导出成功"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("已生成 AI / Web 交付包"));
    box.setInformativeText(QStringLiteral(
        "index.html — 可直接打开的交互预览\n"
        "design.json — DeepSeek 可读的完整设计数据\n"
        "UI_SPEC.md — 布局 / 颜色 / 交互 / 动画规范\n"
        "AI_HANDOFF.md — 交给 DeepSeek 的任务说明与提示词\n\n"
        "文件夹：%1").arg(directory));
    QPushButton* openButton = box.addButton(QStringLiteral("打开文件夹"), QMessageBox::ActionRole);
    box.addButton(QStringLiteral("完成"), QMessageBox::AcceptRole);
    box.exec();
    if (box.clickedButton() == openButton) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
    }
}

void MainWindow::exportPng()
{
    flushEdits();
    QString defaultPath = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))
                              .filePath(safeFileName(m_document.title) + QStringLiteral(".png"));
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出 PNG 预览图"), defaultPath, QStringLiteral("PNG 图片 (*.png)"));
    if (path.isEmpty()) return;
    const QImage image = ProjectIO::renderToImage(m_document, 2.0, false);
    if (image.isNull() || !image.save(path, "PNG")) {
        QMessageBox::critical(this, QStringLiteral("导出失败"), QStringLiteral("无法写入 PNG 文件。"));
        return;
    }
    m_hintLabel->setText(QStringLiteral("已导出 PNG：%1").arg(QFileInfo(path).fileName()));
}

void MainWindow::showPreview()
{
    flushEdits();
    if (m_document.elements.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("预览"), QStringLiteral("画布中还没有元素。"));
        return;
    }
    m_timeline->stopPlayback();
    m_canvas->clearPreviewAnimation();
    PreviewDialog dialog(m_document, this);
    dialog.exec();
    m_canvas->clearPreviewAnimation();
    m_canvas->setFocus();
}

void MainWindow::resizeCanvasDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("画布大小"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();
    auto* widthSpin = new QSpinBox(&dialog);
    widthSpin->setRange(100, 20000);
    widthSpin->setValue(qRound(m_document.canvasSize.width()));
    widthSpin->setSuffix(QStringLiteral(" px"));
    auto* heightSpin = new QSpinBox(&dialog);
    heightSpin->setRange(100, 20000);
    heightSpin->setValue(qRound(m_document.canvasSize.height()));
    heightSpin->setSuffix(QStringLiteral(" px"));
    form->addRow(QStringLiteral("宽度"), widthSpin);
    form->addRow(QStringLiteral("高度"), heightSpin);
    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) return;
    beginEdit();
    m_document.canvasSize = QSizeF(widthSpin->value(), heightSpin->value());
    onDocumentEdited();
    endEdit();
    m_canvas->fitToView();
}

void MainWindow::chooseCanvasBackground()
{
    const QColor color = QColorDialog::getColor(m_document.background, this,
                                                QStringLiteral("画布背景颜色"),
                                                QColorDialog::DontUseNativeDialog);
    if (!color.isValid() || color == m_document.background) return;
    beginEdit();
    m_document.background = color;
    onDocumentEdited();
    endEdit();
}

void MainWindow::createGuideDialog(bool horizontal)
{
    const double dimension = horizontal ? m_document.canvasSize.height()
                                        : m_document.canvasSize.width();
    bool ok = false;
    const double position = QInputDialog::getDouble(
        this,
        horizontal ? QStringLiteral("新建横向参考线") : QStringLiteral("新建纵向参考线"),
        horizontal ? QStringLiteral("Y 坐标 (px)：") : QStringLiteral("X 坐标 (px)："),
        qRound(dimension / 2.0), -dimension, dimension * 2.0, 1, &ok);
    if (!ok) return;
    beginEdit();
    m_document.addGuide(horizontal ? QStringLiteral("horizontal") : QStringLiteral("vertical"),
                        position);
    onDocumentEdited();
    endEdit();
    m_hintLabel->setText(horizontal
                             ? QStringLiteral("已创建横向参考线 Y = %1 px").arg(qRound(position))
                             : QStringLiteral("已创建纵向参考线 X = %1 px").arg(qRound(position)));
}

void MainWindow::openHelpDocument(const QString& fileName)
{
    const QString path = findResource(QStringLiteral("docs/") + fileName);
    if (path.isEmpty()) {
        QMessageBox::information(
            this, QStringLiteral("文档未找到"),
            QStringLiteral("未找到 docs/%1。\n请确认 docs 目录与程序或项目目录同级。").arg(fileName));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::selectExample(const QString& exampleFileName)
{
    const QString path = findResource(QStringLiteral("examples/") + exampleFileName);
    if (path.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("示例未找到"),
                                 QStringLiteral("未找到 examples/%1。").arg(exampleFileName));
        return;
    }
    openDocument(path);
}

// ---------------------------------------------------------------------------
// Edit operations
// ---------------------------------------------------------------------------
void MainWindow::copySelection()
{
    const QStringList ids = m_canvas->selection();
    if (ids.isEmpty()) return;
    QJsonArray array;
    for (const auto& id : ids) {
        if (const Element* element = m_document.findElement(id)) array.append(element->toJson());
    }
    QJsonArray interactionArray;
    for (const auto& interaction : m_document.interactions) {
        if (ids.contains(interaction.sourceId)) interactionArray.append(interaction.toJson());
    }
    QJsonObject root;
    root["format"] = QStringLiteral("DeepDesignClipboard");
    root["version"] = 1;
    root["elements"] = array;
    root["interactions"] = interactionArray;
    QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented)));
    m_hintLabel->setText(QStringLiteral("已拷贝 %1 个对象").arg(ids.size()));
}

void MainWindow::cutSelection()
{
    copySelection();
    deleteSelection();
}

void MainWindow::pasteSelection()
{
    const QString text = QApplication::clipboard()->text();
    if (text.trimmed().isEmpty()) return;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        m_hintLabel->setText(QStringLiteral("剪贴板内容不是有效的 DeepDesign 对象"));
        return;
    }
    QJsonArray elements;
    QJsonArray interactions;
    if (document.isObject()) {
        elements = document.object().value("elements").toArray();
        interactions = document.object().value("interactions").toArray();
    } else if (document.isArray()) {
        elements = document.array();
    }
    if (elements.isEmpty()) return;

    beginEdit();
    QHash<QString, QString> idMap;
    QStringList newIds;
    for (const auto& value : elements) {
        Element element = Element::fromJson(value.toObject());
        const QString oldId = element.id;
        element.id = m_document.makeId(elementTypeToString(element.type));
        element.name = m_document.uniqueName(element.name);
        element.translateBy(QPointF(24, 24));
        m_document.addElement(element);
        if (!oldId.isEmpty()) idMap.insert(oldId, element.id);
        newIds.append(element.id);
    }
    for (const auto& value : interactions) {
        Interaction interaction = Interaction::fromJson(value.toObject());
        if (!idMap.contains(interaction.sourceId)) continue;
        if (!interaction.animationId.isEmpty() && !m_document.findAnimation(interaction.animationId)) continue;
        interaction.sourceId = idMap.value(interaction.sourceId);
        m_document.interactions.append(interaction);
    }
    onDocumentEdited();
    endEdit();
    m_canvas->setSelection(newIds);
}

void MainWindow::duplicateSelection()
{
    const QStringList ids = m_canvas->selection();
    if (ids.isEmpty()) return;
    beginEdit();
    QStringList newIds;
    for (const auto& id : ids) {
        const QString newId = m_document.duplicateElement(id);
        if (!newId.isEmpty()) newIds.append(newId);
    }
    onDocumentEdited();
    endEdit();
    if (!newIds.isEmpty()) {
        m_canvas->setSelection(newIds);
        m_hintLabel->setText(QStringLiteral("已复制 %1 个对象").arg(newIds.size()));
    }
}

void MainWindow::deleteSelection()
{
    if (m_canvas->selection().isEmpty()) return;
    m_canvas->deleteSelection();
}

void MainWindow::invertSelection()
{
    QStringList ids;
    const QStringList selected = m_canvas->selection();
    for (const auto& element : m_document.elements) {
        if (element.visible && !selected.contains(element.id)) ids.append(element.id);
    }
    m_canvas->setSelection(ids);
}

void MainWindow::alignSelection(const QString& mode)
{
    const QStringList ids = m_canvas->selection();
    if (ids.isEmpty()) return;
    QRectF target;
    if (ids.size() == 1) {
        target = QRectF(QPointF(0, 0), m_document.canvasSize);
    } else {
        for (const auto& id : ids) {
            if (const Element* element = m_document.findElement(id)) {
                target = target.isNull() ? element->frame : target.united(element->frame);
            }
        }
    }
    beginEdit();
    for (const auto& id : ids) {
        Element* element = m_document.findElement(id);
        if (!element) continue;
        QRectF frame = element->frame;
        if (mode == QLatin1String("left")) frame.moveLeft(target.left());
        else if (mode == QLatin1String("hcenter")) frame.moveLeft(target.center().x() - frame.width() / 2.0);
        else if (mode == QLatin1String("right")) frame.moveLeft(target.right() - frame.width());
        else if (mode == QLatin1String("top")) frame.moveTop(target.top());
        else if (mode == QLatin1String("vcenter")) frame.moveTop(target.center().y() - frame.height() / 2.0);
        else if (mode == QLatin1String("bottom")) frame.moveTop(target.bottom() - frame.height());
        element->frame = frame;
    }
    onDocumentEdited();
    endEdit();
    m_hintLabel->setText(QStringLiteral("已对齐 %1 个对象").arg(ids.size()));
}

void MainWindow::distributeSelection(bool horizontal)
{
    const QStringList ids = m_canvas->selection();
    if (ids.size() < 3) {
        m_hintLabel->setText(QStringLiteral("等距分布至少需要选择 3 个对象"));
        return;
    }
    QVector<Element*> selected;
    for (const auto& id : ids) {
        if (Element* element = m_document.findElement(id)) selected.append(element);
    }
    std::sort(selected.begin(), selected.end(), [horizontal](const Element* a, const Element* b) {
        return horizontal ? a->center().x() < b->center().x() : a->center().y() < b->center().y();
    });
    const double first = horizontal ? selected.first()->center().x() : selected.first()->center().y();
    const double last = horizontal ? selected.last()->center().x() : selected.last()->center().y();
    const double step = (last - first) / (selected.size() - 1);
    beginEdit();
    for (int i = 0; i < selected.size(); ++i) {
        QRectF frame = selected[i]->frame;
        if (horizontal) frame.moveLeft(first + step * i - frame.width() / 2.0);
        else frame.moveTop(first + step * i - frame.height() / 2.0);
        selected[i]->frame = frame;
    }
    onDocumentEdited();
    endEdit();
    m_hintLabel->setText(QStringLiteral("已等距分布 %1 个对象").arg(selected.size()));
}

void MainWindow::reorderSelection(int mode)
{
    const QStringList ids = m_canvas->selection();
    if (ids.isEmpty()) return;
    beginEdit();
    if (mode == 0) m_document.bringForward(ids);
    else if (mode == 1) m_document.sendBackward(ids);
    else if (mode == 2) m_document.bringToFront(ids);
    else m_document.sendToBack(ids);
    onDocumentEdited();
    endEdit();
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent* event)
{
    flushEdits();
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    QSettings settings(QStringLiteral("DeepDesign"), QStringLiteral("DeepDesignStudio"));
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        for (const auto& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                const QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();
                if (suffix == QLatin1String("ddesign") || suffix == QLatin1String("json")) {
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    for (const auto& url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) continue;
        const QString path = url.toLocalFile();
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix == QLatin1String("ddesign") || suffix == QLatin1String("json")) {
            event->acceptProposedAction();
            openDocument(path);
            return;
        }
    }
}

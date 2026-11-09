# 架构说明

## 1. 技术栈

- Qt 6.11.2 Widgets，MinGW 13.1，C++17
- qmake（无 CMake 依赖）
- 全部图标由 `QPainter` 手绘，无图片资源
- 深色 Photoshop 风格主题在 `Theme.cpp` 中集中定义

## 2. 模块分层

```text
main.cpp
  ├─ CLI（--selftest / --render / --export / --validate）
  └─ MainWindow
        ├─ DesignDocument（Model）
        ├─ CanvasWidget ── Renderer ── QPainter
        ├─ ToolPalette / OptionsBar
        ├─ LayersPanel / PropertiesPanel / TimelinePanel
        ├─ PreviewDialog ── Renderer + 独立动画引擎
        └─ HtmlExporter / SpecExporter
```

### Model 层

`DesignDocument` 是普通 C++ 值类型，不依赖 QWidget：

- `elements`：从底到顶的有序元素数组
- `animations`：动画、轨道、关键帧
- `interactions`：触发器
- `guides`：参考线
- `toJson/fromJson`：文件读写

元素支持 7 种类型：矩形、椭圆、直线、多边形、星形、自由路径、文本。  
渐变 `GradientFill` 是元素的独立可选属性；`fill` 保留为回退色。

### 渲染层

`Renderer::paintElement` 是编辑器和预览共用的唯一绘制入口：

- 文档坐标 → 屏幕坐标由调用方的 `QPainter` 变换负责
- 元素围绕中心旋转
- 渐变由 `QLinearGradient` / `QRadialGradient` 生成
- 动画覆盖通过 `QVariantMap`（属性 → 值）应用到元素副本

### 画布层

`CanvasWidget` 负责：

- 坐标变换（origin/zoom）、标尺、网格、参考线
- 工具状态机（选择 / 创建 / 钢笔 / 抓手 / 缩放 / 参考线）
- 命中测试、八向缩放手柄、旋转手柄、框选
- 智能吸附与参考线
- 内联文本编辑器（QTextEdit 覆盖层）
- 通过信号向 `MainWindow` 报告编辑开始/结束（用于撤销快照）

### 面板

- `LayersPanel`：自定义 delegate 绘制图层行，支持眼睛/锁、拖拽排序
- `PropertiesPanel`：变换、外观、渐变编辑、调色板、文字、交互
- `TimelinePanel`：动画列表、关键帧画布、播放器、记录关键帧
- `OptionsBar`：跟随工具的上下文选项（形状/文字/缩放/对齐）
- `Widgets`：`ColorButton`、`GradientEditor`、`ColorPaletteWidget`

### 撤销

`MainWindow` 持有一个 `QUndoStack`。每次交互：

```text
beginEdit()  -> 记录 m_document.toJson() 快照
...修改文档、刷新 UI...
endEdit()    -> 比较前后 JSON，不同则 push SnapshotCommand
```

`SnapshotCommand` 的 undo/redo 直接恢复整份 JSON。  
对于连续编辑（属性输入、渐变拖动），各面板使用“首次变更 + 300–450ms 防抖”只生成一个撤销步骤。

### 导出

- `ProjectIO`：`.ddesign` 读写、离屏 PNG 渲染
- `HtmlExporter`：生成自包含 `index.html`（HTML/CSS/内联 SVG + JS 时间轴引擎）
- `SpecExporter`：生成 `UI_SPEC.md`、`AI_HANDOFF.md`、`design.json`
- 导出包目录结构：

```text
index.html
design.json
UI_SPEC.md
AI_HANDOFF.md
```

## 3. 数据流

```text
用户输入
  ├─ Canvas / Panels 修改 DesignDocument
  ├─ 面板发 documentEdited
  └─ MainWindow.onDocumentEdited
        ├─ Canvas.update()
        ├─ LayersPanel.reload()
        ├─ PropertiesPanel.refresh()
        ├─ TimelinePanel.refresh()
        └─ 标题/状态/动作状态更新

保存：DesignDocument.toJson() -> QSaveFile（原子写）
打开：QJsonDocument -> DesignDocument.fromJson() -> 画布 fit
导出：DesignDocument -> HTML + Markdown + JSON
```

动画预览有两套引擎：

1. **编辑器时间轴**：由 `TimelinePanel` 驱动，`CanvasWidget::setPreviewAnimation()` 使用 `sampleAnimation()` 渲染。
2. **交互预览 / 导出 HTML**：各自维护活动动画列表，逐帧采样，支持多动画叠加、循环与 `stopAll`。

## 4. 扩展点

- 新增元素类型：`ElementType` + `Renderer::shapePath` + HTML 导出映射 + 属性面板。
- 新增动画属性：`animatableProperties` + `Track` 采样 + Renderer overrides + HTML/JS 映射。
- 新增交互动作：`Interaction.action` + 预览执行器 + HTML 运行时。
- 新增文件字段：`Element/Animation` 的 JSON 序列化 + Schema 文档（旧文件缺失字段使用默认值）。

## 5. 构建

```bat
qmake ..\DeepDesignStudio.pro -spec win32-g++
mingw32-make -j8
```

产物：`bin\DeepDesignStudio.exe`。  
`deploy.bat` 使用 `windeployqt` 复制 Qt 运行库和插件，并附带 docs/examples。

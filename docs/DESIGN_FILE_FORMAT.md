# DeepDesign `.ddesign` 设计文件格式规范

版本：**1.0**  
格式标识：`format: "DeepDesign"`  
文件扩展名：`.ddesign`（本质是 UTF-8 JSON，也接受 `.json`）  
生成器：灵绘 DeepDesign Studio 1.0

> 这份文件是 **唯一事实来源**。人类设计师、渲染器、以及 DeepSeek 等 AI 都应读取同一份 JSON，而不是根据截图猜测。

---

## 1. 顶层结构

```json
{
  "format": "DeepDesign",
  "version": 1,
  "generator": "DeepDesign Studio 1.0",
  "title": "登录界面 · 极光蓝",
  "canvas": {
    "width": 1280,
    "height": 800,
    "background": "#0A0F1A"
  },
  "elements": [],
  "animations": [],
  "interactions": [],
  "guides": []
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `format` | string | 固定为 `"DeepDesign"` |
| `version` | number | 当前为 `1`，向后兼容读取 |
| `generator` | string | 生成工具，可忽略 |
| `title` | string | 设计标题 |
| `canvas.width` / `canvas.height` | number | 画板尺寸，单位 px |
| `canvas.background` | color | 画板背景 |
| `elements` | array | 元素数组，**从底到顶**（后面的覆盖前面的） |
| `animations` | array | 动画时间线 |
| `interactions` | array | 交互触发器 |
| `guides` | array | 参考线（设计辅助，不参与渲染） |

### 颜色表示

- 不透明颜色：`#RRGGBB`，例如 `#2D8CEB`
- 带透明度颜色：`#RRGGBBAA`，例如 `#2D8CEB33`
- 所有颜色均为 sRGB。设计软件内部使用 0–255 Alpha。

### 坐标和变换

- 画板左上角为 `(0, 0)`，X 向右，Y 向下，单位为 px。
- 每个元素有一个**未旋转**的矩形 `frame`：`x / y / width / height`。
- `rotation` 为角度，围绕元素矩形中心旋转，正值顺时针。
- `opacity` 为 `0.0–1.0` 的全局透明度。
- 元素自身的 `fill` / `stroke` 也可以带透明度，二者会相乘。

---

## 2. 元素（Element）

### 2.1 通用字段

| 字段 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `id` | string | 自动 | 文件内唯一，如 `rect_1` |
| `type` | string | — | `rect` / `ellipse` / `line` / `polygon` / `star` / `path` / `text` |
| `name` | string | 类型名 | 图层名称，AI 可用于推断语义 |
| `x`,`y`,`width`,`height` | number | — | 未旋转矩形框，px |
| `rotation` | number | `0` | 角度 |
| `opacity` | number | `1` | 0–1 |
| `fill` | color | — | 填充色；渐变时作为回退色 |
| `gradient` | object | 无 | 渐变填充，见 2.3 |
| `stroke` | color | — | 描边色 |
| `strokeWidth` | number | `0` | 描边宽度，px；0 表示无描边 |
| `visible` | boolean | `true` | 是否可见 |
| `locked` | boolean | `false` | 是否锁定（编辑器行为） |

### 2.2 各类型专属字段

#### `rect` 矩形

```json
{
  "type": "rect",
  "radius": 16
}
```

| 字段 | 说明 |
|---|---|
| `radius` | 圆角半径，px，自动限制在 `min(w,h)/2` 内 |

#### `ellipse` 椭圆

无专属字段；`fill`/`stroke` 作用于椭圆。

#### `line` 直线

```json
{
  "type": "line",
  "lineStart": [0, 0.5],
  "lineEnd": [1, 0.5]
}
```

- `lineStart` / `lineEnd` 是相对矩形框的**归一化坐标**（0–1）。
- 实际端点 = `(x + nx*w, y + ny*h)`。
- 线的颜色使用 `stroke`（若 `strokeWidth<=0` 则回退到 `fill`），最小显示宽度 1px。

#### `polygon` 多边形 / `star` 星形

```json
{
  "type": "polygon",
  "sides": 6
}
```

- `sides`：边数，3–60。
- `polygon` 顶点位于外接椭圆上，从正上方开始。
- `star` 有 `sides*2` 个顶点，外/内半径比为 `1 : 0.44`。
- 顶点坐标均按元素矩形框的外接椭圆计算。

#### `path` 自由路径

```json
{
  "type": "path",
  "closed": true,
  "points": [[0.05, 0.6], [0.25, 0.05], [0.85, 0.1]]
}
```

- `points`：归一化坐标数组。
- `closed`：是否闭合；开放路径仍可填充（浏览器/SVG 会隐式闭合）。
- 使用 QPainterPath / SVG polygon 渲染。

#### `text` 文本

```json
{
  "type": "text",
  "text": "欢迎回来",
  "fontFamily": "Microsoft YaHei UI",
  "fontSize": 36,
  "bold": true,
  "align": "center",
  "textColor": "#F4F7FF"
}
```

- `text` 支持换行 `\n`。
- `fontSize` 单位 px（CSS px）。
- `align`：`left` / `center` / `right`（水平）。
- 文本在矩形框内垂直居中，按词换行。
- `fill` 对文本不生效；文本颜色由 `textColor` 决定。

### 2.3 渐变填充 `gradient`

渐变是**可选**字段；存在且合法时覆盖 `fill`，`fill` 仍保留为回退色/主色。

```json
{
  "gradient": {
    "type": "linear",
    "angle": 135,
    "stops": [
      {"position": 0.0, "color": "#2D8CEB"},
      {"position": 0.45, "color": "#A47BFF"},
      {"position": 1.0, "color": "#FF6B81"}
    ]
  }
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `type` | string | `"linear"` 或 `"radial"` |
| `angle` | number | 线性渐变角度，单位度；`0` = 从左到右，`90` = 从上到下；径向渐变忽略 |
| `stops` | array | 至少 2 个颜色停止点，`position` 为 0–1 |

渲染规则：

- **线性**：渐变轴穿过元素中心，方向由角度决定；两端颜色覆盖元素包围盒。
- **径向**：圆心为元素中心，半径约 `max(width,height) * 0.62`。
- CSS 导出时线性角度需换算为 `angle + 90`（CSS 0° 朝上）；径向使用 `circle at 50% 50%`。
- SVG 渐变使用 `gradientUnits="userSpaceOnUse"`，坐标按元素矩形框计算。

> 渐变是静态填充。当前 `fill` 属性动画只对纯色填充生效；渐变元素上的 `fill` 轨道会被忽略。

---

## 3. 动画（Animation）

```json
{
  "id": "anim_intro",
  "name": "入场动画",
  "duration": 1.2,
  "loop": false,
  "easing": "easeOutCubic",
  "tracks": [
    {
      "elementId": "login_card",
      "property": "opacity",
      "keyframes": [
        {"time": 0, "value": 0},
        {"time": 0.55, "value": 1}
      ]
    }
  ]
}
```

| 字段 | 说明 |
|---|---|
| `duration` | 秒，>0 |
| `loop` | 是否循环 |
| `easing` | 整条动画统一缓动 |
| `tracks[].elementId` | 目标元素 id |
| `tracks[].property` | 动画属性，见下 |
| `keyframes[].time` | 相对动画开始的秒数 |
| `keyframes[].value` | number 或颜色字符串（fill） |

### 可动画属性

| property | 值类型 | 说明 |
|---|---|---|
| `x`,`y` | number | 绝对位置，px |
| `width`,`height` | number | 绝对尺寸，px |
| `rotation` | number | 绝对角度 |
| `opacity` | number | 0–1 |
| `fill` | color | 纯色填充颜色（渐变元素忽略） |
| `strokeWidth` | number | 描边宽度 |

### 采样规则

- 关键帧按 `time` 升序。
- 时间在关键帧之间时，使用 `easing` 对区间归一化时间做插值。
- 数值线性插值；颜色在 RGB(A) 空间线性插值。
- 时间超出最后一个关键帧时保持最后一帧的值。
- 动画结束后保持最后一帧（fill-mode both 语义），直到停止/重播。
- 多个动画同时作用同一属性时，**后触发的动画覆盖先触发的**。

### 缓动 id

`linear`、`easeInQuad`、`easeOutQuad`、`easeInOutCubic`、`easeOutCubic`、`easeOutBack`、`easeOutElastic`、`easeOutBounce`。

---

## 4. 交互（Interaction）

```json
{
  "sourceId": "login_button",
  "trigger": "click",
  "action": "playAnimation",
  "animationId": "anim_login_click"
}
```

| 字段 | 说明 |
|---|---|
| `sourceId` | 触发的元素 id |
| `trigger` | `click` / `hover` / `leave` / `load` |
| `action` | `playAnimation` / `stopAll` |
| `animationId` | `playAnimation` 时必填 |

触发语义：

- `click`：鼠标点击 / 键盘 Enter、Space（无障碍）。
- `hover`：鼠标进入元素。
- `leave`：鼠标离开元素。
- `load`：页面/预览加载后自动触发。
- `playAnimation`：再次触发时从头重播；可同时播放多个动画。
- `stopAll`：停止全部动画并恢复元素初始样式。

---

## 5. 参考线（Guide）

参考线用于**手动测量和手动对齐**，不参与最终界面渲染。

```json
{
  "id": "guide_1",
  "orientation": "horizontal",
  "position": 220
}
```

| 字段 | 说明 |
|---|---|
| `orientation` | `horizontal`（水平线，y = position）或 `vertical`（垂直线，x = position） |
| `position` | 画板坐标，px |

- 水平参考线：`y = position`。
- 垂直参考线：`x = position`。
- 导出 HTML 时忽略参考线；AI 可以将其作为布局对齐依据。
- 在编辑器中可从上方/左侧标尺拖出参考线；拖出画板外可删除。

---

## 6. 图层顺序与语义

1. `elements` 数组顺序 = 从底到顶。
2. 后绘制的元素覆盖先绘制的元素。
3. 建议 `name` 使用业务语义，例如「登录按钮」「标题」「播放列表」，方便 AI 推断控件类型与无障碍标签。
4. 元素的 `id` 在文件内唯一且稳定，动画/交互通过 id 引用。

---

## 7. 完整示例

```json
{
  "format": "DeepDesign",
  "version": 1,
  "title": "示例",
  "canvas": {"width": 800, "height": 600, "background": "#0B1018"},
  "elements": [
    {
      "id": "card",
      "type": "rect",
      "name": "渐变卡片",
      "x": 120, "y": 100, "width": 560, "height": 400,
      "rotation": 0, "opacity": 1,
      "fill": "#2D8CEB",
      "gradient": {
        "type": "linear",
        "angle": 135,
        "stops": [
          {"position": 0, "color": "#2D8CEB"},
          {"position": 1, "color": "#FF6B81"}
        ]
      },
      "stroke": "#8CB4FF",
      "strokeWidth": 2,
      "radius": 28,
      "visible": true,
      "locked": false
    }
  ],
  "animations": [],
  "interactions": [],
  "guides": [
    {"id": "guide_1", "orientation": "horizontal", "position": 220}
  ]
}
```

---

## 8. 读取与兼容性

- 读取时忽略未知字段；缺失字段使用默认值。
- `version > 1` 时，当前软件会拒绝打开并提示版本过高。
- 写入使用 UTF-8、缩进 JSON，便于 diff、代码审查与 AI 读取。
- 保存使用原子写入（`QSaveFile`），避免中断损坏原文件。

---

## 9. 参考实现

- 编辑器：Qt 6 / C++17，`QPainter` 渲染，`QJsonObject` 序列化。
- 导出：自包含 `index.html`（CSS/SVG + requestAnimationFrame），`design.json`，`UI_SPEC.md`，`AI_HANDOFF.md`。

# AI 交付指南：让 DeepSeek 按 `.ddesign` 实现界面

这份指南配合：

- `design.json` / `*.ddesign`：唯一事实来源
- `UI_SPEC.md`：自动生成的颜色、图层、交互、动画规范
- `AI_HANDOFF.md`：导出包中的任务说明与提示词
- `index.html`：浏览器可运行的参考实现

目标：让 DeepSeek 不靠截图猜测，而能按设计数据**可复现地**生成 UI 与交互动效。

---

## 1. 推荐工作流

1. 从灵绘 DeepDesign Studio 导出 AI 交付包。
2. 把整个文件夹发给 DeepSeek（或先粘贴 `design.json`，再附 `AI_HANDOFF.md`）。
3. 让 DeepSeek 先阅读 `design.json`，再阅读 `UI_SPEC.md`。
4. 指定目标技术栈（React/Vue/HTML/Flutter/Qt）。
5. 要求它输出可直接运行的代码，并逐条核对校验清单。
6. 用 `index.html` 作为视觉与交互的参考基准。

---

## 2. 给 DeepSeek 的提示词模板

```text
你是资深 UI 工程师。请严格根据 design.json（DeepDesign 格式）和 UI_SPEC.md，
实现一个可运行的交互界面。

【项目】<title>
【画布】<width> × <height> px，背景 <background>
【目标技术栈】<React + CSS / Vue / 原生 HTML+CSS+SVG / Flutter / Qt Widgets>

硬性要求：
1. design.json 是唯一事实来源，不要臆造布局、文案或颜色。
2. elements 数组从底到顶，按绝对坐标定位（left/top/width/height，px）。
3. rotation 围绕元素中心旋转（transform-origin: center）。
4. 矩形/椭圆用 CSS 绘制；直线/多边形/星形/自由路径用内联 SVG；文本使用真实文本节点。
5. gradient 字段存在时使用渐变填充；fill 仅作为回退色。
6. 动画属性是绝对值；用 requestAnimationFrame 或 Web Animations API 实现时间轴采样；
   支持 loop、easing、多动画叠加；结束后保持最后一帧；stopAll 恢复初始状态。
7. 交互支持 click / hover / leave / load；动作 playAnimation / stopAll。
8. 画板整体等比缩放到视口，内部坐标不可改变。
9. 交互元素加语义化标签、tabindex、Enter/Space 支持，文本必须可选中。
10. 输出完整可运行代码，并说明你如何验证。

先阅读 design.json，再实现。不要忽略 gradient、guides、animations 和 interactions。
```

---

## 3. 数据映射表

### 3.1 元素类型

| `.ddesign` | HTML / CSS | SVG | React / Vue |
|---|---|---|---|
| `rect` | `<div>` + `background` + `border-radius` + `border` | — | 组件 + style |
| `ellipse` | `<div>` + `border-radius:50%` | — | 组件 + style |
| `line` | — | `<line>`，`stroke-linecap=round` | `<svg>` 子组件 |
| `polygon` | — | `<polygon>`，按外接椭圆计算顶点 | `<svg>` 子组件 |
| `star` | — | `<polygon>`，外/内半径比 1:0.44，`sides*2` 顶点 | `<svg>` 子组件 |
| `path` | — | `<polygon>`/`<polyline>`，points 为归一化坐标 × 宽高 | `<svg>` 子组件 |
| `text` | `<div>` + flex 垂直居中 + `white-space:pre-wrap` | — | 文本组件 |

### 3.2 变换

```css
.layer {
  position: absolute;
  left: var(--x); top: var(--y);
  width: var(--w); height: var(--h);
  opacity: var(--opacity);
  transform: rotate(var(--rotation));
  transform-origin: center;
}
```

### 3.3 渐变

```css
background: linear-gradient(225deg, #2D8CEB 0%, #A47BFF 45%, #FF6B81 100%);
/* 或 */
background: radial-gradient(circle at 50% 50%, #2D8CEB 0%, #0F172A 100%);
```

- 设计文件角度 `angle` → CSS 角度 `angle + 90`。
- 0° = 左→右，90° = 上→下。
- `stops[].position` 为 0–1，CSS 中乘 100 加 `%`。
- SVG 图形需要在 `<defs>` 中定义 `<linearGradient>` / `<radialGradient>`，
  使用 `gradientUnits="userSpaceOnUse"`，并用 `fill="url(#id)"` 引用。
- 颜色可能是 `#RRGGBB` 或 `#RRGGBBAA`，CSS/SVG 均支持。

### 3.4 动画

| 属性 | 映射 |
|---|---|
| `x` / `y` | `left` / `top` |
| `width` / `height` | `width` / `height` |
| `rotation` | `transform: rotate()` |
| `opacity` | `opacity` |
| `fill` | `background-color` 或 SVG `fill`（渐变元素忽略） |
| `strokeWidth` | `border-width` 或 SVG `stroke-width` |

动画采样伪代码：

```js
for (const track of activeAnimation.tracks) {
  const value = sample(track.keyframes, time, animation.easing);
  apply(track.elementId, track.property, value);
}
```

- 多个动画同时播放时，按触发顺序应用，后者覆盖同属性。
- 动画结束后保持最后一帧。
- `stopAll` 恢复元素初始值（从 `design.json` 重新应用基础样式）。

### 3.5 交互

| `trigger` | 浏览器事件 |
|---|---|
| `click` | `click`；同时监听 Enter/Space |
| `hover` | `mouseenter` |
| `leave` | `mouseleave` |
| `load` | DOMContentLoaded 后延迟约 50–100ms 执行 |

| `action` | 行为 |
|---|---|
| `playAnimation` | 播放 `animationId`；重复触发从头播 |
| `stopAll` | 停止所有动画并恢复初始状态 |

### 3.6 参考线

- `guides` 仅用于手动测量/对齐，**不要渲染到最终界面**。
- 可以把参考线位置作为栅格/间距依据，但不要生成可见线。
- 水平参考线：`y = position`；纵向：`x = position`。

---

## 4. 布局策略

设计使用**固定画板 + 绝对定位**。实现时：

1. 创建固定尺寸 artboard（如 `1280 × 800`），内部 `position: relative`。
2. 元素绝对定位，`z-index` 由数组顺序决定。
3. 外层容器按 `min(viewportWidth/width, viewportHeight/height)` 等比缩放 artboard。
4. 不要使用 flex/grid 重排设计元素的位置，否则会破坏像素级还原。
5. 需要响应式时，整体缩放而不是改变内部坐标。

---

## 5. 语义与可访问性

- 图层名包含「按钮」「btn」「button」「输入」「标题」「导航」时，优先使用对应语义标签。
- 交互元素满足：
  - 可获得焦点（`tabindex="0"` 或原生控件）；
  - Enter/Space 可触发 click 交互；
  - 有可读的 `aria-label` 或文本内容；
  - `cursor: pointer`。
- 文本必须是真实文本节点，可选中、可缩放。
- 不要用背景图片替代文本或图形。

---

## 6. 校验清单

- [ ] 画布尺寸、背景与 `design.json` 一致。
- [ ] 元素 z 顺序、位置、尺寸、旋转一致。
- [ ] 渐变方向、色标位置与颜色一致。
- [ ] 文本内容、字体、字号、字重、对齐、颜色一致。
- [ ] 交互触发方式与目标动画正确。
- [ ] 动画时长、缓动、循环、关键帧值一致。
- [ ] 动画结束后保持最终帧；`stopAll` 正确恢复。
- [ ] 画板等比适配视口，无裁切、无错位。
- [ ] 无控制台错误、无外部资源 404。
- [ ] 键盘可操作交互元素，文本可选中。
- [ ] 参考线未被渲染成可见元素。

---

## 7. 常见错误

1. **把绝对坐标改成 flex 布局**：导致整体错位。
2. **旋转中心错误**：必须 `transform-origin: center`。
3. **把动画值当偏移量**：它们是绝对值，例如 `x: 120` 表示 `left: 120px`。
4. **忘记多动画叠加**：同一属性后触发的动画应覆盖前者。
5. **渐变角度直接抄 CSS**：需要 `angle + 90`。
6. **把参考线画出来**：它们只是设计辅助。
7. **丢失文本**：文本必须是真实节点，不能栅格化。
8. **忽略 `visible/locked`**：`visible=false` 的图层不应渲染。

# 渐变与参考线测试 · UI 交付规范

> 由 **灵绘 DeepDesign Studio** 自动生成。此文件与 `design.json` 配套，`design.json` 是唯一事实来源，本文件用于快速理解与实现。

| 属性 | 值 |
|---|---|
| 画布尺寸 | 800 × 600 px |
| 画板背景 | `#0B1018` |
| 元素数量 | 2 |
| 动画数量 | 0 |
| 交互数量 | 0 |
| 参考线数量 | 2 |
| 坐标系 | 左上角为原点，X 向右，Y 向下，单位为 px |
| 图层顺序 | 数组顺序从底到顶，`z` 越大越靠前 |

## 1. 设计令牌（Design Tokens）

### 1.1 颜色

| 色值 | 用途 |
|---|---|
| `#0B1018` | 画板背景 |
| `#2D8CEB` | 填充 / 线性渐变 |
| `#8CB4FF` | 描边 |
| `#A47BFF` | 线性渐变 |
| `#FF6B81` | 线性渐变 |
| `#FFFFFF` | 文字 |

### 1.2 字体

| 字体 | 字号 | 字重 |
|---|---|---|
| Microsoft YaHei UI | 40px | 400 / 700 |

### 1.3 CSS 变量建议

```css
:root {
  --canvas-width: 800px;
  --canvas-height: 600px;
  --color-1: #0B1018; /* 画板背景 */
  --color-2: #2D8CEB; /* 填充 / 线性渐变 */
  --color-3: #8CB4FF; /* 描边 */
  --color-4: #A47BFF; /* 线性渐变 */
  --color-5: #FF6B81; /* 线性渐变 */
  --color-6: #FFFFFF; /* 文字 */
}
```

## 2. 图层与布局（从底到顶）

| z | ID | 类型 | 名称 | 位置 / 尺寸 | 旋转 | 不透明度 | 样式 | 内容 |
|---|---|---|---|---|---|---|---|---|
| 1 | `gradient_card` | 矩形 | 渐变卡片 | x=120 y=100 w=560 h=400 | 0° | 100% | 线性渐变 135° [#2D8CEB@0% → #A47BFF@50% → #FF6B81@100%]; stroke `#8CB4FF` / 2px; radius 28px |  |
| 2 | `gradient_title` | 文本 | 渐变标题 | x=170 y=240 w=460 h=80 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 40px bold / #FFFFFF | “Gradient & Guides” |

### 2.1 参考线（手动测量 / 对齐）

参考线是设计辅助线，不参与最终渲染；用于手动测量和视觉对齐。

| ID | 方向 | 位置 |
|---|---|---|
| `guide_1` | 横向 | Y = 220 px |
| `guide_2` | 纵向 | X = 300 px |

## 3. 交互（Interactions）

没有定义交互。

## 4. 动效时间线（Animations）

没有定义动画。

## 5. 实现指导（框架无关）

### 5.1 画布与布局

- 创建一个宽 `800px`、高 `600px` 的定尺画板，背景 `#0B1018`，`overflow: hidden`。
- 所有元素按 **绝对定位** 实现，使用上表中的 `x/y/width/height`。
- 元素以中心点为旋转原点：`transform: rotate(<rotation>deg); transform-origin: center;`。
- 为适配不同屏幕，将画板整体等比缩放到容器内，不要改变内部绝对坐标。
- 图层顺序遵循 z 值，后面的元素覆盖前面的元素。

### 5.2 元素映射

| 设计类型 | HTML / CSS / SVG 实现 |
|---|---|
| 矩形 rect | `div` + `border-radius` + `background` + `border` |
| 椭圆 ellipse | `div` + `border-radius: 50%` |
| 直线 line | 内联 `svg > line`，使用绝对宽高和 `stroke-linecap: round` |
| 多边形 polygon | 内联 `svg > polygon`，按外接椭圆均分顶点 |
| 星形 star | 内联 `svg > polygon`，外/内半径比 1 : 0.44，顶点数 = sides × 2 |
| 自由路径 path | 内联 `svg > polygon`（闭合）或 `polyline`（开放），points 为归一化坐标 × 宽高 |
| 文本 text | `div` + flex 垂直居中 + `white-space: pre-wrap` + 指定字体/字号/字重/颜色/对齐 |
| 渐变填充 gradient | CSS `linear-gradient` / `radial-gradient`；SVG 使用 `<linearGradient>` / `<radialGradient>` |

### 5.3 交互实现

- `click` → 点击；`hover` → 鼠标进入；`leave` → 鼠标离开；`load` → 页面加载后自动执行。
- `playAnimation`：播放对应动画，重复触发时从头重播；可多个动画同时播放，后触发的覆盖先触发的同属性。
- `stopAll`：停止所有动画并恢复元素初始样式。
- 交互对象应设置 `cursor: pointer`，并保留键盘可访问性（为按钮类元素补充 role / tabindex）。

### 5.4 动效实现

- 动画属性均为 **绝对值**：x/y/width/height 单位 px，rotation 单位度，opacity 范围 0–1，fill 为颜色。
- 关键帧之间按缓动曲线插值；时长单位为秒；`loop=true` 时循环播放。
- 推荐使用 `requestAnimationFrame` 统一采样（与设计软件行为一致），或使用 Web Animations API。
- 缓动对应关系：linear, easeInQuad, easeOutQuad, easeInOutCubic, easeOutCubic, easeOutBack, easeOutElastic, easeOutBounce。
- 动效结束后保持最后一帧（fill-mode: both 的语义），直到 stopAll 或重播。

### 5.5 渐变填充

- `gradient.type`：`linear`（线性）或 `radial`（径向）；`gradient.angle` 为角度，0° 表示从左到右，90° 表示从上到下。
- `gradient.stops` 为颜色停止点数组，`position` 范围 0–1，按位置从 0 到 1 排列。
- CSS 线性渐变角度需换算为 `angle + 90`（CSS 0° 朝上）；径向渐变以元素中心为圆心。
- 渐变填充元素仍保留 `fill` 字段作为回退色；`fill` 动画对渐变元素不生效。

### 5.5 语义与可访问性

- 根据图层名称推断语义：包含“按钮/button/btn”的交互元素应输出 `<button>` 或带 `role=button` 的可聚焦元素。
- 文本应使用真实文本节点，不要转成图片；保持选中与缩放能力。
- 交互元素需支持 Enter/Space 触发 click 交互。

## 6. 校验清单

- [ ] 画布尺寸、背景色与设计一致
- [ ] 所有元素的位置、尺寸、旋转、图层顺序与设计一致
- [ ] 所有文本内容、字体、字号、颜色与设计一致
- [ ] 交互触发方式与目标动画正确
- [ ] 动画时长、缓动、循环和关键帧值与设计一致
- [ ] 画板在目标设备上等比缩放，无裁切或错位

---

生成时间：2026-11-09 23:51:17

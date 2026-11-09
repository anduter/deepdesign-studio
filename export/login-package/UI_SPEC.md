# 登录界面 · 极光蓝 · UI 交付规范

> 由 **灵绘 DeepDesign Studio** 自动生成。此文件与 `design.json` 配套，`design.json` 是唯一事实来源，本文件用于快速理解与实现。

| 属性 | 值 |
|---|---|
| 画布尺寸 | 1280 × 800 px |
| 画板背景 | `#0A0F1A` |
| 元素数量 | 24 |
| 动画数量 | 3 |
| 交互数量 | 3 |
| 参考线数量 | 0 |
| 坐标系 | 左上角为原点，X 向右，Y 向下，单位为 px |
| 图层顺序 | 数组顺序从底到顶，`z` 越大越靠前 |

## 1. 设计令牌（Design Tokens）

### 1.1 颜色

| 色值 | 用途 |
|---|---|
| `#0A0F1A` | 画板背景 |
| `#0A1019` | 填充 |
| `#0D1421` | 线性渐变 |
| `#10233A` | 填充 |
| `#111A2A` | 线性渐变 |
| `#121A2B` | 填充 |
| `#12335F` | 填充 |
| `#1A2740` | 线性渐变 |
| `#1A2B46` | 填充 |
| `#23304A` | 描边 |
| `#26324A` | 填充 / 描边 |
| `#2A3854` | 描边 |
| `#2B1A4A` | 填充 |
| `#2D8CEB` | 填充 / 线性渐变 |
| `#3DDC97` | 填充 |
| `#5AA9F0` | 填充 / 描边 / 文字 / 线性渐变 |
| `#70A9F2` | 描边 |
| `#7E8CA3` | 文字 |
| `#8B97AC` | 文字 |
| `#A47BFF` | 填充 |
| `#C7D2E3` | 文字 |
| `#F4F7FF` | 文字 |
| `#FFFFFF` | 文字 |

### 1.2 字体

| 字体 | 字号 | 字重 |
|---|---|---|
| Microsoft YaHei UI | 12px, 14px, 15px, 17px, 36px | 400 / 700 |

### 1.3 CSS 变量建议

```css
:root {
  --canvas-width: 1280px;
  --canvas-height: 800px;
  --color-1: #0A0F1A; /* 画板背景 */
  --color-2: #0A1019; /* 填充 */
  --color-3: #0D1421; /* 线性渐变 */
  --color-4: #10233A; /* 填充 */
  --color-5: #111A2A; /* 线性渐变 */
  --color-6: #121A2B; /* 填充 */
  --color-7: #12335F; /* 填充 */
  --color-8: #1A2740; /* 线性渐变 */
  --color-9: #1A2B46; /* 填充 */
  --color-10: #23304A; /* 描边 */
  --color-11: #26324A; /* 填充 / 描边 */
  --color-12: #2A3854; /* 描边 */
  --color-13: #2B1A4A; /* 填充 */
  --color-14: #2D8CEB; /* 填充 / 线性渐变 */
  --color-15: #3DDC97; /* 填充 */
  --color-16: #5AA9F0; /* 填充 / 文字 / 线性渐变 / 描边 */
  --color-17: #70A9F2; /* 描边 */
  --color-18: #7E8CA3; /* 文字 */
  --color-19: #8B97AC; /* 文字 */
  --color-20: #A47BFF; /* 填充 */
  --color-21: #C7D2E3; /* 文字 */
  --color-22: #F4F7FF; /* 文字 */
  --color-23: #FFFFFF; /* 文字 */
}
```

## 2. 图层与布局（从底到顶）

| z | ID | 类型 | 名称 | 位置 / 尺寸 | 旋转 | 不透明度 | 样式 | 内容 |
|---|---|---|---|---|---|---|---|---|
| 1 | `glow_blue` | 椭圆 | 蓝色光晕 | x=-170 y=-190 w=650 h=650 | 0° | 55% | fill `#12335F` |  |
| 2 | `glow_violet` | 椭圆 | 紫色光晕 | x=900 y=480 w=500 h=500 | 0° | 55% | fill `#2B1A4A` |  |
| 3 | `wave` | 自由路径 | 底部波纹 | x=0 y=620 w=1280 h=180 | 0° | 70% | fill `#10233A` | 9 个锚点，闭合 |
| 4 | `deco_star_1` | 星形 | 装饰星 | x=1030 y=170 w=44 h=44 | 12° | 50% | fill `#5AA9F0`; sides 5 |  |
| 5 | `deco_star_2` | 星形 | 装饰星 2 | x=250 y=610 w=26 h=26 | -12° | 60% | fill `#A47BFF`; sides 5 |  |
| 6 | `deco_hex` | 多边形 | 装饰六边形 | x=1060 y=640 w=92 h=92 | 18° | 80% | fill `#1A2B46`; sides 6 |  |
| 7 | `login_card` | 矩形 | 登录卡片 | x=380 y=90 w=520 h=620 | 0° | 100% | 线性渐变 135° [#1A2740@0% → #111A2A@60% → #0D1421@100%]; stroke `#23304A` / 1.5px; radius 28px |  |
| 8 | `logo_card` | 矩形 | 品牌标记 | x=604 y=126 w=72 h=72 | 0° | 100% | 线性渐变 135° [#5AA9F0@0% → #2D8CEB@100%]; stroke `#5AA9F0` / 1px; radius 18px |  |
| 9 | `logo_letter` | 文本 | 品牌字母 | x=604 y=126 w=72 h=72 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 36px bold / #FFFFFF | “D” |
| 10 | `title` | 文本 | 欢迎标题 | x=380 y=232 w=520 h=52 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 36px bold / #F4F7FF | “欢迎回来” |
| 11 | `subtitle` | 文本 | 副标题 | x=380 y=292 w=520 h=28 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 14px / #8B97AC | “登录以继续使用灵绘云设计工作台” |
| 12 | `email_label` | 文本 | 邮箱标签 | x=450 y=334 w=200 h=22 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 12px / #7E8CA3 | “邮箱地址” |
| 13 | `email_input` | 矩形 | 邮箱输入框 | x=450 y=360 w=380 h=54 | 0° | 100% | fill `#0A1019`; stroke `#2A3854` / 1px; radius 12px |  |
| 14 | `email_dot` | 椭圆 | 邮箱状态点 | x=466 y=382 w=8 h=8 | 0° | 100% | fill `#3DDC97` |  |
| 15 | `email_text` | 文本 | 邮箱内容 | x=486 y=360 w=322 h=54 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 14px / #C7D2E3 | “hello@deepdesign.ai” |
| 16 | `password_label` | 文本 | 密码标签 | x=450 y=430 w=200 h=22 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 12px / #7E8CA3 | “密码” |
| 17 | `password_input` | 矩形 | 密码输入框 | x=450 y=456 w=380 h=54 | 0° | 100% | fill `#0A1019`; stroke `#2A3854` / 1px; radius 12px |  |
| 18 | `password_text` | 文本 | 密码占位 | x=470 y=456 w=330 h=54 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 15px / #C7D2E3 | “••••••••••” |
| 19 | `show_password` | 文本 | 显示密码 | x=738 y=456 w=76 h=54 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 12px / #5AA9F0 | “显示” |
| 20 | `login_button` | 矩形 | 登录按钮 | x=450 y=548 w=380 h=58 | 0° | 100% | fill `#2D8CEB`; stroke `#70A9F2` / 1px; radius 14px |  |
| 21 | `login_button_text` | 文本 | 登录按钮文字 | x=450 y=548 w=380 h=58 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 17px bold / #FFFFFF | “登 录” |
| 22 | `forgot_password` | 文本 | 忘记密码 | x=690 y=620 w=140 h=22 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 12px / #7E8CA3 | “忘记密码？” |
| 23 | `divider` | 直线 | 分隔线 | x=450 y=654 w=380 h=1 | 0° | 100% | fill `#26324A`; stroke `#26324A` / 1px | 方向 (0,0) → (1,0) |
| 24 | `register_hint` | 文本 | 注册提示 | x=380 y=672 w=520 h=24 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 12px / #8B97AC | “还没有账号？ 立即注册” |

### 2.1 参考线（手动测量 / 对齐）

无参考线。

## 3. 交互（Interactions）

| 触发对象 | 触发方式 | 动作 | 动画 |
|---|---|---|---|
| 登录卡片 (`login_card`) | 载入时 | 播放动画 | 入场动画 (`anim_intro`) |
| 登录按钮 (`login_button`) | 悬停 | 播放动画 | 按钮悬停 (`anim_login_hover`) |
| 登录按钮 (`login_button`) | 点击 | 播放动画 | 登录点击反馈 (`anim_login_click`) |

## 4. 动效时间线（Animations）

### 入场动画 (`anim_intro`)

- 时长：**1.2s**
- 缓动：**平滑减速** (`easeOutCubic`)
- 循环：**否**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 登录卡片 (`login_card`) | 不透明度 | 0s → `0` · 0.55s → `1` |
| 登录卡片 (`login_card`) | Y 位置 | 0s → `118` · 0.7s → `90` |
| 品牌标记 (`logo_card`) | 旋转 | 0s → `-90` · 0.75s → `0` |
| 品牌标记 (`logo_card`) | 不透明度 | 0s → `0` · 0.45s → `1` |
| 品牌字母 (`logo_letter`) | 不透明度 | 0s → `0` · 0.55s → `1` |
| 欢迎标题 (`title`) | Y 位置 | 0.1s → `266` · 0.75s → `232` |
| 欢迎标题 (`title`) | 不透明度 | 0.1s → `0` · 0.75s → `1` |
| 副标题 (`subtitle`) | Y 位置 | 0.18s → `324` · 0.85s → `292` |
| 副标题 (`subtitle`) | 不透明度 | 0.18s → `0` · 0.85s → `1` |
| 邮箱输入框 (`email_input`) | X 位置 | 0.3s → `420` · 0.9s → `450` |
| 邮箱输入框 (`email_input`) | 不透明度 | 0.3s → `0` · 0.9s → `1` |
| 密码输入框 (`password_input`) | X 位置 | 0.38s → `410` · 0.98s → `450` |
| 密码输入框 (`password_input`) | 不透明度 | 0.38s → `0` · 0.98s → `1` |
| 登录按钮 (`login_button`) | Y 位置 | 0.45s → `580` · 1.05s → `548` |
| 登录按钮 (`login_button`) | 不透明度 | 0.45s → `0` · 1.05s → `1` |
| 登录按钮文字 (`login_button_text`) | 不透明度 | 0.55s → `0` · 1.1s → `1` |

### 按钮悬停 (`anim_login_hover`)

- 时长：**0.32s**
- 缓动：**平滑减速** (`easeOutCubic`)
- 循环：**否**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 登录按钮 (`login_button`) | 填充颜色 | 0s → `#2D8CEB` · 0.32s → `#3D9CFA` |
| 登录按钮 (`login_button`) | X 位置 | 0s → `450` · 0.32s → `446` |
| 登录按钮 (`login_button`) | 宽度 | 0s → `380` · 0.32s → `388` |
| 登录按钮 (`login_button`) | Y 位置 | 0s → `548` · 0.32s → `544` |
| 登录按钮 (`login_button`) | 高度 | 0s → `58` · 0.32s → `66` |

### 登录点击反馈 (`anim_login_click`)

- 时长：**0.55s**
- 缓动：**缓入缓出** (`easeInOutCubic`)
- 循环：**否**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 登录按钮 (`login_button`) | 填充颜色 | 0s → `#2D8CEB` · 0.22s → `#3DDC97` · 0.36s → `#3DDC97` · 0.55s → `#2D8CEB` |
| 登录卡片 (`login_card`) | 旋转 | 0s → `0` · 0.18s → `-0.8` · 0.36s → `0.8` · 0.55s → `0` |

## 5. 实现指导（框架无关）

### 5.1 画布与布局

- 创建一个宽 `1280px`、高 `800px` 的定尺画板，背景 `#0A0F1A`，`overflow: hidden`。
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

# 音乐播放器 · Aurora · UI 交付规范

> 由 **灵绘 DeepDesign Studio** 自动生成。此文件与 `design.json` 配套，`design.json` 是唯一事实来源，本文件用于快速理解与实现。

| 属性 | 值 |
|---|---|
| 画布尺寸 | 1280 × 800 px |
| 画板背景 | `#0A0D14` |
| 元素数量 | 47 |
| 动画数量 | 4 |
| 交互数量 | 5 |
| 参考线数量 | 0 |
| 坐标系 | 左上角为原点，X 向右，Y 向下，单位为 px |
| 图层顺序 | 数组顺序从底到顶，`z` 越大越靠前 |

## 1. 设计令牌（Design Tokens）

### 1.1 颜色

| 色值 | 用途 |
|---|---|
| `#0A0D14` | 画板背景 |
| `#0C111C` | 线性渐变 |
| `#0D1420` | 填充 |
| `#0F1725` | 填充 / 线性渐变 |
| `#101724` | 线性渐变 |
| `#111826` | 填充 |
| `#13325C` | 填充 |
| `#151F30` | 线性渐变 |
| `#161F2F` | 填充 |
| `#16233A` | 填充 |
| `#182234` | 填充 |
| `#1B2536` | 填充 / 描边 |
| `#1B2638` | 线性渐变 |
| `#1B2A45` | 填充 |
| `#1E2A3D` | 描边 |
| `#1F2B40` | 描边 |
| `#233A5C` | 线性渐变 |
| `#24324A` | 描边 |
| `#26324A` | 填充 |
| `#2A3854` | 描边 |
| `#2D8CEB` | 填充 / 描边 |
| `#3B1F4E` | 填充 |
| `#5AA9F0` | 填充 / 文字 |
| `#6F7B90` | 文字 |
| `#70A9F2` | 描边 |
| `#7E8CA3` | 文字 |
| `#8B97AC` | 文字 |
| `#A47BFF` | 填充 |
| `#C7D2E3` | 填充 / 文字 |
| `#EAF3FF` | 填充 |
| `#F4F7FF` | 文字 |
| `#FFB454` | 填充 |
| `#FFFFFF` | 填充 / 描边 |

### 1.2 字体

| 字体 | 字号 | 字重 |
|---|---|---|
| Microsoft YaHei UI | 11px, 12px, 13px, 14px, 15px, 23px, 36px | 400 / 700 |

### 1.3 CSS 变量建议

```css
:root {
  --canvas-width: 1280px;
  --canvas-height: 800px;
  --color-1: #0A0D14; /* 画板背景 */
  --color-2: #0C111C; /* 线性渐变 */
  --color-3: #0D1420; /* 填充 */
  --color-4: #0F1725; /* 填充 / 线性渐变 */
  --color-5: #101724; /* 线性渐变 */
  --color-6: #111826; /* 填充 */
  --color-7: #13325C; /* 填充 */
  --color-8: #151F30; /* 线性渐变 */
  --color-9: #161F2F; /* 填充 */
  --color-10: #16233A; /* 填充 */
  --color-11: #182234; /* 填充 */
  --color-12: #1B2536; /* 填充 / 描边 */
  --color-13: #1B2638; /* 线性渐变 */
  --color-14: #1B2A45; /* 填充 */
  --color-15: #1E2A3D; /* 描边 */
  --color-16: #1F2B40; /* 描边 */
  --color-17: #233A5C; /* 线性渐变 */
  --color-18: #24324A; /* 描边 */
  --color-19: #26324A; /* 填充 */
  --color-20: #2A3854; /* 描边 */
  --color-21: #2D8CEB; /* 填充 / 描边 */
  --color-22: #3B1F4E; /* 填充 */
  --color-23: #5AA9F0; /* 填充 / 文字 */
  --color-24: #6F7B90; /* 文字 */
  --color-25: #70A9F2; /* 描边 */
  --color-26: #7E8CA3; /* 文字 */
  --color-27: #8B97AC; /* 文字 */
  --color-28: #A47BFF; /* 填充 */
  --color-29: #C7D2E3; /* 填充 / 文字 */
  --color-30: #EAF3FF; /* 填充 */
  --color-31: #F4F7FF; /* 文字 */
  --color-32: #FFB454; /* 填充 */
  --color-33: #FFFFFF; /* 填充 / 描边 */
}
```

## 2. 图层与布局（从底到顶）

| z | ID | 类型 | 名称 | 位置 / 尺寸 | 旋转 | 不透明度 | 样式 | 内容 |
|---|---|---|---|---|---|---|---|---|
| 1 | `bg_glow_1` | 椭圆 | 背景光晕 1 | x=-220 y=-240 w=700 h=700 | 0° | 50% | fill `#13325C` |  |
| 2 | `bg_glow_2` | 椭圆 | 背景光晕 2 | x=900 y=420 w=620 h=620 | 0° | 45% | fill `#3B1F4E` |  |
| 3 | `bg_star` | 星形 | 背景星点 | x=1110 y=130 w=60 h=60 | 14° | 55% | fill `#FFB454`; sides 5 |  |
| 4 | `player_card` | 矩形 | 播放器主卡片 | x=100 y=86 w=1080 h=628 | 0° | 100% | 线性渐变 135° [#151F30@0% → #0C111C@100%]; stroke `#1F2B40` / 1.5px; radius 30px |  |
| 5 | `album_card` | 矩形 | 专辑封面卡片 | x=136 y=122 w=396 h=556 | 0° | 100% | 线性渐变 90° [#1B2638@0% → #101724@100%]; stroke `#24324A` / 1px; radius 24px |  |
| 6 | `cover` | 矩形 | 专辑封面 | x=164 y=150 w=340 h=340 | 0° | 100% | 线性渐变 135° [#233A5C@0% → #0F1725@100%]; stroke `#2D8CEB` / 1.5px; radius 20px |  |
| 7 | `vinyl` | 椭圆 | 唱片 | x=204 y=190 w=260 h=260 | 0° | 100% | fill `#2D8CEB` |  |
| 8 | `vinyl_ring` | 椭圆 | 唱片纹路 | x=236 y=222 w=196 h=196 | 0° | 95% | fill `#0F1725`; stroke `#FFFFFF` / 3px |  |
| 9 | `vinyl_hole` | 椭圆 | 唱片中心 | x=300 y=286 w=68 h=68 | 0° | 100% | fill `#EAF3FF` |  |
| 10 | `album_badge` | 文本 | 专辑标签 | x=164 y=506 w=180 h=22 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px bold / #5AA9F0 | “NOW PLAYING” |
| 11 | `album_title` | 文本 | 专辑标题 | x=164 y=530 w=340 h=36 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 23px bold / #F4F7FF | “AURORA NIGHTS” |
| 12 | `album_artist` | 文本 | 专辑艺人 | x=164 y=566 w=340 h=24 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 13px / #8B97AC | “DeepSeek Waves” |
| 13 | `progress_bg` | 矩形 | 进度条背景 | x=164 y=612 w=340 h=4 | 0° | 100% | fill `#26324A`; radius 2px |  |
| 14 | `progress` | 矩形 | 播放进度 | x=164 y=612 w=148 h=4 | 0° | 100% | fill `#2D8CEB`; radius 2px |  |
| 15 | `time_now` | 文本 | 已播放时间 | x=164 y=626 w=60 h=20 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #7E8CA3 | “0:48” |
| 16 | `time_total` | 文本 | 总时长 | x=444 y=626 w=60 h=20 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #7E8CA3 | “3:12” |
| 17 | `playing_label` | 文本 | 播放状态 | x=620 y=140 w=300 h=22 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 12px bold / #5AA9F0 | “正在播放” |
| 18 | `track_title` | 文本 | 歌曲标题 | x=620 y=166 w=540 h=48 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 36px bold / #F4F7FF | “Aurora Nights” |
| 19 | `track_artist` | 文本 | 歌曲艺人 | x=620 y=214 w=540 h=26 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 15px / #8B97AC | “DeepSeek Waves · 氛围电子” |
| 20 | `like_star` | 星形 | 收藏星标 | x=1110 y=178 w=52 h=52 | 8° | 90% | fill `#FFB454`; sides 5 |  |
| 21 | `playlist_card` | 矩形 | 播放列表 | x=620 y=260 w=540 h=350 | 0° | 100% | fill `#0D1420`; stroke `#1E2A3D` / 1px; radius 22px |  |
| 22 | `active_row` | 矩形 | 当前播放行 | x=638 y=278 w=504 h=58 | 0° | 100% | fill `#16233A`; radius 14px |  |
| 23 | `active_dot` | 椭圆 | 播放指示点 | x=656 y=300 w=10 h=10 | 0° | 100% | fill `#2D8CEB` |  |
| 24 | `track_1` | 文本 | 曲目 1 | x=682 y=286 w=330 h=42 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 15px bold / #F4F7FF | “Aurora Nights” |
| 25 | `track_1_artist` | 文本 | 曲目 1 艺人 | x=682 y=310 w=330 h=20 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #7E8CA3 | “DeepSeek Waves” |
| 26 | `track_1_time` | 文本 | 曲目 1 时长 | x=1064 y=288 w=60 h=40 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #8B97AC | “3:12” |
| 27 | `track_2` | 文本 | 曲目 2 | x=682 y=350 w=300 h=34 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 14px / #C7D2E3 | “Neon Tides” |
| 28 | `track_2_artist` | 文本 | 曲目 2 艺人 | x=682 y=376 w=300 h=20 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #6F7B90 | “Mirage Unit” |
| 29 | `track_2_time` | 文本 | 曲目 2 时长 | x=1064 y=358 w=60 h=30 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #6F7B90 | “4:05” |
| 30 | `row_sep_2` | 直线 | 分隔线 2 | x=656 y=410 w=468 h=1 | 0° | 100% | fill `#1B2536`; stroke `#1B2536` / 1px | 方向 (0,0) → (1,0) |
| 31 | `track_3` | 文本 | 曲目 3 | x=682 y=426 w=300 h=34 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 14px / #C7D2E3 | “Glass Horizon” |
| 32 | `track_3_artist` | 文本 | 曲目 3 艺人 | x=682 y=452 w=300 h=20 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #6F7B90 | “Lumen Field” |
| 33 | `track_3_time` | 文本 | 曲目 3 时长 | x=1064 y=434 w=60 h=30 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #6F7B90 | “3:48” |
| 34 | `row_sep_3` | 直线 | 分隔线 3 | x=656 y=486 w=468 h=1 | 0° | 100% | fill `#1B2536`; stroke `#1B2536` / 1px | 方向 (0,0) → (1,0) |
| 35 | `track_4` | 文本 | 曲目 4 | x=682 y=502 w=300 h=34 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 14px / #C7D2E3 | “Silent Orbit” |
| 36 | `track_4_artist` | 文本 | 曲目 4 艺人 | x=682 y=528 w=300 h=20 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #6F7B90 | “DeepSeek Waves” |
| 37 | `track_4_time` | 文本 | 曲目 4 时长 | x=1064 y=510 w=60 h=30 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px / #6F7B90 | “5:21” |
| 38 | `ctrl_prev` | 椭圆 | 上一首按钮 | x=640 y=650 w=54 h=54 | 0° | 100% | fill `#182234`; stroke `#2A3854` / 1px |  |
| 39 | `ctrl_prev_icon` | 多边形 | 上一首图标 | x=653 y=663 w=28 h=28 | -90° | 100% | fill `#C7D2E3`; sides 3 |  |
| 40 | `play_button` | 椭圆 | 播放按钮 | x=726 y=636 w=82 h=82 | 0° | 100% | fill `#2D8CEB`; stroke `#70A9F2` / 1px |  |
| 41 | `play_icon` | 多边形 | 播放图标 | x=754 y=661 w=30 h=34 | 90° | 100% | fill `#FFFFFF`; sides 3 |  |
| 42 | `ctrl_next` | 椭圆 | 下一首按钮 | x=840 y=650 w=54 h=54 | 0° | 100% | fill `#182234`; stroke `#2A3854` / 1px |  |
| 43 | `ctrl_next_icon` | 多边形 | 下一首图标 | x=853 y=663 w=28 h=28 | 90° | 100% | fill `#C7D2E3`; sides 3 |  |
| 44 | `eq_1` | 矩形 | 频谱柱 1 | x=934 y=676 w=8 h=24 | 0° | 100% | fill `#2D8CEB`; radius 4px |  |
| 45 | `eq_2` | 矩形 | 频谱柱 2 | x=950 y=664 w=8 h=36 | 0° | 100% | fill `#5AA9F0`; radius 4px |  |
| 46 | `eq_3` | 矩形 | 频谱柱 3 | x=966 y=682 w=8 h=18 | 0° | 100% | fill `#A47BFF`; radius 4px |  |
| 47 | `eq_label` | 文本 | 频谱标签 | x=998 y=668 w=120 h=30 | 0° | 100% | fill `#00000000`; font Microsoft YaHei UI 11px bold / #5AA9F0 | “LIVE MIX” |

### 2.1 参考线（手动测量 / 对齐）

无参考线。

## 3. 交互（Interactions）

| 触发对象 | 触发方式 | 动作 | 动画 |
|---|---|---|---|
| 播放器主卡片 (`player_card`) | 载入时 | 播放动画 | 播放器入场 (`anim_music_intro`) |
| 专辑封面 (`cover`) | 悬停 | 播放动画 | 封面悬停 (`anim_cover_hover`) |
| 播放按钮 (`play_button`) | 点击 | 播放动画 | 播放脉冲 (`anim_play_pulse`) |
| 播放按钮 (`play_button`) | 点击 | 播放动画 | 频谱律动 (`anim_equalizer`) |
| 下一首按钮 (`ctrl_next`) | 点击 | 播放动画 | 播放脉冲 (`anim_play_pulse`) |

## 4. 动效时间线（Animations）

### 播放器入场 (`anim_music_intro`)

- 时长：**1.25s**
- 缓动：**平滑减速** (`easeOutCubic`)
- 循环：**否**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 播放器主卡片 (`player_card`) | 不透明度 | 0s → `0` · 0.65s → `1` |
| 播放器主卡片 (`player_card`) | Y 位置 | 0s → `116` · 0.8s → `86` |
| 专辑封面卡片 (`album_card`) | 不透明度 | 0.08s → `0` · 0.8s → `1` |
| 专辑封面卡片 (`album_card`) | X 位置 | 0.08s → `108` · 0.8s → `136` |
| 专辑封面 (`cover`) | 旋转 | 0.15s → `-8` · 0.9s → `0` |
| 歌曲标题 (`track_title`) | X 位置 | 0.2s → `660` · 0.95s → `620` |
| 歌曲标题 (`track_title`) | 不透明度 | 0.2s → `0` · 0.95s → `1` |
| 歌曲艺人 (`track_artist`) | 不透明度 | 0.3s → `0` · 1s → `1` |
| 播放列表 (`playlist_card`) | 不透明度 | 0.3s → `0` · 1.05s → `1` |
| 播放列表 (`playlist_card`) | Y 位置 | 0.3s → `286` · 1.05s → `260` |
| 播放按钮 (`play_button`) | 不透明度 | 0.5s → `0` · 1.15s → `1` |
| 上一首按钮 (`ctrl_prev`) | 不透明度 | 0.55s → `0` · 1.2s → `1` |
| 下一首按钮 (`ctrl_next`) | 不透明度 | 0.55s → `0` · 1.2s → `1` |

### 播放脉冲 (`anim_play_pulse`)

- 时长：**1.6s**
- 缓动：**缓入缓出** (`easeInOutCubic`)
- 循环：**是**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 播放按钮 (`play_button`) | 填充颜色 | 0s → `#2D8CEB` · 0.8s → `#3DDC97` · 1.6s → `#2D8CEB` |
| 播放进度 (`progress`) | 宽度 | 0s → `148` · 1.6s → `340` |
| 唱片纹路 (`vinyl_ring`) | 旋转 | 0s → `0` · 1.6s → `360` |

### 频谱律动 (`anim_equalizer`)

- 时长：**0.8s**
- 缓动：**缓入缓出** (`easeInOutCubic`)
- 循环：**是**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 频谱柱 1 (`eq_1`) | 高度 | 0s → `10` · 0.2s → `34` · 0.4s → `14` · 0.6s → `40` · 0.8s → `10` |
| 频谱柱 1 (`eq_1`) | Y 位置 | 0s → `690` · 0.2s → `666` · 0.4s → `686` · 0.6s → `660` · 0.8s → `690` |
| 频谱柱 2 (`eq_2`) | 高度 | 0s → `30` · 0.2s → `12` · 0.4s → `38` · 0.6s → `20` · 0.8s → `30` |
| 频谱柱 2 (`eq_2`) | Y 位置 | 0s → `670` · 0.2s → `688` · 0.4s → `662` · 0.6s → `680` · 0.8s → `670` |
| 频谱柱 3 (`eq_3`) | 高度 | 0s → `18` · 0.2s → `42` · 0.4s → `16` · 0.6s → `32` · 0.8s → `18` |
| 频谱柱 3 (`eq_3`) | Y 位置 | 0s → `682` · 0.2s → `658` · 0.4s → `684` · 0.6s → `668` · 0.8s → `682` |

### 封面悬停 (`anim_cover_hover`)

- 时长：**0.4s**
- 缓动：**回弹** (`easeOutBack`)
- 循环：**否**

| 对象 | 属性 | 关键帧 |
|---|---|---|
| 专辑封面 (`cover`) | X 位置 | 0s → `164` · 0.4s → `158` |
| 专辑封面 (`cover`) | Y 位置 | 0s → `150` · 0.4s → `144` |
| 专辑封面 (`cover`) | 宽度 | 0s → `340` · 0.4s → `352` |
| 专辑封面 (`cover`) | 高度 | 0s → `340` · 0.4s → `352` |
| 收藏星标 (`like_star`) | 旋转 | 0s → `8` · 0.4s → `28` |

## 5. 实现指导（框架无关）

### 5.1 画布与布局

- 创建一个宽 `1280px`、高 `800px` 的定尺画板，背景 `#0A0D14`，`overflow: hidden`。
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

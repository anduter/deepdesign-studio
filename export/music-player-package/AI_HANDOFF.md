# DeepSeek 交付说明 · 音乐播放器 · Aurora

> 这是一个 **AI 可直接消费** 的 UI 设计交付包。给 DeepSeek 的推荐做法：把整个文件夹交给它，或先粘贴 `design.json`，再让它阅读本说明。

## 文件清单

| 文件 | 用途 |
|---|---|
| `design.json` | 完整设计数据（唯一事实来源） |
| `UI_SPEC.md` | 颜色/字体令牌、图层表、交互表、动画表、实现指导 |
| `AI_HANDOFF.md` | 本文件，给 AI 的任务说明与提示词 |
| `index.html` | 可交互的 HTML 预览（也是实现参考） |

## 设计概览

| 项目 | 值 |
|---|---|
| 画布 | 1280 × 800 px |
| 背景 | `#0A0D14` |
| 图层 | 47 |
| 动画 | 4 |
| 交互 | 5 |
| 参考线 | 0（仅设计辅助，不参与渲染） |
| 主色数量 | 34 |

## 直接使用的提示词

将下面整段内容发给 DeepSeek（并附加 `design.json` / `UI_SPEC.md`）：

```text
你是资深 UI/前端工程师。请根据同目录下的 design.json（DeepDesign 设计文件）和 UI_SPEC.md，实现一个可运行的交互界面。

【项目】音乐播放器 · Aurora
【画布】1280 × 800 px，背景 #0A0D14
【元素】47 个，按 z 值从底到顶排列；坐标为绝对定位，单位为 px。
【动画】4 个；【交互】5 个。

要求：
1. 严格以 design.json 为唯一事实来源，不要臆造布局或文案。
2. 用 HTML/CSS/SVG 或 React/Vue 实现，保持像素级布局与图层顺序；画板整体等比适配视口。
3. 文本必须是真实可选中文本；交互元素补充语义化标签与键盘支持。
4. 用 requestAnimationFrame 或 Web Animations API 实现时间轴动画：属性为绝对值，支持循环、缓动、多动画叠加与 stopAll。
5. 交互触发：click / hover / leave / load；动作：playAnimation / stopAll。
6. 生成可直接打开的 index.html（或可运行工程），不依赖外部图片资源；图形用 CSS/SVG 绘制。
7. 输出前逐条核对 UI_SPEC.md 的校验清单，并说明你如何验证。

请先阅读 design.json，再输出实现。若存在歧义，以 design.json 中的数值为准。

```

## 给 AI 的硬性约束

1. **不要重排布局**。元素坐标是绝对坐标（左上原点，Y 向下，px），优先 1:1 复刻。
2. **不要改文案**。所有文本以图层表/JSON 的 `text` 字段为准。
3. **图层顺序即数组顺序**，从底到顶；`z` 值越大越靠前。
4. **旋转围绕元素中心**，`transform-origin: center`。
5. **动画值是绝对值**，不是偏移量；多个动画叠加时后触发覆盖同属性。
6. **交互需支持 click/hover/leave/load**，动作为 playAnimation/stopAll。
7. 图形必须用 CSS/SVG 绘制，文本使用真实文本，不引入外部图片。
8. 画板整体等比缩放适配视口，内部坐标保持不变。

## 实现路径建议

- **Web**：一个绝对定位的 artboard + 每层一个 DOM 节点；矩形/椭圆用 CSS，其他图形用内联 SVG；用 requestAnimationFrame 采样关键帧。
- **React/Vue**：把 elements 映射为组件，z 顺序即 key 顺序；动画放入独立 hook/engine，避免每帧触发框架重渲染。
- **Qt/C++**：QWidget 自绘 / QGraphicsScene；元素数据对应 QPainterPath 与 QTimer 采样。
- **移动端**：Flutter CustomPainter / SwiftUI Canvas，同样绝对定位 + 时间轴采样。

## 动画属性对照

| design.json 属性 | CSS 属性 | 说明 |
|---|---|---|
| `x` / `y` | `left` / `top` | px |
| `width` / `height` | `width` / `height` | px |
| `rotation` | `transform: rotate()` | 度，绕中心 |
| `opacity` | `opacity` | 0–1 |
| `fill` | `background-color` / SVG `fill` | 颜色 |
| `gradient` | `linear-gradient` / `radial-gradient` 或 SVG 渐变 | 静态渐变填充 |
| `strokeWidth` | `border-width` / SVG `stroke-width` | px |

## 验证清单

- [ ] 打开生成的页面，和 `index.html` 预览逐像素对比（允许统一缩放）。
- [ ] 所有交互对象可触发正确的动画；再次触发从头播放。
- [ ] 动画结束后保持最终帧；stopAll 可恢复初始状态。
- [ ] 无控制台错误、无外部资源 404。
- [ ] 键盘可操作交互元素，文本可选中。

---

### 附：design.json 摘要

```json
{
    "animations": [
        {
            "duration": 1.25,
            "easing": "easeOutCubic",
            "id": "anim_music_intro",
            "loop": false,
            "name": "播放器入场",
            "tracks": [
                {
                    "elementId": "player_card",
                    "keyframes": [
                        {
                            "time": 0,
                            "value": 0
                        },
                        {
                            "time": 0.65,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "player_card",
                    "keyframes": [
                        {
                            "time": 0,
                            "value": 116
                        },
                        {
                            "time": 0.8,
                            "value": 86
                        }
                    ],
                    "property": "y"
                },
                {
                    "elementId": "album_card",
                    "keyframes": [
                        {
                            "time": 0.08,
                            "value": 0
                        },
                        {
                            "time": 0.8,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "album_card",
                    "keyframes": [
                        {
                            "time": 0.08,
                            "value": 108
                        },
                        {
                            "time": 0.8,
                            "value": 136
                        }
                    ],
                    "property": "x"
                },
                {
                    "elementId": "cover",
                    "keyframes": [
                        {
                            "time": 0.15,
                            "value": -8
                        },
                        {
                            "time": 0.9,
                            "value": 0
                        }
                    ],
                    "property": "rotation"
                },
                {
                    "elementId": "track_title",
                    "keyframes": [
                        {
                            "time": 0.2,
                            "value": 660
                        },
                        {
                            "time": 0.95,
                            "value": 620
                        }
                    ],
                    "property": "x"
                },
                {
                    "elementId": "track_title",
                    "keyframes": [
                        {
                            "time": 0.2,
                            "value": 0
                        },
                        {
                            "time": 0.95,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "track_artist",
                    "keyframes": [
                        {
                            "time": 0.3,
                            "value": 0
                        },
                        {
                            "time": 1,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "playlist_card",
                    "keyframes": [
                        {
                            "time": 0.3,
                            "value": 0
                        },
                        {
                            "time": 1.05,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "playlist_card",
                    "keyframes": [
                        {
                            "time": 0.3,
                            "value": 286
                        },
                        {
                            "time": 1.05,
                            "value": 260
                        }
                    ],
                    "property": "y"
                },
                {
                    "elementId": "play_button",
                    "keyframes": [
                        {
                            "time": 0.5,
                            "value": 0
                        },
                        {
                            "time": 1.15,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "ctrl_prev",
                    "keyframes": [
                        {
                            "time": 0.55,
                            "value": 0
                        },
                        {
                            "time": 1.2,
                            "value": 1
                        }
                    ],
                    "property": "opacity"
                },
                {
                    "elementId": "ctrl_next",
                    "keyframes": [
                        {
                            "time": 0.55,
                            "value": 0
                        },
                        {
                            "time": 1.2,
                            "value": 1
                        }
                    ],
  
…（完整内容见 design.json）
```

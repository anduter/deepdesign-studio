#include "SpecExporter.h"
#include "Animation.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMap>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QtMath>

namespace {
QString md(QString text)
{
    text.replace(QLatin1Char('|'), QStringLiteral("\\|"));
    text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return text;
}

QString number(double value, int decimals = 2)
{
    QString text = QString::number(value, 'f', decimals);
    while (text.contains(QLatin1Char('.')) && (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.')))) {
        text.chop(1);
    }
    return text;
}

QString colorOf(const QColor& color)
{
    return colorToString(color);
}

QString valueText(const Track& track, const QVariant& value)
{
    if (track.property == QLatin1String("fill")) {
        if (value.canConvert<QColor>()) return colorOf(value.value<QColor>());
        return value.toString();
    }
    return number(value.toDouble(), 2);
}

QString keyframeText(const Track& track)
{
    QStringList parts;
    for (const auto& key : track.keys) {
        parts << QStringLiteral("%1s → `%2`").arg(number(key.time, 2), valueText(track, key.value));
    }
    return parts.join(QStringLiteral(" · "));
}

QString typeName(ElementType type)
{
    switch (type) {
    case ElementType::Rectangle: return QStringLiteral("矩形");
    case ElementType::Ellipse: return QStringLiteral("椭圆");
    case ElementType::Line: return QStringLiteral("直线");
    case ElementType::Polygon: return QStringLiteral("多边形");
    case ElementType::Star: return QStringLiteral("星形");
    case ElementType::Path: return QStringLiteral("自由路径");
    case ElementType::Text: return QStringLiteral("文本");
    }
    return QStringLiteral("元素");
}

QString styleText(const Element& element)
{
    QStringList parts;
    if (element.hasGradient()) {
        QStringList stops;
        for (const auto& stop : element.gradient.stops) {
            stops << QStringLiteral("%1@%2%").arg(colorOf(stop.color),
                                                   number(stop.position * 100.0, 0));
        }
        parts << QStringLiteral("%1渐变 %2° [%3]")
                     .arg(element.gradient.type == QLatin1String("radial") ? QStringLiteral("径向")
                                                                          : QStringLiteral("线性"))
                     .arg(number(element.gradient.angle, 0), stops.join(QStringLiteral(" → ")));
    } else {
        parts << QStringLiteral("fill `%1`").arg(colorOf(element.fill));
    }
    if (element.strokeWidth > 0.0) {
        parts << QStringLiteral("stroke `%1` / %2px")
                     .arg(colorOf(element.stroke), number(element.strokeWidth, 1));
    }
    if (element.type == ElementType::Rectangle) parts << QStringLiteral("radius %1px").arg(number(element.radius, 1));
    if (element.type == ElementType::Polygon || element.type == ElementType::Star) {
        parts << QStringLiteral("sides %1").arg(element.sides);
    }
    if (element.type == ElementType::Text) {
        parts << QStringLiteral("font %1 %2px%3 / %4")
                     .arg(element.fontFamily, number(element.fontSize, 0),
                          element.bold ? QStringLiteral(" bold") : QString(),
                          colorOf(element.textColor));
    }
    return parts.join(QStringLiteral("; "));
}

QSet<QString> colorsForDocument(const DesignDocument& document)
{
    QSet<QString> colors;
    colors.insert(colorOf(document.background));
    for (const auto& element : document.elements) {
        colors.insert(colorOf(element.fill));
        if (element.hasGradient()) {
            for (const auto& stop : element.gradient.stops) colors.insert(colorOf(stop.color));
        }
        colors.insert(colorOf(element.stroke));
        colors.insert(colorOf(element.textColor));
    }
    return colors;
}
} // namespace

QString SpecExporter::generateUiSpec(const DesignDocument& document)
{
    QString mdText;
    mdText += QStringLiteral("# %1 · UI 交付规范\n\n").arg(md(document.title));
    mdText += QStringLiteral("> 由 **灵绘 DeepDesign Studio** 自动生成。此文件与 `design.json` 配套，"
                             "`design.json` 是唯一事实来源，本文件用于快速理解与实现。\n\n");
    mdText += QStringLiteral("| 属性 | 值 |\n|---|---|\n");
    mdText += QStringLiteral("| 画布尺寸 | %1 × %2 px |\n")
                  .arg(number(document.canvasSize.width(), 0), number(document.canvasSize.height(), 0));
    mdText += QStringLiteral("| 画板背景 | `%1` |\n").arg(colorOf(document.background));
    mdText += QStringLiteral("| 元素数量 | %1 |\n").arg(document.elements.size());
    mdText += QStringLiteral("| 动画数量 | %1 |\n").arg(document.animations.size());
    mdText += QStringLiteral("| 交互数量 | %1 |\n").arg(document.interactions.size());
    mdText += QStringLiteral("| 参考线数量 | %1 |\n").arg(document.guides.size());
    mdText += QStringLiteral("| 坐标系 | 左上角为原点，X 向右，Y 向下，单位为 px |\n");
    mdText += QStringLiteral("| 图层顺序 | 数组顺序从底到顶，`z` 越大越靠前 |\n\n");

    // ---- Design tokens ---------------------------------------------------
    mdText += QStringLiteral("## 1. 设计令牌（Design Tokens）\n\n");
    QMap<QString, QSet<QString>> colorUsage;
    colorUsage[colorOf(document.background)].insert(QStringLiteral("画板背景"));
    for (const auto& element : document.elements) {
        if (element.fill.alpha() > 0) colorUsage[colorOf(element.fill)].insert(QStringLiteral("填充"));
        if (element.hasGradient()) {
            for (const auto& stop : element.gradient.stops) {
                colorUsage[colorOf(stop.color)].insert(
                    element.gradient.type == QLatin1String("radial") ? QStringLiteral("径向渐变")
                                                                     : QStringLiteral("线性渐变"));
            }
        }
        if (element.strokeWidth > 0) colorUsage[colorOf(element.stroke)].insert(QStringLiteral("描边"));
        if (element.type == ElementType::Text) colorUsage[colorOf(element.textColor)].insert(QStringLiteral("文字"));
    }
    mdText += QStringLiteral("### 1.1 颜色\n\n| 色值 | 用途 |\n|---|---|\n");
    for (auto it = colorUsage.constBegin(); it != colorUsage.constEnd(); ++it) {
        QStringList uses(it.value().begin(), it.value().end());
        uses.sort();
        mdText += QStringLiteral("| `%1` | %2 |\n").arg(it.key(), uses.join(QStringLiteral(" / ")));
    }
    mdText += QLatin1Char('\n');

    QMap<QString, QSet<int>> fonts;
    for (const auto& element : document.elements) {
        if (element.type != ElementType::Text) continue;
        fonts[element.fontFamily].insert(qRound(element.fontSize));
    }
    mdText += QStringLiteral("### 1.2 字体\n\n");
    if (fonts.isEmpty()) {
        mdText += QStringLiteral("此设计没有文本图层；界面文字可参考最接近的语义化字体。\n\n");
    } else {
        mdText += QStringLiteral("| 字体 | 字号 | 字重 |\n|---|---|---|\n");
        for (auto it = fonts.constBegin(); it != fonts.constEnd(); ++it) {
            QStringList sizes;
            for (int size : it.value()) sizes << QStringLiteral("%1px").arg(size);
            sizes.sort();
            bool boldUsed = false;
            for (const auto& element : document.elements) {
                if (element.type == ElementType::Text && element.fontFamily == it.key() && element.bold) boldUsed = true;
            }
            mdText += QStringLiteral("| %1 | %2 | %3 |\n")
                          .arg(md(it.key()), sizes.join(QStringLiteral(", ")),
                               boldUsed ? QStringLiteral("400 / 700") : QStringLiteral("400"));
        }
        mdText += QLatin1Char('\n');
    }

    mdText += QStringLiteral("### 1.3 CSS 变量建议\n\n```css\n:root {\n");
    mdText += QStringLiteral("  --canvas-width: %1px;\n").arg(number(document.canvasSize.width(), 0));
    mdText += QStringLiteral("  --canvas-height: %1px;\n").arg(number(document.canvasSize.height(), 0));
    int tokenIndex = 1;
    for (auto it = colorUsage.constBegin(); it != colorUsage.constEnd(); ++it) {
        mdText += QStringLiteral("  --color-%1: %2; /* %3 */\n")
                      .arg(tokenIndex++)
                      .arg(it.key(), QStringList(it.value().begin(), it.value().end()).join(QStringLiteral(" / ")));
    }
    mdText += QStringLiteral("}\n```\n\n");

    // ---- Layer tree ------------------------------------------------------
    mdText += QStringLiteral("## 2. 图层与布局（从底到顶）\n\n");
    if (document.elements.isEmpty()) {
        mdText += QStringLiteral("暂无元素。\n\n");
    } else {
        mdText += QStringLiteral("| z | ID | 类型 | 名称 | 位置 / 尺寸 | 旋转 | 不透明度 | 样式 | 内容 |\n");
        mdText += QStringLiteral("|---|---|---|---|---|---|---|---|---|\n");
        for (int i = 0; i < document.elements.size(); ++i) {
            const Element& element = document.elements[i];
            QString content;
            if (element.type == ElementType::Text) {
                QString text = element.text;
                text.replace(QLatin1Char('\n'), QStringLiteral(" / "));
                if (text.size() > 80) text = text.left(80) + QStringLiteral("…");
                content = QStringLiteral("“%1”").arg(md(text));
            } else if (element.type == ElementType::Path) {
                content = QStringLiteral("%1 个锚点%2")
                              .arg(element.points.size())
                              .arg(element.closed ? QStringLiteral("，闭合") : QStringLiteral("，开放"));
            } else if (element.type == ElementType::Line) {
                content = QStringLiteral("方向 (%1,%2) → (%3,%4)")
                              .arg(number(element.lineStart.x(), 2), number(element.lineStart.y(), 2),
                                   number(element.lineEnd.x(), 2), number(element.lineEnd.y(), 2));
            }
            mdText += QStringLiteral("| %1 | `%2` | %3 | %4 | x=%5 y=%6 w=%7 h=%8 | %9° | %10 | %11 | %12 |\n")
                          .arg(i + 1)
                          .arg(element.id, typeName(element.type), md(element.name))
                          .arg(number(element.frame.x(), 1), number(element.frame.y(), 1),
                               number(element.frame.width(), 1), number(element.frame.height(), 1))
                          .arg(number(element.rotation, 1))
                          .arg(number(element.opacity * 100, 0) + QStringLiteral("%"))
                          .arg(styleText(element), content);
        }
        mdText += QLatin1Char('\n');
    }

    mdText += QStringLiteral("### 2.1 参考线（手动测量 / 对齐）\n\n");
    if (document.guides.isEmpty()) {
        mdText += QStringLiteral("无参考线。\n\n");
    } else {
        mdText += QStringLiteral("参考线是设计辅助线，不参与最终渲染；用于手动测量和视觉对齐。\n\n");
        mdText += QStringLiteral("| ID | 方向 | 位置 |\n|---|---|---|\n");
        for (const auto& guide : document.guides) {
            const bool horizontal = guide.orientation == QLatin1String("horizontal");
            mdText += QStringLiteral("| `%1` | %2 | %3 = %4 px |\n")
                          .arg(guide.id,
                               horizontal ? QStringLiteral("横向") : QStringLiteral("纵向"),
                               horizontal ? QStringLiteral("Y") : QStringLiteral("X"),
                               number(guide.position, 1));
        }
        mdText += QLatin1Char('\n');
    }

    // ---- Interactions ----------------------------------------------------
    mdText += QStringLiteral("## 3. 交互（Interactions）\n\n");
    if (document.interactions.isEmpty()) {
        mdText += QStringLiteral("没有定义交互。\n\n");
    } else {
        mdText += QStringLiteral("| 触发对象 | 触发方式 | 动作 | 动画 |\n|---|---|---|---|\n");
        for (const auto& interaction : document.interactions) {
            const Element* element = document.findElement(interaction.sourceId);
            QString source = element ? QStringLiteral("%1 (`%2`)").arg(md(element->name), interaction.sourceId)
                                     : interaction.sourceId;
            QString animation = QStringLiteral("—");
            if (const Animation* anim = document.findAnimation(interaction.animationId)) {
                animation = QStringLiteral("%1 (`%2`)").arg(md(anim->name), anim->id);
            }
            const QString action = interaction.action == QLatin1String("stopAll")
                                       ? QStringLiteral("停止全部动画")
                                       : QStringLiteral("播放动画");
            mdText += QStringLiteral("| %1 | %2 | %3 | %4 |\n")
                          .arg(source, interaction.triggerDisplayName(), action, animation);
        }
        mdText += QLatin1Char('\n');
    }

    // ---- Animations ------------------------------------------------------
    mdText += QStringLiteral("## 4. 动效时间线（Animations）\n\n");
    if (document.animations.isEmpty()) {
        mdText += QStringLiteral("没有定义动画。\n\n");
    }
    for (const auto& animation : document.animations) {
        mdText += QStringLiteral("### %1 (`%2`)\n\n").arg(md(animation.name), animation.id);
        mdText += QStringLiteral("- 时长：**%1s**\n").arg(number(animation.duration, 2));
        mdText += QStringLiteral("- 缓动：**%1** (`%2`)\n")
                      .arg(easingDisplayFromId(animation.easing), animation.easing);
        mdText += QStringLiteral("- 循环：**%1**\n\n").arg(animation.loop ? QStringLiteral("是") : QStringLiteral("否"));
        mdText += QStringLiteral("| 对象 | 属性 | 关键帧 |\n|---|---|---|\n");
        for (const auto& track : animation.tracks) {
            const Element* element = document.findElement(track.elementId);
            const QString name = element ? element->name : track.elementId;
            mdText += QStringLiteral("| %1 (`%2`) | %3 | %4 |\n")
                          .arg(md(name), track.elementId,
                               trackPropertyDisplayName(track.property), keyframeText(track));
        }
        mdText += QLatin1Char('\n');
    }

    // ---- Implementation notes -------------------------------------------
    mdText += QStringLiteral("## 5. 实现指导（框架无关）\n\n");
    mdText += QStringLiteral("### 5.1 画布与布局\n\n");
    mdText += QStringLiteral("- 创建一个宽 `%1px`、高 `%2px` 的定尺画板，背景 `%3`，`overflow: hidden`。\n")
                  .arg(number(document.canvasSize.width(), 0), number(document.canvasSize.height(), 0),
                       colorOf(document.background));
    mdText += QStringLiteral("- 所有元素按 **绝对定位** 实现，使用上表中的 `x/y/width/height`。\n");
    mdText += QStringLiteral("- 元素以中心点为旋转原点：`transform: rotate(<rotation>deg); transform-origin: center;`。\n");
    mdText += QStringLiteral("- 为适配不同屏幕，将画板整体等比缩放到容器内，不要改变内部绝对坐标。\n");
    mdText += QStringLiteral("- 图层顺序遵循 z 值，后面的元素覆盖前面的元素。\n\n");

    mdText += QStringLiteral("### 5.2 元素映射\n\n");
    mdText += QStringLiteral("| 设计类型 | HTML / CSS / SVG 实现 |\n|---|---|\n");
    mdText += QStringLiteral("| 矩形 rect | `div` + `border-radius` + `background` + `border` |\n");
    mdText += QStringLiteral("| 椭圆 ellipse | `div` + `border-radius: 50%` |\n");
    mdText += QStringLiteral("| 直线 line | 内联 `svg > line`，使用绝对宽高和 `stroke-linecap: round` |\n");
    mdText += QStringLiteral("| 多边形 polygon | 内联 `svg > polygon`，按外接椭圆均分顶点 |\n");
    mdText += QStringLiteral("| 星形 star | 内联 `svg > polygon`，外/内半径比 1 : 0.44，顶点数 = sides × 2 |\n");
    mdText += QStringLiteral("| 自由路径 path | 内联 `svg > polygon`（闭合）或 `polyline`（开放），points 为归一化坐标 × 宽高 |\n");
    mdText += QStringLiteral("| 文本 text | `div` + flex 垂直居中 + `white-space: pre-wrap` + 指定字体/字号/字重/颜色/对齐 |\n");
    mdText += QStringLiteral("| 渐变填充 gradient | CSS `linear-gradient` / `radial-gradient`；SVG 使用 `<linearGradient>` / `<radialGradient>` |\n\n");

    mdText += QStringLiteral("### 5.3 交互实现\n\n");
    mdText += QStringLiteral("- `click` → 点击；`hover` → 鼠标进入；`leave` → 鼠标离开；`load` → 页面加载后自动执行。\n");
    mdText += QStringLiteral("- `playAnimation`：播放对应动画，重复触发时从头重播；可多个动画同时播放，后触发的覆盖先触发的同属性。\n");
    mdText += QStringLiteral("- `stopAll`：停止所有动画并恢复元素初始样式。\n");
    mdText += QStringLiteral("- 交互对象应设置 `cursor: pointer`，并保留键盘可访问性（为按钮类元素补充 role / tabindex）。\n\n");

    mdText += QStringLiteral("### 5.4 动效实现\n\n");
    mdText += QStringLiteral("- 动画属性均为 **绝对值**：x/y/width/height 单位 px，rotation 单位度，opacity 范围 0–1，fill 为颜色。\n");
    mdText += QStringLiteral("- 关键帧之间按缓动曲线插值；时长单位为秒；`loop=true` 时循环播放。\n");
    mdText += QStringLiteral("- 推荐使用 `requestAnimationFrame` 统一采样（与设计软件行为一致），或使用 Web Animations API。\n");
    mdText += QStringLiteral("- 缓动对应关系：linear, easeInQuad, easeOutQuad, easeInOutCubic, easeOutCubic, easeOutBack, easeOutElastic, easeOutBounce。\n");
    mdText += QStringLiteral("- 动效结束后保持最后一帧（fill-mode: both 的语义），直到 stopAll 或重播。\n\n");
    mdText += QStringLiteral("### 5.5 渐变填充\n\n");
    mdText += QStringLiteral("- `gradient.type`：`linear`（线性）或 `radial`（径向）；`gradient.angle` 为角度，"
                             "0° 表示从左到右，90° 表示从上到下。\n");
    mdText += QStringLiteral("- `gradient.stops` 为颜色停止点数组，`position` 范围 0–1，按位置从 0 到 1 排列。\n");
    mdText += QStringLiteral("- CSS 线性渐变角度需换算为 `angle + 90`（CSS 0° 朝上）；径向渐变以元素中心为圆心。\n");
    mdText += QStringLiteral("- 渐变填充元素仍保留 `fill` 字段作为回退色；`fill` 动画对渐变元素不生效。\n\n");

    mdText += QStringLiteral("### 5.5 语义与可访问性\n\n");
    mdText += QStringLiteral("- 根据图层名称推断语义：包含“按钮/button/btn”的交互元素应输出 `<button>` 或带 `role=button` 的可聚焦元素。\n");
    mdText += QStringLiteral("- 文本应使用真实文本节点，不要转成图片；保持选中与缩放能力。\n");
    mdText += QStringLiteral("- 交互元素需支持 Enter/Space 触发 click 交互。\n\n");

    mdText += QStringLiteral("## 6. 校验清单\n\n");
    mdText += QStringLiteral("- [ ] 画布尺寸、背景色与设计一致\n");
    mdText += QStringLiteral("- [ ] 所有元素的位置、尺寸、旋转、图层顺序与设计一致\n");
    mdText += QStringLiteral("- [ ] 所有文本内容、字体、字号、颜色与设计一致\n");
    mdText += QStringLiteral("- [ ] 交互触发方式与目标动画正确\n");
    mdText += QStringLiteral("- [ ] 动画时长、缓动、循环和关键帧值与设计一致\n");
    mdText += QStringLiteral("- [ ] 画板在目标设备上等比缩放，无裁切或错位\n\n");
    mdText += QStringLiteral("---\n\n生成时间：%1\n")
                  .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    return mdText;
}

QString SpecExporter::generatePrompt(const DesignDocument& document)
{
    QString prompt;
    prompt += QStringLiteral("你是资深 UI/前端工程师。请根据同目录下的 design.json（DeepDesign 设计文件）和 UI_SPEC.md，"
                             "实现一个可运行的交互界面。\n\n");
    prompt += QStringLiteral("【项目】%1\n").arg(document.title);
    prompt += QStringLiteral("【画布】%1 × %2 px，背景 %3\n")
                  .arg(number(document.canvasSize.width(), 0), number(document.canvasSize.height(), 0),
                       colorOf(document.background));
    prompt += QStringLiteral("【元素】%1 个，按 z 值从底到顶排列；坐标为绝对定位，单位为 px。\n")
                  .arg(document.elements.size());
    prompt += QStringLiteral("【动画】%1 个；【交互】%2 个。\n\n")
                  .arg(document.animations.size()).arg(document.interactions.size());
    prompt += QStringLiteral("要求：\n");
    prompt += QStringLiteral("1. 严格以 design.json 为唯一事实来源，不要臆造布局或文案。\n");
    prompt += QStringLiteral("2. 用 HTML/CSS/SVG 或 React/Vue 实现，保持像素级布局与图层顺序；画板整体等比适配视口。\n");
    prompt += QStringLiteral("3. 文本必须是真实可选中文本；交互元素补充语义化标签与键盘支持。\n");
    prompt += QStringLiteral("4. 用 requestAnimationFrame 或 Web Animations API 实现时间轴动画：属性为绝对值，"
                             "支持循环、缓动、多动画叠加与 stopAll。\n");
    prompt += QStringLiteral("5. 交互触发：click / hover / leave / load；动作：playAnimation / stopAll。\n");
    prompt += QStringLiteral("6. 生成可直接打开的 index.html（或可运行工程），不依赖外部图片资源；图形用 CSS/SVG 绘制。\n");
    prompt += QStringLiteral("7. 输出前逐条核对 UI_SPEC.md 的校验清单，并说明你如何验证。\n\n");
    prompt += QStringLiteral("请先阅读 design.json，再输出实现。若存在歧义，以 design.json 中的数值为准。\n");
    return prompt;
}

QString SpecExporter::generateAiHandoff(const DesignDocument& document)
{
    QString text;
    text += QStringLiteral("# DeepSeek 交付说明 · %1\n\n").arg(md(document.title));
    text += QStringLiteral("> 这是一个 **AI 可直接消费** 的 UI 设计交付包。给 DeepSeek 的推荐做法："
                             "把整个文件夹交给它，或先粘贴 `design.json`，再让它阅读本说明。\n\n");
    text += QStringLiteral("## 文件清单\n\n");
    text += QStringLiteral("| 文件 | 用途 |\n|---|---|\n");
    text += QStringLiteral("| `design.json` | 完整设计数据（唯一事实来源） |\n");
    text += QStringLiteral("| `UI_SPEC.md` | 颜色/字体令牌、图层表、交互表、动画表、实现指导 |\n");
    text += QStringLiteral("| `AI_HANDOFF.md` | 本文件，给 AI 的任务说明与提示词 |\n");
    text += QStringLiteral("| `index.html` | 可交互的 HTML 预览（也是实现参考） |\n\n");

    text += QStringLiteral("## 设计概览\n\n");
    text += QStringLiteral("| 项目 | 值 |\n|---|---|\n");
    text += QStringLiteral("| 画布 | %1 × %2 px |\n")
                .arg(number(document.canvasSize.width(), 0), number(document.canvasSize.height(), 0));
    text += QStringLiteral("| 背景 | `%1` |\n").arg(colorOf(document.background));
    text += QStringLiteral("| 图层 | %1 |\n").arg(document.elements.size());
    text += QStringLiteral("| 动画 | %1 |\n").arg(document.animations.size());
    text += QStringLiteral("| 交互 | %1 |\n").arg(document.interactions.size());
    text += QStringLiteral("| 参考线 | %1（仅设计辅助，不参与渲染） |\n").arg(document.guides.size());
    const QSet<QString> colors = colorsForDocument(document);
    text += QStringLiteral("| 主色数量 | %1 |\n\n").arg(colors.size());

    text += QStringLiteral("## 直接使用的提示词\n\n");
    text += QStringLiteral("将下面整段内容发给 DeepSeek（并附加 `design.json` / `UI_SPEC.md`）：\n\n");
    text += QStringLiteral("```text\n%1\n```\n\n").arg(generatePrompt(document));

    text += QStringLiteral("## 给 AI 的硬性约束\n\n");
    text += QStringLiteral("1. **不要重排布局**。元素坐标是绝对坐标（左上原点，Y 向下，px），优先 1:1 复刻。\n");
    text += QStringLiteral("2. **不要改文案**。所有文本以图层表/JSON 的 `text` 字段为准。\n");
    text += QStringLiteral("3. **图层顺序即数组顺序**，从底到顶；`z` 值越大越靠前。\n");
    text += QStringLiteral("4. **旋转围绕元素中心**，`transform-origin: center`。\n");
    text += QStringLiteral("5. **动画值是绝对值**，不是偏移量；多个动画叠加时后触发覆盖同属性。\n");
    text += QStringLiteral("6. **交互需支持 click/hover/leave/load**，动作为 playAnimation/stopAll。\n");
    text += QStringLiteral("7. 图形必须用 CSS/SVG 绘制，文本使用真实文本，不引入外部图片。\n");
    text += QStringLiteral("8. 画板整体等比缩放适配视口，内部坐标保持不变。\n\n");

    text += QStringLiteral("## 实现路径建议\n\n");
    text += QStringLiteral("- **Web**：一个绝对定位的 artboard + 每层一个 DOM 节点；矩形/椭圆用 CSS，其他图形用内联 SVG；"
                             "用 requestAnimationFrame 采样关键帧。\n");
    text += QStringLiteral("- **React/Vue**：把 elements 映射为组件，z 顺序即 key 顺序；动画放入独立 hook/engine，避免每帧触发框架重渲染。\n");
    text += QStringLiteral("- **Qt/C++**：QWidget 自绘 / QGraphicsScene；元素数据对应 QPainterPath 与 QTimer 采样。\n");
    text += QStringLiteral("- **移动端**：Flutter CustomPainter / SwiftUI Canvas，同样绝对定位 + 时间轴采样。\n\n");

    text += QStringLiteral("## 动画属性对照\n\n");
    text += QStringLiteral("| design.json 属性 | CSS 属性 | 说明 |\n|---|---|---|\n");
    text += QStringLiteral("| `x` / `y` | `left` / `top` | px |\n");
    text += QStringLiteral("| `width` / `height` | `width` / `height` | px |\n");
    text += QStringLiteral("| `rotation` | `transform: rotate()` | 度，绕中心 |\n");
    text += QStringLiteral("| `opacity` | `opacity` | 0–1 |\n");
    text += QStringLiteral("| `fill` | `background-color` / SVG `fill` | 颜色 |\n");
    text += QStringLiteral("| `gradient` | `linear-gradient` / `radial-gradient` 或 SVG 渐变 | 静态渐变填充 |\n");
    text += QStringLiteral("| `strokeWidth` | `border-width` / SVG `stroke-width` | px |\n\n");

    text += QStringLiteral("## 验证清单\n\n");
    text += QStringLiteral("- [ ] 打开生成的页面，和 `index.html` 预览逐像素对比（允许统一缩放）。\n");
    text += QStringLiteral("- [ ] 所有交互对象可触发正确的动画；再次触发从头播放。\n");
    text += QStringLiteral("- [ ] 动画结束后保持最终帧；stopAll 可恢复初始状态。\n");
    text += QStringLiteral("- [ ] 无控制台错误、无外部资源 404。\n");
    text += QStringLiteral("- [ ] 键盘可操作交互元素，文本可选中。\n\n");

    text += QStringLiteral("---\n\n");
    text += QStringLiteral("### 附：design.json 摘要\n\n");
    text += QStringLiteral("```json\n");
    QJsonDocument json(document.toJson());
    QString compact = QString::fromUtf8(json.toJson(QJsonDocument::Indented));
    if (compact.size() > 6000) {
        compact = compact.left(6000) + QStringLiteral("\n…（完整内容见 design.json）");
    }
    text += compact + QStringLiteral("\n```\n");
    return text;
}

bool SpecExporter::writeFiles(const DesignDocument& document, const QString& directory, QString* error)
{
    QDir dir(directory);
    if (!dir.exists() && !QDir().mkpath(directory)) {
        if (error) *error = QStringLiteral("无法创建目录：%1").arg(directory);
        return false;
    }
    auto writeFile = [&](const QString& name, const QByteArray& data) {
        QSaveFile file(dir.filePath(name));
        if (!file.open(QIODevice::WriteOnly)) {
            if (error) *error = QStringLiteral("无法写入 %1：%2").arg(name, file.errorString());
            return false;
        }
        file.write(data);
        if (!file.commit()) {
            if (error) *error = QStringLiteral("保存 %1 失败：%2").arg(name, file.errorString());
            return false;
        }
        return true;
    };

    if (!writeFile(QStringLiteral("UI_SPEC.md"), generateUiSpec(document).toUtf8())) return false;
    if (!writeFile(QStringLiteral("AI_HANDOFF.md"), generateAiHandoff(document).toUtf8())) return false;
    if (!writeFile(QStringLiteral("design.json"),
                   QJsonDocument(document.toJson()).toJson(QJsonDocument::Indented))) return false;
    if (error) error->clear();
    return true;
}

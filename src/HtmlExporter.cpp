#include "HtmlExporter.h"
#include "SpecExporter.h"

#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStringList>
#include <QtMath>
#include <cmath>

namespace {
QString htmlEscape(const QString& text)
{
    return text.toHtmlEscaped();
}

QString cssFontFamily(QString family)
{
    family.remove(QLatin1Char('\\'));
    family.remove(QLatin1Char('\''));
    family.remove(QLatin1Char('"'));
    if (family.trimmed().isEmpty()) family = QStringLiteral("Microsoft YaHei UI");
    return QStringLiteral("'%1'").arg(family);
}

QString pointList(const QVector<QPointF>& points, double width, double height)
{
    QStringList parts;
    for (const auto& point : points) {
        parts << QStringLiteral("%1,%2")
                     .arg(QString::number(point.x() * width, 'f', 2),
                          QString::number(point.y() * height, 'f', 2));
    }
    return parts.join(QLatin1Char(' '));
}

QString polygonPoints(const Element& element)
{
    const QRectF r = element.boundingRect();
    const double cx = r.width() / 2.0;
    const double cy = r.height() / 2.0;
    const double rx = r.width() / 2.0;
    const double ry = r.height() / 2.0;
    const int sides = qBound(3, element.sides, 60);
    QVector<QPointF> points;
    if (element.type == ElementType::Polygon) {
        for (int i = 0; i < sides; ++i) {
            const double angle = -M_PI / 2.0 + i * 2.0 * M_PI / sides;
            points.append(QPointF(cx + rx * qCos(angle), cy + ry * qSin(angle)));
        }
    } else {
        const int count = sides * 2;
        for (int i = 0; i < count; ++i) {
            const double radius = (i % 2 == 0) ? 1.0 : 0.44;
            const double angle = -M_PI / 2.0 + i * M_PI / sides;
            points.append(QPointF(cx + rx * radius * qCos(angle), cy + ry * radius * qSin(angle)));
        }
    }
    QStringList parts;
    for (const auto& point : points) {
        parts << QStringLiteral("%1,%2")
                     .arg(QString::number(point.x(), 'f', 2), QString::number(point.y(), 'f', 2));
    }
    return parts.join(QLatin1Char(' '));
}

QString svgPaint(const QColor& color, const QString& fallback = QStringLiteral("none"))
{
    if (!color.isValid() || color.alpha() == 0) return fallback;
    return colorToString(color);
}

QString cssGradient(const GradientFill& gradient)
{
    if (!gradient.enabled || gradient.stops.size() < 2) return {};
    QStringList stops;
    for (const auto& stop : gradient.stops) {
        stops << QStringLiteral("%1 %2%").arg(colorToString(stop.color))
                         .arg(QString::number(stop.position * 100.0, 'f', 2));
    }
    if (gradient.type == QLatin1String("radial")) {
        return QStringLiteral("radial-gradient(circle at 50% 50%, %1)").arg(stops.join(QStringLiteral(", ")));
    }
    const double cssAngle = std::fmod(gradient.angle + 90.0, 360.0);
    return QStringLiteral("linear-gradient(%1deg, %2)")
        .arg(QString::number(cssAngle, 'f', 2), stops.join(QStringLiteral(", ")));
}

QString svgGradientDefs(const Element& element)
{
    if (!element.hasGradient()) return {};
    const QRectF r = element.boundingRect();
    const QString gradientId = QStringLiteral("grad_") + element.id;
    const GradientFill& gradient = element.gradient;
    QStringList stops;
    for (const auto& stop : gradient.stops) {
        stops << QStringLiteral("<stop offset=\"%1%\" stop-color=\"%2\"/>")
                     .arg(QString::number(stop.position * 100.0, 'f', 2),
                          colorToString(stop.color));
    }
    QString defs;
    if (gradient.type == QLatin1String("radial")) {
        defs = QStringLiteral("<radialGradient id=\"%1\" gradientUnits=\"userSpaceOnUse\" "
                              "cx=\"%2\" cy=\"%3\" r=\"%4\">%5</radialGradient>")
                   .arg(gradientId,
                        QString::number(r.width() / 2.0, 'f', 2),
                        QString::number(r.height() / 2.0, 'f', 2),
                        QString::number(qMax(r.width(), r.height()) * 0.62, 'f', 2),
                        stops.join(QString()));
    } else {
        const double radians = qDegreesToRadians(gradient.angle);
        const double dx = qCos(radians);
        const double dy = qSin(radians);
        const double half = (qAbs(r.width() * dx) + qAbs(r.height() * dy)) / 2.0;
        defs = QStringLiteral("<linearGradient id=\"%1\" gradientUnits=\"userSpaceOnUse\" "
                              "x1=\"%2\" y1=\"%3\" x2=\"%4\" y2=\"%5\">%6</linearGradient>")
                   .arg(gradientId,
                        QString::number(r.width() / 2.0 - dx * half, 'f', 2),
                        QString::number(r.height() / 2.0 - dy * half, 'f', 2),
                        QString::number(r.width() / 2.0 + dx * half, 'f', 2),
                        QString::number(r.height() / 2.0 + dy * half, 'f', 2),
                        stops.join(QString()));
    }
    return QStringLiteral("<defs>") + defs + QStringLiteral("</defs>");
}

QString svgFill(const Element& element)
{
    if (element.hasGradient()) return QStringLiteral("url(#grad_%1)").arg(element.id);
    return svgPaint(element.fill, QStringLiteral("none"));
}

QString elementHtml(const Element& element)
{
    const QRectF r = element.boundingRect();
    const QString id = htmlEscape(element.id);
    const QString name = htmlEscape(element.name);
    const QString base = QStringLiteral(
        "left:%1px;top:%2px;width:%3px;height:%4px;opacity:%5;"
        "transform:rotate(%6deg);")
        .arg(QString::number(element.frame.x(), 'f', 2),
             QString::number(element.frame.y(), 'f', 2),
             QString::number(r.width(), 'f', 2),
             QString::number(r.height(), 'f', 2),
             QString::number(element.opacity, 'f', 3),
             QString::number(element.rotation, 'f', 2));
    const QString hidden = element.visible ? QString() : QStringLiteral("display:none;");
    const QString style = base + hidden;

    QString inner;
    QString extraClass;
    if (element.type == ElementType::Rectangle || element.type == ElementType::Ellipse) {
        const double radius = element.type == ElementType::Ellipse
                                  ? 0.0
                                  : (r.width() > 0 ? qBound(0.0, element.radius,
                                                            qMin(r.width(), r.height()) / 2.0) : 0.0);
        const QString radiusCss = element.type == ElementType::Ellipse
                                      ? QStringLiteral("50%")
                                      : QStringLiteral("%1px").arg(QString::number(radius, 'f', 2));
        const QString border = element.strokeWidth > 0.0
                                   ? QStringLiteral("border:%1px solid %2;")
                                         .arg(QString::number(element.strokeWidth, 'f', 2),
                                              svgPaint(element.stroke, QStringLiteral("transparent")))
                                   : QStringLiteral("border:0;");
        const QString fill = element.hasGradient() ? cssGradient(element.gradient)
                                                   : svgPaint(element.fill, QStringLiteral("transparent"));
        inner = QStringLiteral(
            "<div class=\"shape-fill\" style=\"position:absolute;inset:0;box-sizing:border-box;"
            "background:%1;%2border-radius:%3;\"></div>")
            .arg(fill, border, radiusCss);
    } else if (element.type == ElementType::Line) {
        const QString stroke = svgPaint(element.strokeWidth > 0 ? element.stroke : element.fill,
                                        QStringLiteral("#FFFFFF"));
        const double strokeWidth = qMax(1.0, element.strokeWidth);
        inner = QStringLiteral(
            "<svg data-shape viewBox=\"0 0 %1 %2\" preserveAspectRatio=\"none\" "
            "style=\"display:block;width:100%;height:100%;overflow:visible;\">"
            "<line x1=\"%3\" y1=\"%4\" x2=\"%5\" y2=\"%6\" stroke=\"%7\" stroke-width=\"%8\" "
            "stroke-linecap=\"round\"/></svg>")
            .arg(QString::number(qMax(0.01, r.width()), 'f', 2),
                 QString::number(qMax(0.01, r.height()), 'f', 2),
                 QString::number(element.lineStart.x() * r.width(), 'f', 2),
                 QString::number(element.lineStart.y() * r.height(), 'f', 2),
                 QString::number(element.lineEnd.x() * r.width(), 'f', 2),
                 QString::number(element.lineEnd.y() * r.height(), 'f', 2),
                 stroke, QString::number(strokeWidth, 'f', 2));
    } else if (element.type == ElementType::Text) {
        const QString align = element.align == QLatin1String("left") ? QStringLiteral("left")
                              : element.align == QLatin1String("right") ? QStringLiteral("right")
                                                                        : QStringLiteral("center");
        inner = QStringLiteral("<div class=\"textbox\" style=\"width:100%;padding:2px 2px;"
                               "white-space:pre-wrap;overflow-wrap:anywhere;text-align:%1;\">%2</div>")
                    .arg(align, htmlEscape(element.text));
        extraClass = QStringLiteral(" layer-text");
    } else {
        QString points;
        if (element.type == ElementType::Path) {
            points = pointList(element.points, r.width(), r.height());
        } else {
            points = polygonPoints(element);
        }
        const QString tag = (element.type == ElementType::Path && !element.closed)
                                ? QStringLiteral("polyline")
                                : QStringLiteral("polygon");
        const QString fill = svgFill(element);
        const QString stroke = element.strokeWidth > 0 ? svgPaint(element.stroke) : QStringLiteral("none");
        const QString defs = svgGradientDefs(element);
        inner = QStringLiteral(
            "<svg data-shape viewBox=\"0 0 %1 %2\" preserveAspectRatio=\"none\" "
            "style=\"display:block;width:100%;height:100%;overflow:visible;\">"
            "%3<%4 points=\"%5\" fill=\"%6\" stroke=\"%7\" stroke-width=\"%8\" "
            "stroke-linejoin=\"round\"/></svg>")
            .arg(QString::number(qMax(0.01, r.width()), 'f', 2),
                 QString::number(qMax(0.01, r.height()), 'f', 2),
                 defs, tag, points, fill, stroke,
                 QString::number(element.strokeWidth, 'f', 2));
    }

    QString textStyle;
    if (element.type == ElementType::Text) {
        textStyle = QStringLiteral("color:%1;font-family:%2;font-size:%3px;font-weight:%4;"
                                   "line-height:1.25;")
                        .arg(svgPaint(element.textColor, QStringLiteral("#FFFFFF")),
                             cssFontFamily(element.fontFamily),
                             QString::number(element.fontSize, 'f', 1),
                             element.bold ? QStringLiteral("700") : QStringLiteral("400"));
    }

    const QString classes = QStringLiteral("layer") + extraClass;
    return QStringLiteral(
               "<div class=\"%1\" id=\"%2\" data-id=\"%2\" data-name=\"%3\" data-type=\"%4\" "
               "style=\"%5%6\">%7</div>")
        .arg(classes, id, name, elementTypeToString(element.type), style, textStyle, inner);
}

} // namespace

QString HtmlExporter::generateHtml(const DesignDocument& document)
{
    QString html;
    html += QStringLiteral("<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    html += QStringLiteral("<meta charset=\"utf-8\">\n");
    html += QStringLiteral("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n");
    html += QStringLiteral("<title>%1 · 交互预览</title>\n").arg(htmlEscape(document.title));
    html += QStringLiteral(R"HTML(
<style>
html, body {
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
  overflow: hidden;
  background: #0b0b0b;
  font-family: "Microsoft YaHei UI", "Segoe UI", system-ui, sans-serif;
}
.viewport {
  position: fixed;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}
.holder { position: relative; flex: none; }
.stage {
  position: absolute;
  left: 0;
  top: 0;
  transform-origin: top left;
  overflow: hidden;
  box-shadow: 0 24px 80px rgba(0, 0, 0, .65);
}
.layer {
  position: absolute;
  box-sizing: border-box;
  transform-origin: center center;
  will-change: transform, left, top, width, height, opacity;
}
.layer-text { display: flex; align-items: center; justify-content: flex-start; }
.chrome {
  position: fixed;
  left: 18px;
  top: 14px;
  color: #9a9a9a;
  font-size: 11px;
  line-height: 1.6;
  pointer-events: none;
  text-shadow: 0 1px 4px rgba(0, 0, 0, .8);
}
.chrome strong { color: #e2e2e2; font-weight: 600; }
.chrome span { color: #6e6e6e; }
.actions {
  position: fixed;
  right: 18px;
  bottom: 16px;
  display: flex;
  gap: 8px;
}
.actions button {
  background: rgba(35, 35, 35, .92);
  color: #d6d6d6;
  border: 1px solid #4a4a4a;
  border-radius: 4px;
  padding: 6px 12px;
  font-size: 11px;
  cursor: pointer;
  font-family: inherit;
}
.actions button:hover { background: #3d3d3d; border-color: #2d8ceb; color: #fff; }
.hint {
  position: fixed;
  left: 50%;
  bottom: 16px;
  transform: translateX(-50%);
  color: #6e6e6e;
  font-size: 11px;
  pointer-events: none;
}
</style>
)HTML");

    html += QStringLiteral("</head>\n<body>\n");
    html += QStringLiteral("<div class=\"chrome\"><strong>%1</strong><br><span>%2 × %3 px · DeepDesign</span></div>\n")
                .arg(htmlEscape(document.title))
                .arg(qRound(document.canvasSize.width()))
                .arg(qRound(document.canvasSize.height()));
    html += QStringLiteral("<div class=\"viewport\"><div class=\"holder\" id=\"holder\">");
    html += QStringLiteral("<div class=\"stage\" id=\"stage\" style=\"width:%1px;height:%2px;background:%3;\">")
                .arg(QString::number(document.canvasSize.width(), 'f', 2),
                     QString::number(document.canvasSize.height(), 'f', 2),
                     colorToString(document.background));
    for (const auto& element : document.elements) {
        html += elementHtml(element);
    }
    html += QStringLiteral("</div></div></div>\n");
    html += QStringLiteral("<div class=\"hint\">点击或悬停交互对象 · 由 .ddesign 设计文件生成</div>\n");
    html += QStringLiteral("<div class=\"actions\"><button onclick=\"playAll()\">播放全部</button>"
                           "<button onclick=\"replay()\">重播</button></div>\n");

    QJsonDocument jsonDocument(document.toJson());
    QString json = QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Compact));
    json.replace(QStringLiteral("</"), QStringLiteral("<\\/"));
    json.replace(QChar(0x2028), QStringLiteral("\\u2028"));
    json.replace(QChar(0x2029), QStringLiteral("\\u2029"));

    html += QStringLiteral("<script>\nconst DESIGN = %1;\n").arg(json);
    html += QStringLiteral(R"JS(
const stage = document.getElementById('stage');
const holder = document.getElementById('holder');
const W = DESIGN.canvas.width, H = DESIGN.canvas.height;
const byId = new Map(DESIGN.elements.map(e => [e.id, e]));
const animations = new Map(DESIGN.animations.map(a => [a.id, a]));
let active = [];
let rafId = 0;

function fit() {
  const scale = Math.max(0.05, Math.min((window.innerWidth - 56) / W, (window.innerHeight - 56) / H, 1.6));
  holder.style.width = (W * scale) + 'px';
  holder.style.height = (H * scale) + 'px';
  stage.style.transform = 'scale(' + scale + ')';
}
window.addEventListener('resize', fit);
fit();

function clamp01(v) { return Math.max(0, Math.min(1, v)); }
function ease(name, t) {
  t = clamp01(t);
  switch (name) {
    case 'easeInQuad': return t * t;
    case 'easeOutQuad': return 1 - (1 - t) * (1 - t);
    case 'easeInOutCubic': return t < .5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
    case 'easeOutCubic': return 1 - Math.pow(1 - t, 3);
    case 'easeOutBack': {
      const c1 = 1.70158, c3 = c1 + 1;
      return 1 + c3 * Math.pow(t - 1, 3) + c1 * Math.pow(t - 1, 2);
    }
    case 'easeOutElastic': {
      if (t <= 0) return 0;
      if (t >= 1) return 1;
      const c4 = (2 * Math.PI) / 3;
      return Math.pow(2, -10 * t) * Math.sin((t * 10 - .75) * c4) + 1;
    }
    case 'easeOutBounce': {
      const n1 = 7.5625, d1 = 2.75;
      if (t < 1 / d1) return n1 * t * t;
      if (t < 2 / d1) { t -= 1.5 / d1; return n1 * t * t + .75; }
      if (t < 2.5 / d1) { t -= 2.25 / d1; return n1 * t * t + .9375; }
      t -= 2.625 / d1; return n1 * t * t + .984375;
    }
    default: return t;
  }
}

function parseColor(value) {
  if (!value) return { r: 0, g: 0, b: 0, a: 1 };
  let hex = String(value).replace('#', '');
  if (hex.length === 3) hex = hex.split('').map(c => c + c).join('');
  if (hex.length === 6) hex += 'ff';
  if (hex.length !== 8) return { r: 255, g: 255, b: 255, a: 1 };
  return {
    r: parseInt(hex.slice(0, 2), 16),
    g: parseInt(hex.slice(2, 4), 16),
    b: parseInt(hex.slice(4, 6), 16),
    a: parseInt(hex.slice(6, 8), 16) / 255,
  };
}
function lerpColor(a, b, t) {
  const ca = parseColor(a), cb = parseColor(b);
  const r = Math.round(ca.r + (cb.r - ca.r) * t);
  const g = Math.round(ca.g + (cb.g - ca.g) * t);
  const bl = Math.round(ca.b + (cb.b - ca.b) * t);
  const al = ca.a + (cb.a - ca.a) * t;
  return 'rgba(' + r + ',' + g + ',' + bl + ',' + al.toFixed(3) + ')';
}

function sampleTrack(track, time, easing) {
  const keys = track.keyframes;
  if (!keys || !keys.length) return null;
  if (time <= keys[0].time) return keys[0].value;
  if (time >= keys[keys.length - 1].time) return keys[keys.length - 1].value;
  let i = 0;
  while (i < keys.length - 2 && time > keys[i + 1].time) i++;
  const a = keys[i], b = keys[i + 1];
  const span = b.time - a.time;
  let u = span <= 0.000001 ? 1 : (time - a.time) / span;
  u = ease(easing, u);
  if (track.property === 'fill') return lerpColor(a.value, b.value, u);
  return a.value + (b.value - a.value) * u;
}

function gradientCss(e) {
  if (!e.gradient || !e.gradient.stops || e.gradient.stops.length < 2) return e.fill;
  const stops = e.gradient.stops.map(s => s.color + ' ' + (s.position * 100).toFixed(2) + '%').join(', ');
  if (e.gradient.type === 'radial') return 'radial-gradient(circle at 50% 50%, ' + stops + ')';
  const angle = ((e.gradient.angle + 90) % 360 + 360) % 360;
  return 'linear-gradient(' + angle.toFixed(2) + 'deg, ' + stops + ')';
}

function svgFill(e) {
  if (e.gradient && e.gradient.stops && e.gradient.stops.length >= 2) {
    return 'url(#grad_' + e.id + ')';
  }
  return e.fill;
}

function resetElement(e) {
  const node = document.getElementById(e.id);
  if (!node) return;
  node.style.left = e.x + 'px';
  node.style.top = e.y + 'px';
  node.style.width = e.width + 'px';
  node.style.height = e.height + 'px';
  node.style.opacity = e.opacity;
  node.style.transform = 'rotate(' + e.rotation + 'deg)';
  node.style.display = e.visible ? '' : 'none';
  if (e.type === 'text') {
    node.style.color = e.textColor;
  } else if (e.type === 'rect' || e.type === 'ellipse') {
    node.querySelector('.shape-fill').style.background = gradientCss(e);
    const border = node.querySelector('.shape-fill');
    if (e.strokeWidth > 0) {
      border.style.border = e.strokeWidth + 'px solid ' + e.stroke;
      border.style.borderRadius = e.type === 'ellipse' ? '50%' :
        Math.max(0, Math.min(e.radius, Math.min(e.width, e.height) / 2)) + 'px';
    } else {
      border.style.border = '0';
    }
  } else {
    const shape = node.querySelector('[data-shape]');
    if (!shape) return;
    if (e.type === 'line') {
      const stroke = e.strokeWidth > 0 ? e.stroke : (e.fill || '#FFFFFF');
      shape.setAttribute('stroke', stroke);
      shape.setAttribute('stroke-width', Math.max(1, e.strokeWidth));
    } else {
      shape.setAttribute('fill', svgFill(e));
      shape.setAttribute('stroke', e.strokeWidth > 0 ? e.stroke : 'none');
      shape.setAttribute('stroke-width', e.strokeWidth);
    }
  }
}

function applyValue(element, property, value) {
  const node = document.getElementById(element.id);
  if (!node) return;
  if (property === 'x' || property === 'y' || property === 'width' || property === 'height') {
    node.style[property] = value + 'px';
    return;
  }
  if (property === 'rotation') {
    node.style.transform = 'rotate(' + value + 'deg)';
    return;
  }
  if (property === 'opacity') {
    node.style.opacity = value;
    return;
  }
  if (property === 'fill') {
    if (element.gradient) return;
    if (element.type === 'text') {
      node.style.color = value;
    } else if (element.type === 'rect' || element.type === 'ellipse') {
      const fill = node.querySelector('.shape-fill');
      if (fill) fill.style.background = value;
    } else {
      const shape = node.querySelector('[data-shape]');
      if (shape) {
        if (element.type === 'line') shape.setAttribute('stroke', value);
        else shape.setAttribute('fill', value);
      }
    }
    return;
  }
  if (property === 'strokeWidth') {
    if (element.type === 'rect' || element.type === 'ellipse') {
      const fill = node.querySelector('.shape-fill');
      if (fill) {
        fill.style.borderWidth = value + 'px';
        fill.style.borderStyle = value > 0 ? 'solid' : 'none';
      }
    } else {
      const shape = node.querySelector('[data-shape]');
      if (shape) shape.setAttribute('stroke-width', value);
    }
  }
}

function startFrameLoop() {
  if (rafId) return;
  rafId = requestAnimationFrame(frame);
}

function frame(now) {
  rafId = 0;
  const merged = new Map();
  let keepGoing = false;
  for (const act of active) {
    const anim = animations.get(act.id);
    if (!anim) continue;
    let t = (now - act.start) / 1000;
    const duration = Math.max(0.001, anim.duration);
    if (anim.loop) {
      t = t % duration;
      keepGoing = true;
    } else if (t >= duration) {
      t = duration;
      act.done = true;
    } else {
      keepGoing = true;
    }
    for (const track of anim.tracks) {
      const value = sampleTrack(track, t, anim.easing);
      if (value === null) continue;
      if (!merged.has(track.elementId)) merged.set(track.elementId, new Map());
      merged.get(track.elementId).set(track.property, value);
    }
  }
  for (const [elementId, props] of merged) {
    const element = byId.get(elementId);
    if (!element) continue;
    for (const [property, value] of props) applyValue(element, property, value);
  }
  if (keepGoing) rafId = requestAnimationFrame(frame);
}

function playAnimation(id) {
  const anim = animations.get(id);
  if (!anim) return;
  const now = performance.now();
  const existing = active.find(a => a.id === id);
  if (existing) {
    existing.start = now;
    existing.done = false;
  } else {
    active.push({ id: id, start: now, done: false });
  }
  startFrameLoop();
}

function stopAll() {
  active = [];
  if (rafId) cancelAnimationFrame(rafId);
  rafId = 0;
  for (const element of DESIGN.elements) resetElement(element);
}

function trigger(sourceId, eventType) {
  for (const interaction of DESIGN.interactions) {
    if (interaction.sourceId !== sourceId || interaction.trigger !== eventType) continue;
    if (interaction.action === 'stopAll') stopAll();
    else if (interaction.action === 'playAnimation' && interaction.animationId) {
      playAnimation(interaction.animationId);
    }
  }
}

function replay() {
  stopAll();
  for (const interaction of DESIGN.interactions) {
    if (interaction.trigger === 'load') trigger(interaction.sourceId, 'load');
  }
}

function playAll() {
  stopAll();
  for (const anim of DESIGN.animations) playAnimation(anim.id);
}

const interactive = new Set();
for (const interaction of DESIGN.interactions) {
  if (interaction.trigger === 'click' || interaction.trigger === 'hover') {
    interactive.add(interaction.sourceId);
  }
}
for (const element of DESIGN.elements) {
  const node = document.getElementById(element.id);
  if (!node) continue;
  node.addEventListener('click', () => trigger(element.id, 'click'));
  node.addEventListener('mouseenter', () => trigger(element.id, 'hover'));
  node.addEventListener('mouseleave', () => trigger(element.id, 'leave'));
  if (interactive.has(element.id)) {
    node.style.cursor = 'pointer';
    node.setAttribute('tabindex', '0');
    node.addEventListener('keydown', (event) => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        trigger(element.id, 'click');
      }
    });
  }
}

window.DeepDesign = { playAnimation, stopAll, replay, playAll, design: DESIGN };
const urlParams = new URLSearchParams(location.search);
if (!urlParams.has('noanim')) {
  setTimeout(replay, 60);
}
)JS");
    html += QStringLiteral("</script>\n</body>\n</html>\n");
    return html;
}

bool HtmlExporter::exportPackage(const DesignDocument& document, const QString& directory, QString* error)
{
    QDir dir(directory);
    if (!dir.exists() && !QDir().mkpath(directory)) {
        if (error) *error = QStringLiteral("无法创建导出目录：%1").arg(directory);
        return false;
    }
    QSaveFile file(dir.filePath(QStringLiteral("index.html")));
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = QStringLiteral("无法写入 index.html：%1").arg(file.errorString());
        return false;
    }
    const QByteArray html = generateHtml(document).toUtf8();
    if (file.write(html) != html.size() || !file.commit()) {
        if (error) *error = QStringLiteral("保存 index.html 失败：%1").arg(file.errorString());
        return false;
    }
    return SpecExporter::writeFiles(document, directory, error);
}

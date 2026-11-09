#include "PreviewDialog.h"
#include "Animation.h"
#include "Icons.h"
#include "Renderer.h"
#include "Theme.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineF>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>
#include <cmath>
#include <functional>

namespace {
double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab = b - a;
    const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
    if (len2 < 0.000001) return QLineF(p, a).length();
    double t = ((p.x() - a.x()) * ab.x() + (p.y() - a.y()) * ab.y()) / len2;
    t = qBound(0.0, t, 1.0);
    return QLineF(p, a + ab * t).length();
}
} // namespace

// ===========================================================================
// PreviewCanvas
// ===========================================================================
class PreviewCanvas : public QWidget {
public:
    explicit PreviewCanvas(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        m_timer = new QTimer(this);
        m_timer->setInterval(16);
        m_clock.start();
        connect(m_timer, &QTimer::timeout, this, [this]() {
            if (m_paused) return;
            const double delta = m_clock.restart() / 1000.0;
            bool anyActive = false;
            for (auto& active : m_active) {
                const Animation* animation = m_document.findAnimation(active.animationId);
                if (!animation) continue;
                if (active.done) {
                    anyActive = true;
                    continue;
                }
                active.elapsed += delta;
                if (active.elapsed >= animation->duration) {
                    if (animation->loop) {
                        active.elapsed = std::fmod(active.elapsed, animation->duration);
                        anyActive = true;
                    } else {
                        active.elapsed = animation->duration;
                        active.done = true;
                    }
                } else {
                    anyActive = true;
                }
            }
            update();
            if (!anyActive) m_timer->stop();
        });
    }

    void setDocument(const DesignDocument& document)
    {
        m_document = document;
        update();
    }

    void replay()
    {
        m_active.clear();
        m_paused = false;
        m_timer->start();
        m_clock.restart();
        m_hoveredId.clear();
        for (const auto& interaction : m_document.interactions) {
            if (interaction.trigger == QLatin1String("load")) execute(interaction);
        }
        update();
    }

    void playAll()
    {
        m_active.clear();
        for (const auto& animation : m_document.animations) {
            playAnimation(animation.id);
        }
        m_paused = false;
        m_timer->start();
        m_clock.restart();
        update();
    }

    void togglePause()
    {
        m_paused = !m_paused;
        if (!m_paused) {
            m_clock.restart();
            m_timer->start();
        }
        update();
    }

    bool isPaused() const { return m_paused; }
    std::function<void()> closeRequested;
    std::function<void()> replayRequested;
    std::function<void()> statusChanged;

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient bg(0, 0, 0, height());
        bg.setColorAt(0.0, QColor("#151515"));
        bg.setColorAt(1.0, QColor("#0D0D0D"));
        p.fillRect(rect(), bg);

        const QRectF content = rect().adjusted(24, 24, -24, -24);
        const double scale = qMin(content.width() / m_document.canvasSize.width(),
                                  content.height() / m_document.canvasSize.height());
        const double safeScale = qMax(0.01, scale);
        const QSizeF artboardSize(m_document.canvasSize.width() * safeScale,
                                  m_document.canvasSize.height() * safeScale);
        const QPointF origin(content.center().x() - artboardSize.width() / 2,
                             content.center().y() - artboardSize.height() / 2);
        const QRectF artboard(origin, artboardSize);
        m_origin = origin;
        m_scale = safeScale;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 110));
        p.drawRoundedRect(artboard.translated(0, 12).adjusted(-8, -8, 8, 8), 8, 8);

        p.save();
        p.setClipRect(artboard);
        p.translate(origin);
        p.scale(safeScale, safeScale);
        Renderer::paintScene(p, m_document, currentOverrides(), false, safeScale);
        p.restore();

        p.setPen(QPen(QColor("#3A3A3A"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(artboard);

        // Subtle bottom hint
        const QString hint = m_paused ? QStringLiteral("已暂停 · 空格继续 · Esc 退出")
                                      : QStringLiteral("Esc 退出 · R 重播 · 空格暂停");
        QFont f = font();
        f.setPixelSize(11);
        p.setFont(f);
        const int textWidth = p.fontMetrics().horizontalAdvance(hint) + 24;
        const QRectF hintBox(width() / 2.0 - textWidth / 2.0, height() - 35, textWidth, 23);
        p.setPen(QPen(QColor(255, 255, 255, 26), 1));
        p.setBrush(QColor(20, 20, 20, 210));
        p.drawRoundedRect(hintBox, 11, 11);
        p.setPen(QColor("#A8A8A8"));
        p.drawText(hintBox, Qt::AlignCenter, hint);

        if (m_paused) {
            QFont pf = font();
            pf.setPixelSize(13);
            pf.setBold(true);
            p.setFont(pf);
            const QRectF pauseBox(width() / 2.0 - 56, 22, 112, 30);
            p.setPen(QPen(QColor(255, 255, 255, 30), 1));
            p.setBrush(QColor(30, 30, 30, 225));
            p.drawRoundedRect(pauseBox, 6, 6);
            p.setPen(QColor("#E0E0E0"));
            p.drawText(pauseBox, Qt::AlignCenter, QStringLiteral("暂停中"));
        }
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        setFocus();
        if (event->button() != Qt::LeftButton) return;
        const QString id = elementAt(event->position());
        if (id.isEmpty()) return;
        for (const auto& interaction : m_document.interactions) {
            if (interaction.sourceId == id && interaction.trigger == QLatin1String("click")) {
                execute(interaction);
            }
        }
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const QString id = elementAt(event->position());
        if (id != m_hoveredId) {
            if (!m_hoveredId.isEmpty()) {
                for (const auto& interaction : m_document.interactions) {
                    if (interaction.sourceId == m_hoveredId &&
                        interaction.trigger == QLatin1String("leave")) {
                        execute(interaction);
                    }
                }
            }
            m_hoveredId = id;
            if (!id.isEmpty()) {
                for (const auto& interaction : m_document.interactions) {
                    if (interaction.sourceId == id && interaction.trigger == QLatin1String("hover")) {
                        execute(interaction);
                    }
                }
            }
            update();
        }
        bool interactive = false;
        if (!id.isEmpty()) {
            for (const auto& interaction : m_document.interactions) {
                if (interaction.sourceId == id &&
                    (interaction.trigger == QLatin1String("click") ||
                     interaction.trigger == QLatin1String("hover"))) {
                    interactive = true;
                    break;
                }
            }
        }
        setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Escape) {
            if (closeRequested) closeRequested();
            return;
        }
        if (event->key() == Qt::Key_R) {
            if (replayRequested) replayRequested();
            return;
        }
        if (event->key() == Qt::Key_Space) {
            togglePause();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    struct ActiveAnimation {
        QString animationId;
        double elapsed = 0.0;
        bool done = false;
    };

    QHash<QString, QVariantMap> currentOverrides() const
    {
        QHash<QString, QVariantMap> overrides;
        for (const auto& active : m_active) {
            const Animation* animation = m_document.findAnimation(active.animationId);
            if (!animation) continue;
            double time = active.elapsed;
            if (animation->loop && animation->duration > 0.0) {
                time = std::fmod(time, animation->duration);
            }
            const auto sample = m_document.sampleAnimation(active.animationId, time);
            for (auto it = sample.constBegin(); it != sample.constEnd(); ++it) {
                for (auto prop = it.value().constBegin(); prop != it.value().constEnd(); ++prop) {
                    overrides[it.key()][prop.key()] = prop.value();
                }
            }
        }
        return overrides;
    }

    void playAnimation(const QString& animationId)
    {
        if (!m_document.findAnimation(animationId)) return;
        for (auto& active : m_active) {
            if (active.animationId == animationId) {
                active.elapsed = 0.0;
                active.done = false;
                m_timer->start();
                m_clock.restart();
                return;
            }
        }
        ActiveAnimation active;
        active.animationId = animationId;
        m_active.append(active);
        m_timer->start();
        m_clock.restart();
    }

    void execute(const Interaction& interaction)
    {
        if (interaction.action == QLatin1String("stopAll")) {
            m_active.clear();
            update();
            return;
        }
        if (interaction.action == QLatin1String("playAnimation") && !interaction.animationId.isEmpty()) {
            playAnimation(interaction.animationId);
        }
    }

    QPointF mapToDocument(const QPointF& screen) const
    {
        if (m_scale <= 0.00001) return {};
        return (screen - m_origin) / m_scale;
    }

    QString elementAt(const QPointF& screen) const
    {
        const QPointF doc = mapToDocument(screen);
        for (int i = m_document.elements.size() - 1; i >= 0; --i) {
            const Element& element = m_document.elements[i];
            if (!element.visible) continue;
            const QRectF r = element.boundingRect();
            const QPointF center = r.center();
            const double rad = qDegreesToRadians(-element.rotation);
            const double dx = doc.x() - center.x();
            const double dy = doc.y() - center.y();
            const QPointF local(center.x() + dx * qCos(rad) - dy * qSin(rad),
                                center.y() + dx * qSin(rad) + dy * qCos(rad));
            bool hit = false;
            if (element.type == ElementType::Line) {
                const QPointF a(r.x() + element.lineStart.x() * r.width(),
                                r.y() + element.lineStart.y() * r.height());
                const QPointF b(r.x() + element.lineEnd.x() * r.width(),
                                r.y() + element.lineEnd.y() * r.height());
                hit = distanceToSegment(local, a, b) <=
                      qMax(8.0, element.strokeWidth / 2.0 + 3.0);
            } else if (element.type == ElementType::Text) {
                hit = r.contains(local);
            } else {
                const QPainterPath path = Renderer::shapePath(element);
                hit = path.contains(local);
                if (!hit) {
                    QPainterPathStroker stroker;
                    stroker.setWidth(qMax(element.strokeWidth, 8.0));
                    hit = stroker.createStroke(path).contains(local);
                }
            }
            if (hit) return element.id;
        }
        return {};
    }

    DesignDocument m_document;
    QVector<ActiveAnimation> m_active;
    QTimer* m_timer = nullptr;
    QElapsedTimer m_clock;
    bool m_paused = false;
    QString m_hoveredId;
    QPointF m_origin;
    double m_scale = 1.0;
};

// ===========================================================================
// PreviewDialog
// ===========================================================================
PreviewDialog::PreviewDialog(const DesignDocument& document, QWidget* parent)
    : QDialog(parent)
    , m_document(document)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setStyleSheet(QStringLiteral("QDialog { background: #111111; }"));
    buildUi();

    const QRect available = screen() ? screen()->availableGeometry() : QRect(0, 0, 1280, 800);
    resize(qMin(available.width() - 60, 1500), qMin(available.height() - 60, 950));
    move(available.center() - rect().center());
    setFocusPolicy(Qt::StrongFocus);
}

void PreviewDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* top = new QWidget(this);
    top->setFixedHeight(44);
    top->setStyleSheet(QStringLiteral("background: #1E1E1E; border-bottom: 1px solid #303030;"));
    auto* topLayout = new QHBoxLayout(top);
    topLayout->setContentsMargins(14, 0, 10, 0);
    topLayout->setSpacing(8);

    auto* logo = new QLabel(top);
    QPixmap logoPixmap(44, 44);
    logoPixmap.setDevicePixelRatio(2.0);
    logoPixmap.fill(Qt::transparent);
    {
        QPainter painter(&logoPixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        Icons::drawLogo(painter, QRectF(3, 5, 32, 32), false);
    }
    logo->setPixmap(logoPixmap);
    topLayout->addWidget(logo);

    auto* title = new QLabel(QStringLiteral("交互预览  ·  %1").arg(m_document.title), top);
    title->setStyleSheet(QStringLiteral("color: #E4E4E4; font-weight: 600; font-size: 13px;"));
    topLayout->addWidget(title);
    topLayout->addStretch(1);

    m_statusLabel = new QLabel(QStringLiteral("点击或悬停对象触发交互"), top);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8A8A8A; font-size: 11px;"));
    topLayout->addWidget(m_statusLabel);
    topLayout->addSpacing(8);

    auto* playAll = new QPushButton(QStringLiteral("播放全部"), top);
    playAll->setObjectName(QStringLiteral("GhostButton"));
    connect(playAll, &QPushButton::clicked, this, [this]() { m_canvas->playAll(); });
    topLayout->addWidget(playAll);

    auto* replay = new QPushButton(QStringLiteral("重播 R"), top);
    replay->setObjectName(QStringLiteral("GhostButton"));
    connect(replay, &QPushButton::clicked, this, [this]() { m_canvas->replay(); });
    topLayout->addWidget(replay);

    auto* closeButton = new QPushButton(QStringLiteral("退出 Esc"), top);
    closeButton->setObjectName(QStringLiteral("PrimaryButton"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    topLayout->addWidget(closeButton);
    root->addWidget(top);

    m_canvas = new PreviewCanvas(this);
    m_canvas->setDocument(m_document);
    m_canvas->closeRequested = [this]() { close(); };
    m_canvas->replayRequested = [this]() { m_canvas->replay(); };
    root->addWidget(m_canvas, 1);

    connect(this, &QDialog::finished, this, [this]() { m_canvas->setFocus(); });
}

void PreviewDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    if (event->key() == Qt::Key_R) {
        if (m_canvas) m_canvas->replay();
        return;
    }
    if (event->key() == Qt::Key_Space) {
        if (m_canvas) m_canvas->togglePause();
        return;
    }
    QDialog::keyPressEvent(event);
}

void PreviewDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        if (!m_canvas) return;
        m_canvas->replay();
        m_canvas->setFocus();
    });
}

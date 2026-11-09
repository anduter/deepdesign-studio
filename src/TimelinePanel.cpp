#include "TimelinePanel.h"
#include "Animation.h"
#include "CanvasWidget.h"
#include "Icons.h"
#include "Theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
constexpr int kLabelWidth = 172;
constexpr int kRulerHeight = 27;
constexpr int kRowHeight = 24;
constexpr int kRightMargin = 14;

QColor propertyColor(const QString& property)
{
    if (property == QLatin1String("x") || property == QLatin1String("y")) return QColor("#5AA9F0");
    if (property == QLatin1String("width") || property == QLatin1String("height")) return QColor("#3DDC97");
    if (property == QLatin1String("rotation")) return QColor("#FFB454");
    if (property == QLatin1String("opacity")) return QColor("#B8B8B8");
    if (property == QLatin1String("fill")) return QColor("#FF6B81");
    if (property == QLatin1String("strokeWidth")) return QColor("#D6A35C");
    return QColor("#9A9A9A");
}

double niceTimeStep(double duration, double pixelWidth)
{
    static const double steps[] = {0.05, 0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 5.0,
                                   10.0, 20.0, 30.0, 60.0, 120.0};
    if (duration <= 0.0 || pixelWidth <= 1.0) return 1.0;
    const double pxPerSecond = pixelWidth / duration;
    for (double step : steps) {
        if (step * pxPerSecond >= 58.0) return step;
    }
    return 120.0;
}
} // namespace

// ===========================================================================
// TimelineCanvas
// ===========================================================================
class TimelineCanvas : public QWidget {
public:
    explicit TimelineCanvas(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumHeight(100);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setData(DesignDocument* document, const QString& animationId)
    {
        m_document = document;
        m_animationId = animationId;
        m_selectedTrack = -1;
        m_selectedKey = -1;
        rebuildRows();
        update();
    }

    void setTime(double time)
    {
        m_time = time;
        update();
    }

    void setSelection(const QStringList& ids)
    {
        m_selection = ids;
        update();
    }

    void clearKeySelection()
    {
        m_selectedTrack = -1;
        m_selectedKey = -1;
        update();
    }

    int selectedTrack() const { return m_selectedTrack; }
    int selectedKey() const { return m_selectedKey; }

    std::function<void(double)> timeScrubbed;
    std::function<void(int, int)> keyframeSelected;
    std::function<void()> keyframeMoveStarted;
    std::function<void(int, int, double)> keyframeMoved;
    std::function<void()> keyframeMoveFinished;
    std::function<void(int, int)> deleteKeyframeRequested;

    void refreshRows()
    {
        const int oldScroll = m_rowScroll;
        rebuildRows();
        m_rowScroll = qBound(0, oldScroll, maxScroll());
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor("#242424"));

        const int timeLeft = kLabelWidth;
        const int timeWidth = qMax(1, width() - kLabelWidth - kRightMargin);
        const double duration = qMax(0.1, currentDuration());
        const double pxPerSecond = timeWidth / duration;

        p.fillRect(QRect(0, 0, width(), kRulerHeight), QColor("#2D2D2D"));
        p.fillRect(QRect(0, 0, kLabelWidth, height()), QColor("#262626"));
        p.setPen(QPen(QColor("#383838"), 1));
        p.drawLine(0, kRulerHeight, width(), kRulerHeight);
        p.drawLine(kLabelWidth, 0, kLabelWidth, height());

        QFont small = font();
        small.setPixelSize(10);
        p.setFont(small);
        p.setPen(QColor("#D0D0D0"));
        p.drawText(QRect(9, 0, kLabelWidth - 18, kRulerHeight), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("图层 / 属性"));

        if (!m_document || m_animationId.isEmpty() || !animation()) {
            p.setPen(QColor("#7A7A7A"));
            QFont f = font();
            f.setPixelSize(12);
            p.setFont(f);
            p.drawText(QRect(kLabelWidth, kRulerHeight, timeWidth, height() - kRulerHeight),
                       Qt::AlignCenter, QStringLiteral("创建或选择一个动画以开始编辑时间轴"));
            return;
        }

        // Ruler ticks
        const double step = niceTimeStep(duration, timeWidth);
        const double minor = step / 5.0;
        for (double t = 0.0; t <= duration + 0.00001; t += step) {
            const double x = timeLeft + t * pxPerSecond;
            p.setPen(QPen(QColor("#454545"), 1));
            p.drawLine(QPointF(x, kRulerHeight - 7), QPointF(x, kRulerHeight));
            p.setPen(QColor("#A8A8A8"));
            const int decimals = step < 0.1 ? 2 : (step < 1.0 ? 1 : 0);
            p.drawText(QRectF(x + 3, 2, 74, 12), Qt::AlignLeft | Qt::AlignVCenter,
                       QStringLiteral("%1s").arg(QString::number(t, 'f', decimals)));
            p.setPen(QPen(QColor("#2F2F2F"), 1, Qt::DashLine));
            p.drawLine(QPointF(x, kRulerHeight), QPointF(x, height()));
            for (int k = 1; k < 5; ++k) {
                const double mx = timeLeft + (t + minor * k) * pxPerSecond;
                p.setPen(QPen(QColor("#3B3B3B"), 1));
                p.drawLine(QPointF(mx, kRulerHeight - 4), QPointF(mx, kRulerHeight));
            }
        }

        const int visibleRows = qMax(0, (height() - kRulerHeight - 4) / kRowHeight);
        m_rowScroll = qBound(0, m_rowScroll, qMax(0, int(m_rows.size()) - visibleRows));

        if (m_rows.isEmpty()) {
            p.setPen(QColor("#7A7A7A"));
            QFont f = font();
            f.setPixelSize(12);
            p.setFont(f);
            p.drawText(QRect(kLabelWidth, kRulerHeight, timeWidth, height() - kRulerHeight),
                       Qt::AlignCenter,
                       QStringLiteral("选择画布中的对象，点击「记录关键帧」创建动画轨道"));
            return;
        }

        for (int i = 0; i < visibleRows; ++i) {
            const int rowIndex = i + m_rowScroll;
            if (rowIndex >= m_rows.size()) break;
            const Row& row = m_rows[rowIndex];
            const QRect rowRect(0, kRulerHeight + i * kRowHeight, width(), kRowHeight);
            const bool selectedElement = m_selection.contains(row.elementId);
            p.fillRect(QRect(kLabelWidth, rowRect.top(), timeWidth, kRowHeight),
                       selectedElement ? QColor("#2E3642")
                                       : (rowIndex % 2 == 0 ? QColor("#282828") : QColor("#252525")));
            p.fillRect(QRect(0, rowRect.top(), kLabelWidth, kRowHeight),
                       selectedElement ? QColor("#343434") : QColor("#2A2A2A"));
            p.setPen(QPen(QColor("#363636"), 1));
            p.drawLine(0, rowRect.bottom(), width(), rowRect.bottom());

            p.setPen(Qt::NoPen);
            p.setBrush(propertyColor(row.property));
            p.drawEllipse(QPointF(11, rowRect.center().y()), 3.0, 3.0);

            QFont labelFont = font();
            labelFont.setPixelSize(11);
            p.setFont(labelFont);
            p.setPen(selectedElement ? QColor("#E6E6E6") : QColor("#BEBEBE"));
            const QString label = QStringLiteral("%1 · %2")
                                      .arg(row.elementName, trackPropertyDisplayName(row.property));
            const QString elided = p.fontMetrics().elidedText(label, Qt::ElideRight, kLabelWidth - 28);
            p.drawText(QRect(21, rowRect.top(), kLabelWidth - 28, kRowHeight),
                       Qt::AlignVCenter | Qt::AlignLeft, elided);

            const Track* track = trackForRow(row);
            if (!track) continue;
            for (int k = 0; k < track->keys.size(); ++k) {
                const Keyframe& key = track->keys[k];
                const double x = timeLeft + qBound(0.0, key.time, duration) * pxPerSecond;
                const QPointF center(x, rowRect.center().y());
                const bool selectedKey = (rowIndex == m_selectedRow && k == m_selectedKey);
                QPolygonF diamond;
                diamond << center + QPointF(0, -5) << center + QPointF(5, 0)
                        << center + QPointF(0, 5) << center + QPointF(-5, 0);
                QColor keyColor("#DCDCDC");
                if (row.property == QLatin1String("fill")) {
                    const QColor color = key.value.value<QColor>();
                    if (color.isValid()) keyColor = color;
                }
                p.setPen(QPen(selectedKey ? QColor("#FFFFFF") : QColor("#1E1E1E"),
                              selectedKey ? 1.8 : 1.0));
                p.setBrush(selectedKey ? QColor("#2D8CEB") : keyColor);
                p.drawPolygon(diamond);
            }
        }

        // Playhead
        const double playheadX = timeLeft + qBound(0.0, m_time, duration) * pxPerSecond;
        p.setPen(QPen(QColor("#2D8CEB"), 1.2));
        p.drawLine(QPointF(playheadX, kRulerHeight), QPointF(playheadX, height()));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#2D8CEB"));
        QPolygonF head;
        head << QPointF(playheadX - 5, kRulerHeight - 9)
             << QPointF(playheadX + 5, kRulerHeight - 9)
             << QPointF(playheadX, kRulerHeight - 1);
        p.drawPolygon(head);

        QFont bubbleFont = font();
        bubbleFont.setPixelSize(10);
        p.setFont(bubbleFont);
        const QString timeText = QStringLiteral("%1s").arg(QString::number(m_time, 'f', 2));
        const int bubbleWidth = p.fontMetrics().horizontalAdvance(timeText) + 12;
        double bubbleX = playheadX + 6;
        if (bubbleX + bubbleWidth > width() - 4) bubbleX = playheadX - bubbleWidth - 6;
        const QRectF bubble(bubbleX, kRulerHeight + 3, bubbleWidth, 17);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(45, 140, 235, 215));
        p.drawRoundedRect(bubble, 3, 3);
        p.setPen(Qt::white);
        p.drawText(bubble, Qt::AlignCenter, timeText);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        setFocus();
        if (!animation() || event->button() != Qt::LeftButton) return;
        const int x = qRound(event->position().x());
        const int y = qRound(event->position().y());
        if (x < 0 || y < 0 || x > width()) return;

        if (y < kRulerHeight) {
            if (x >= kLabelWidth) {
                m_dragMode = 1;
                setTimeFromX(x);
            }
            return;
        }
        if (x < kLabelWidth) return;

        const int visibleRows = qMax(1, (height() - kRulerHeight - 4) / kRowHeight);
        const int viewRow = (y - kRulerHeight) / kRowHeight;
        if (viewRow < 0 || viewRow >= visibleRows) return;
        const int rowIndex = viewRow + m_rowScroll;
        if (rowIndex < 0 || rowIndex >= m_rows.size()) {
            m_dragMode = 1;
            setTimeFromX(x);
            return;
        }

        const Row& row = m_rows[rowIndex];
        const Track* track = trackForRow(row);
        const int timeLeft = kLabelWidth;
        const int timeWidth = qMax(1, width() - kLabelWidth - kRightMargin);
        const double pxPerSecond = timeWidth / qMax(0.1, currentDuration());
        if (track) {
            for (int k = 0; k < track->keys.size(); ++k) {
                const double keyX = timeLeft + qBound(0.0, track->keys[k].time, currentDuration()) * pxPerSecond;
                if (qAbs(keyX - x) <= 7) {
                    m_selectedRow = rowIndex;
                    m_selectedKey = k;
                    m_selectedTrack = row.trackIndex;
                    if (keyframeSelected) keyframeSelected(row.trackIndex, k);
                    if (keyframeMoveStarted) keyframeMoveStarted();
                    m_dragMode = 2;
                    setCursor(Qt::SizeHorCursor);
                    update();
                    return;
                }
            }
        }
        m_selectedRow = -1;
        m_selectedKey = -1;
        m_selectedTrack = -1;
        m_dragMode = 1;
        setTimeFromX(x);
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const int x = qRound(event->position().x());
        if (m_dragMode == 1) {
            setTimeFromX(x);
            return;
        }
        if (m_dragMode == 2 && m_selectedTrack >= 0 && m_selectedKey >= 0 && animation()) {
            if (m_selectedTrack >= animation()->tracks.size()) return;
            Track& track = animation()->tracks[m_selectedTrack];
            if (m_selectedKey >= track.keys.size()) return;
            const int timeLeft = kLabelWidth;
            const int timeWidth = qMax(1, width() - kLabelWidth - kRightMargin);
            const double duration = qMax(0.1, currentDuration());
            const double time = qBound(0.0, double(x - timeLeft) / timeWidth * duration, duration);
            track.keys[m_selectedKey].time = qRound(time * 1000.0) / 1000.0;
            if (timeScrubbed) timeScrubbed(track.keys[m_selectedKey].time);
            if (keyframeMoved) keyframeMoved(m_selectedTrack, m_selectedKey,
                                             track.keys[m_selectedKey].time);
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        Q_UNUSED(event);
        if (m_dragMode == 2 && keyframeMoveFinished) keyframeMoveFinished();
        m_dragMode = 0;
        setCursor(Qt::ArrowCursor);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (!animation() || event->button() != Qt::LeftButton) return;
        const int x = qRound(event->position().x());
        const int y = qRound(event->position().y());
        if (x < kLabelWidth || y < kRulerHeight) return;
        const int viewRow = (y - kRulerHeight) / kRowHeight;
        const int rowIndex = viewRow + m_rowScroll;
        if (rowIndex < 0 || rowIndex >= m_rows.size()) return;
        const Row& row = m_rows[rowIndex];
        const Track* track = trackForRow(row);
        if (!track) return;
        const int timeWidth = qMax(1, width() - kLabelWidth - kRightMargin);
        const double pxPerSecond = timeWidth / qMax(0.1, currentDuration());
        for (int k = 0; k < track->keys.size(); ++k) {
            const double keyX = kLabelWidth + qBound(0.0, track->keys[k].time, currentDuration()) * pxPerSecond;
            if (qAbs(keyX - x) <= 7) {
                if (deleteKeyframeRequested) deleteKeyframeRequested(row.trackIndex, k);
                return;
            }
        }
    }

    void wheelEvent(QWheelEvent* event) override
    {
        const int visibleRows = qMax(1, (height() - kRulerHeight - 4) / kRowHeight);
        (void)visibleRows;
        if (maxScroll() <= 0) {
            event->ignore();
            return;
        }
        m_rowScroll = qBound(0, m_rowScroll + (event->angleDelta().y() > 0 ? -1 : 1), maxScroll());
        update();
        event->accept();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) &&
            m_selectedTrack >= 0 && m_selectedKey >= 0) {
            if (deleteKeyframeRequested) deleteKeyframeRequested(m_selectedTrack, m_selectedKey);
            return;
        }
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
            if (!animation()) return;
            const double delta = (event->modifiers() & Qt::ShiftModifier) ? 0.1 : 1.0 / 30.0;
            double time = m_time + (event->key() == Qt::Key_Right ? delta : -delta);
            time = qBound(0.0, time, currentDuration());
            m_time = time;
            if (timeScrubbed) timeScrubbed(time);
            update();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    struct Row {
        QString elementId;
        QString elementName;
        QString property;
        int trackIndex = -1;
    };

    Animation* animation() const
    {
        if (!m_document || m_animationId.isEmpty()) return nullptr;
        return m_document->findAnimation(m_animationId);
    }

    double currentDuration() const
    {
        const Animation* anim = animation();
        return anim ? qMax(0.1, anim->duration) : 1.0;
    }

    const Track* trackForRow(const Row& row) const
    {
        const Animation* anim = animation();
        if (!anim || row.trackIndex < 0 || row.trackIndex >= anim->tracks.size()) return nullptr;
        return &anim->tracks[row.trackIndex];
    }

    int maxScroll() const
    {
        const int visibleRows = qMax(1, (height() - kRulerHeight - 4) / kRowHeight);
        return qMax(0, int(m_rows.size()) - visibleRows);
    }

    void rebuildRows()
    {
        m_rows.clear();
        if (!m_document) return;
        const Animation* anim = m_document->findAnimation(m_animationId);
        if (!anim) return;
        const QStringList order = {QStringLiteral("x"), QStringLiteral("y"),
                                   QStringLiteral("width"), QStringLiteral("height"),
                                   QStringLiteral("rotation"), QStringLiteral("opacity"),
                                   QStringLiteral("fill"), QStringLiteral("strokeWidth")};
        for (int e = m_document->elements.size() - 1; e >= 0; --e) {
            const Element& element = m_document->elements[e];
            for (const auto& property : order) {
                for (int t = 0; t < anim->tracks.size(); ++t) {
                    const Track& track = anim->tracks[t];
                    if (track.elementId == element.id && track.property == property &&
                        !track.keys.isEmpty()) {
                        Row row;
                        row.elementId = element.id;
                        row.elementName = element.name;
                        row.property = property;
                        row.trackIndex = t;
                        m_rows.append(row);
                        break;
                    }
                }
            }
        }
    }

    void setTimeFromX(int x)
    {
        if (!animation()) return;
        const int timeWidth = qMax(1, width() - kLabelWidth - kRightMargin);
        const double duration = currentDuration();
        const double time = qBound(0.0, double(x - kLabelWidth) / timeWidth * duration, duration);
        m_time = qRound(time * 1000.0) / 1000.0;
        if (timeScrubbed) timeScrubbed(m_time);
        update();
    }

    DesignDocument* m_document = nullptr;
    QString m_animationId;
    QStringList m_selection;
    QVector<Row> m_rows;
    double m_time = 0.0;
    int m_rowScroll = 0;
    int m_selectedRow = -1;
    int m_selectedKey = -1;
    int m_selectedTrack = -1;
    int m_dragMode = 0;
};

// ===========================================================================
// TimelinePanel
// ===========================================================================
TimelinePanel::TimelinePanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();

    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(16);
    connect(m_playTimer, &QTimer::timeout, this, &TimelinePanel::onTick);
}

void TimelinePanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QFrame(this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    header->setFixedHeight(30);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 0, 6, 0);
    headerLayout->setSpacing(5);

    auto* title = new QLabel(QStringLiteral("时间轴"), header);
    title->setObjectName(QStringLiteral("PanelTitle"));
    headerLayout->addWidget(title);

    m_animationCombo = new QComboBox(header);
    m_animationCombo->setEditable(true);
    m_animationCombo->setMinimumWidth(170);
    m_animationCombo->setMaximumWidth(230);
    m_animationCombo->setToolTip(QStringLiteral("选择动画，可直接输入名称重命名"));
    headerLayout->addWidget(m_animationCombo);

    m_addAnimationButton = new QToolButton(header);
    m_addAnimationButton->setIcon(Icons::icon(QStringLiteral("plus"), QColor("#C8C8C8"), 16));
    m_addAnimationButton->setFixedSize(23, 23);
    m_addAnimationButton->setToolTip(QStringLiteral("新建动画"));
    headerLayout->addWidget(m_addAnimationButton);

    m_deleteAnimationButton = new QToolButton(header);
    m_deleteAnimationButton->setIcon(Icons::icon(QStringLiteral("trash"), QColor("#C8C8C8"), 15));
    m_deleteAnimationButton->setFixedSize(23, 23);
    m_deleteAnimationButton->setToolTip(QStringLiteral("删除当前动画"));
    headerLayout->addWidget(m_deleteAnimationButton);

    headerLayout->addStretch(1);
    m_hintLabel = new QLabel(QStringLiteral("双击关键帧删除 · 拖动关键帧改变时间"), header);
    m_hintLabel->setStyleSheet(QStringLiteral("color: #858585; font-size: 11px;"));
    headerLayout->addWidget(m_hintLabel);
    root->addWidget(header);

    auto* controls = new QWidget(this);
    controls->setFixedHeight(36);
    controls->setStyleSheet(QStringLiteral("background: #2B2B2B; border-bottom: 1px solid #1A1A1A;"));
    auto* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(8, 3, 8, 3);
    controlsLayout->setSpacing(5);

    m_playButton = new QToolButton(controls);
    m_playButton->setIcon(Icons::icon(QStringLiteral("play"), QColor("#D6D6D6"), 18));
    m_playButton->setFixedSize(27, 27);
    m_playButton->setToolTip(QStringLiteral("播放 / 暂停"));
    controlsLayout->addWidget(m_playButton);

    m_stopButton = new QToolButton(controls);
    m_stopButton->setIcon(Icons::icon(QStringLiteral("stop"), QColor("#C8C8C8"), 15));
    m_stopButton->setFixedSize(25, 27);
    m_stopButton->setToolTip(QStringLiteral("停止并回到 0 秒"));
    controlsLayout->addWidget(m_stopButton);

    m_timeLabel = new QLabel(QStringLiteral("0.00 / 1.00 s"), controls);
    QFont mono(QStringLiteral("Consolas"));
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(9);
    m_timeLabel->setFont(mono);
    m_timeLabel->setStyleSheet(QStringLiteral("color: #D6D6D6; padding: 0 6px;"));
    m_timeLabel->setFixedWidth(104);
    controlsLayout->addWidget(m_timeLabel);

    auto* sep1 = new QFrame(controls);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QStringLiteral("background: #444444; max-width: 1px;"));
    sep1->setFixedHeight(20);
    controlsLayout->addWidget(sep1);

    auto* durationLabel = new QLabel(QStringLiteral("时长"), controls);
    durationLabel->setObjectName(QStringLiteral("FieldLabel"));
    controlsLayout->addWidget(durationLabel);
    m_durationSpin = new QDoubleSpinBox(controls);
    m_durationSpin->setRange(0.1, 120.0);
    m_durationSpin->setDecimals(2);
    m_durationSpin->setSingleStep(0.1);
    m_durationSpin->setSuffix(QStringLiteral(" s"));
    m_durationSpin->setFixedWidth(76);
    m_durationSpin->setValue(1.0);
    controlsLayout->addWidget(m_durationSpin);

    auto* easingLabel = new QLabel(QStringLiteral("曲线"), controls);
    easingLabel->setObjectName(QStringLiteral("FieldLabel"));
    controlsLayout->addWidget(easingLabel);
    m_easingCombo = new QComboBox(controls);
    m_easingCombo->addItems(easingDisplayNames());
    m_easingCombo->setFixedWidth(96);
    controlsLayout->addWidget(m_easingCombo);

    m_loopCheck = new QCheckBox(QStringLiteral("循环"), controls);
    controlsLayout->addWidget(m_loopCheck);
    controlsLayout->addStretch(1);

    m_recordButton = new QToolButton(controls);
    m_recordButton->setText(QStringLiteral(" 记录关键帧"));
    m_recordButton->setIcon(Icons::icon(QStringLiteral("record"), QColor("#FF6B6B"), 13));
    m_recordButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_recordButton->setStyleSheet(QStringLiteral(
        "QToolButton { background: #3C3C3C; border: 1px solid #555555; border-radius: 3px; padding: 5px 9px; color: #E0E0E0; }"
        "QToolButton:hover { background: #4A4A4A; }"
        "QToolButton:disabled { color: #707070; background: #2B2B2B; border-color: #3A3A3A; }"));
    m_recordButton->setToolTip(QStringLiteral("在播放头位置记录选中对象的变换与透明度"));
    controlsLayout->addWidget(m_recordButton);

    m_deleteKeyButton = new QToolButton(controls);
    m_deleteKeyButton->setText(QStringLiteral(" 删除关键帧"));
    m_deleteKeyButton->setIcon(Icons::icon(QStringLiteral("trash"), QColor("#C8C8C8"), 14));
    m_deleteKeyButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_deleteKeyButton->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 5px 8px; color: #BEBEBE; }"
        "QToolButton:hover { background: #3A3A3A; }"
        "QToolButton:disabled { color: #666666; }"));
    controlsLayout->addWidget(m_deleteKeyButton);
    root->addWidget(controls);

    m_timelineCanvas = new TimelineCanvas(this);
    root->addWidget(m_timelineCanvas, 1);

    m_timelineCanvas->timeScrubbed = [this](double time) { onTimeScrubbed(time); };
    m_timelineCanvas->keyframeSelected = [this](int track, int key) {
        m_selectedTrack = track;
        m_selectedKey = key;
        updateControls();
    };
    m_timelineCanvas->keyframeMoveStarted = [this]() { onKeyframeMoveStarted(); };
    m_timelineCanvas->keyframeMoved = [this](int track, int key, double time) {
        onKeyframeMoved(track, key, time);
    };
    m_timelineCanvas->keyframeMoveFinished = [this]() { onKeyframeMoveFinished(); };
    m_timelineCanvas->deleteKeyframeRequested = [this](int track, int key) {
        deleteKeyframeAt(track, key);
    };

    connect(m_animationCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_updating) return;
        const QString id = m_animationCombo->currentData().toString();
        if (id.isEmpty()) return;
        setCurrentAnimation(id, true);
    });
    connect(m_animationCombo->lineEdit(), &QLineEdit::editingFinished, this,
            &TimelinePanel::renameCurrentAnimation);
    connect(m_addAnimationButton, &QToolButton::clicked, this, &TimelinePanel::addAnimation);
    connect(m_deleteAnimationButton, &QToolButton::clicked, this, &TimelinePanel::deleteAnimation);
    connect(m_playButton, &QToolButton::clicked, this, &TimelinePanel::togglePlayback);
    connect(m_stopButton, &QToolButton::clicked, this, &TimelinePanel::stopPlayback);
    connect(m_durationSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updating) return;
        Animation* animation = currentAnimation();
        if (!animation) return;
        emit editBegan();
        animation->duration = qMax(0.1, value);
        for (auto& track : animation->tracks) {
            for (auto& key : track.keys) key.time = qBound(0.0, key.time, animation->duration);
            track.sortKeys();
        }
        m_currentTime = qBound(0.0, m_currentTime, animation->duration);
        emit documentEdited();
        emit editEnded();
        updateTimeDisplay();
        m_timelineCanvas->refreshRows();
        m_timelineCanvas->setTime(m_currentTime);
        emit previewChanged(m_currentAnimationId, m_currentTime);
    });
    connect(m_easingCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_updating) return;
        Animation* animation = currentAnimation();
        if (!animation) return;
        emit editBegan();
        animation->easing = easingIdFromDisplay(m_easingCombo->itemText(index));
        emit documentEdited();
        emit editEnded();
    });
    connect(m_loopCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updating) return;
        Animation* animation = currentAnimation();
        if (!animation) return;
        emit editBegan();
        animation->loop = checked;
        emit documentEdited();
        emit editEnded();
    });
    connect(m_recordButton, &QToolButton::clicked, this, &TimelinePanel::recordKeyframes);
    connect(m_deleteKeyButton, &QToolButton::clicked, this, &TimelinePanel::deleteSelectedKeyframe);
}

void TimelinePanel::setContext(DesignDocument* document, CanvasWidget* canvas)
{
    m_document = document;
    m_canvas = canvas;
    if (!m_document) return;
    if (!m_document->animations.isEmpty()) {
        m_currentAnimationId = m_document->animations.first().id;
    } else {
        m_currentAnimationId.clear();
    }
    m_currentTime = 0.0;
    refresh();
}

void TimelinePanel::refresh()
{
    if (!m_document) return;
    m_updating = true;

    const QString previous = m_currentAnimationId;
    QSignalBlocker blocker(m_animationCombo);
    m_animationCombo->clear();
    for (const auto& animation : m_document->animations) {
        m_animationCombo->addItem(animation.name, animation.id);
    }
    if (m_document->animations.isEmpty()) {
        m_currentAnimationId.clear();
    } else {
        bool found = false;
        for (const auto& animation : m_document->animations) {
            if (animation.id == m_currentAnimationId) found = true;
        }
        if (!found) m_currentAnimationId = m_document->animations.first().id;
        const int index = m_animationCombo->findData(m_currentAnimationId);
        if (index >= 0) m_animationCombo->setCurrentIndex(index);
    }

    Animation* animation = currentAnimation();
    if (animation) {
        m_currentTime = qBound(0.0, m_currentTime, animation->duration);
        QSignalBlocker durationBlocker(m_durationSpin);
        QSignalBlocker easingBlocker(m_easingCombo);
        QSignalBlocker loopBlocker(m_loopCheck);
        m_durationSpin->setValue(animation->duration);
        m_easingCombo->setCurrentIndex(qMax(0, m_easingCombo->findText(easingDisplayFromId(animation->easing))));
        m_loopCheck->setChecked(animation->loop);
    }

    if (m_canvas) m_selection = m_canvas->selection();
    m_selectedTrack = -1;
    m_selectedKey = -1;
    m_timelineCanvas->setData(m_document, m_currentAnimationId);
    m_timelineCanvas->setSelection(m_selection);
    m_timelineCanvas->setTime(m_currentTime);
    m_updating = false;
    updateTimeDisplay();
    updateControls();
    Q_UNUSED(previous);
}

void TimelinePanel::setSelection(const QStringList& ids)
{
    m_selection = ids;
    m_timelineCanvas->setSelection(ids);
    updateControls();
}

void TimelinePanel::updateControls()
{
    Animation* animation = currentAnimation();
    const bool hasAnimation = animation != nullptr;
    const bool hasSelection = !m_selection.isEmpty();
    m_animationCombo->setEnabled(m_document && !m_document->animations.isEmpty());
    m_deleteAnimationButton->setEnabled(hasAnimation);
    m_playButton->setEnabled(hasAnimation);
    m_stopButton->setEnabled(hasAnimation);
    m_durationSpin->setEnabled(hasAnimation);
    m_easingCombo->setEnabled(hasAnimation);
    m_loopCheck->setEnabled(hasAnimation);
    m_recordButton->setEnabled(hasAnimation && hasSelection);
    m_deleteKeyButton->setEnabled(m_selectedTrack >= 0 && m_selectedKey >= 0);
    m_playButton->setIcon(Icons::icon(m_playing ? QStringLiteral("pause") : QStringLiteral("play"),
                                      QColor("#D6D6D6"), 18));
    if (!hasAnimation) {
        m_hintLabel->setText(QStringLiteral("点击 ＋ 新建动画，再为选中对象记录关键帧"));
    } else if (!hasSelection) {
        m_hintLabel->setText(QStringLiteral("选择对象后可在播放头位置记录关键帧"));
    } else {
        m_hintLabel->setText(QStringLiteral("双击关键帧删除 · 拖动关键帧改变时间"));
    }
}

void TimelinePanel::updateTimeDisplay()
{
    const Animation* animation = currentAnimation();
    const double duration = animation ? animation->duration : 0.0;
    m_timeLabel->setText(QStringLiteral("%1 / %2 s")
                             .arg(m_currentTime, 0, 'f', 2)
                             .arg(duration, 0, 'f', 2));
    if (m_timelineCanvas) m_timelineCanvas->setTime(m_currentTime);
}

void TimelinePanel::setCurrentAnimation(const QString& id, bool resetTime)
{
    if (m_currentAnimationId == id && !resetTime) return;
    if (m_playing) pausePlayback();
    m_currentAnimationId = id;
    if (resetTime) m_currentTime = 0.0;
    Animation* animation = currentAnimation();
    m_updating = true;
    if (animation) {
        QSignalBlocker durationBlocker(m_durationSpin);
        QSignalBlocker easingBlocker(m_easingCombo);
        QSignalBlocker loopBlocker(m_loopCheck);
        m_durationSpin->setValue(animation->duration);
        m_easingCombo->setCurrentIndex(qMax(0, m_easingCombo->findText(easingDisplayFromId(animation->easing))));
        m_loopCheck->setChecked(animation->loop);
        const int index = m_animationCombo->findData(id);
        if (index >= 0) {
            QSignalBlocker comboBlocker(m_animationCombo);
            m_animationCombo->setCurrentIndex(index);
        }
    }
    m_updating = false;
    m_selectedTrack = -1;
    m_selectedKey = -1;
    m_timelineCanvas->setData(m_document, m_currentAnimationId);
    m_timelineCanvas->setSelection(m_selection);
    m_timelineCanvas->clearKeySelection();
    m_timelineCanvas->setTime(m_currentTime);
    updateTimeDisplay();
    updateControls();
    emit previewChanged(m_currentAnimationId, m_currentTime);
}

Animation* TimelinePanel::currentAnimation() const
{
    if (!m_document || m_currentAnimationId.isEmpty()) return nullptr;
    return m_document->findAnimation(m_currentAnimationId);
}

void TimelinePanel::addAnimation()
{
    if (!m_document) return;
    emit editBegan();
    Animation& animation = m_document->addAnimation();
    animation.duration = 1.0;
    animation.easing = QStringLiteral("easeOutCubic");
    const QString id = animation.id;
    emit documentEdited();
    emit editEnded();
    m_currentAnimationId = id;
    m_currentTime = 0.0;
    refresh();
    QSignalBlocker blocker(m_animationCombo);
    const int index = m_animationCombo->findData(id);
    if (index >= 0) m_animationCombo->setCurrentIndex(index);
    emit statusMessage(QStringLiteral("已创建「%1」，选择对象后点击「记录关键帧」").arg(animation.name));
    if (!m_selection.isEmpty()) recordKeyframes();
}

void TimelinePanel::deleteAnimation()
{
    if (!m_document || m_currentAnimationId.isEmpty()) return;
    emit editBegan();
    m_document->removeAnimation(m_currentAnimationId);
    emit documentEdited();
    emit editEnded();
    if (m_playing) pausePlayback();
    m_currentAnimationId.clear();
    m_currentTime = 0.0;
    refresh();
    emit previewChanged(QString(), 0.0);
    emit statusMessage(QStringLiteral("动画已删除"));
}

void TimelinePanel::renameCurrentAnimation()
{
    if (m_updating || !m_document) return;
    Animation* animation = currentAnimation();
    if (!animation) return;
    const QString name = m_animationCombo->currentText().trimmed();
    if (name.isEmpty() || name == animation->name) return;
    emit editBegan();
    animation->name = name;
    emit documentEdited();
    emit editEnded();
    refresh();
}

void TimelinePanel::recordKeyframes()
{
    if (!m_document || m_currentAnimationId.isEmpty() || m_selection.isEmpty()) return;
    Animation* animation = currentAnimation();
    Element* element = m_document->findElement(m_selection.first());
    if (!animation || !element) return;
    const double time = qBound(0.0, m_currentTime, animation->duration);

    const QVector<QPair<QString, QVariant>> values = {
        {"x", element->frame.x()},
        {"y", element->frame.y()},
        {"width", element->frame.width()},
        {"height", element->frame.height()},
        {"rotation", element->rotation},
        {"opacity", element->opacity},
    };
    QVector<QPair<QString, QVariant>> all = values;
    if (element->type != ElementType::Text) {
        all.append({"fill", element->fill});
    }

    emit editBegan();
    for (const auto& entry : all) {
        Track& track = animation->ensureTrack(element->id, entry.first);
        bool updated = false;
        for (auto& key : track.keys) {
            if (qAbs(key.time - time) <= 0.0005) {
                key.value = entry.second;
                updated = true;
                break;
            }
        }
        if (!updated) {
            Keyframe key;
            key.time = time;
            key.value = entry.second;
            track.keys.append(key);
        }
        track.sortKeys();
    }
    emit documentEdited();
    emit editEnded();
    m_timelineCanvas->refreshRows();
    m_timelineCanvas->setTime(m_currentTime);
    updateControls();
    emit statusMessage(QStringLiteral("已在 %1s 记录 %2 个关键帧")
                           .arg(QString::number(time, 'f', 2))
                           .arg(all.size()));
}

void TimelinePanel::deleteSelectedKeyframe()
{
    if (m_selectedTrack < 0 || m_selectedKey < 0) return;
    deleteKeyframeAt(m_selectedTrack, m_selectedKey);
}

void TimelinePanel::deleteKeyframeAt(int trackIndex, int keyIndex)
{
    Animation* animation = currentAnimation();
    if (!animation || trackIndex < 0 || trackIndex >= animation->tracks.size()) return;
    Track& track = animation->tracks[trackIndex];
    if (keyIndex < 0 || keyIndex >= track.keys.size()) return;
    emit editBegan();
    track.keys.removeAt(keyIndex);
    if (track.keys.isEmpty()) animation->tracks.removeAt(trackIndex);
    m_selectedTrack = -1;
    m_selectedKey = -1;
    emit documentEdited();
    emit editEnded();
    refresh();
    emit statusMessage(QStringLiteral("已删除关键帧"));
}

void TimelinePanel::togglePlayback()
{
    if (!currentAnimation()) return;
    if (m_playing) {
        pausePlayback();
        return;
    }
    const Animation* animation = currentAnimation();
    if (!animation) return;
    if (m_currentTime >= animation->duration - 0.0005) m_currentTime = 0.0;
    m_playing = true;
    m_elapsed.start();
    m_playTimer->start();
    updateControls();
    emit previewChanged(m_currentAnimationId, m_currentTime);
    emit statusMessage(QStringLiteral("正在播放「%1」").arg(animation->name));
}

void TimelinePanel::pausePlayback()
{
    if (!m_playing) return;
    m_playing = false;
    m_playTimer->stop();
    updateControls();
    emit previewChanged(m_currentAnimationId, m_currentTime);
}

void TimelinePanel::stopPlayback()
{
    if (m_playing) {
        m_playing = false;
        m_playTimer->stop();
    }
    m_currentTime = 0.0;
    m_selectedTrack = -1;
    m_selectedKey = -1;
    if (m_timelineCanvas) {
        m_timelineCanvas->clearKeySelection();
        m_timelineCanvas->setTime(0.0);
    }
    updateTimeDisplay();
    updateControls();
    emit previewChanged(m_currentAnimationId, 0.0);
}

void TimelinePanel::playPause()
{
    togglePlayback();
}

void TimelinePanel::addAnimationAction()
{
    addAnimation();
}

void TimelinePanel::recordKeyframesAction()
{
    recordKeyframes();
}

void TimelinePanel::onTick()
{
    Animation* animation = currentAnimation();
    if (!animation || !m_playing) return;
    const double delta = m_elapsed.restart() / 1000.0;
    m_currentTime += delta;
    if (m_currentTime >= animation->duration) {
        if (animation->loop) {
            m_currentTime = std::fmod(m_currentTime, animation->duration);
        } else {
            m_currentTime = animation->duration;
            m_playing = false;
            m_playTimer->stop();
            updateControls();
        }
    }
    updateTimeDisplay();
    emit previewChanged(m_currentAnimationId, m_currentTime);
}

void TimelinePanel::onTimeScrubbed(double time)
{
    if (m_playing) pausePlayback();
    Animation* animation = currentAnimation();
    if (!animation) return;
    m_currentTime = qBound(0.0, time, animation->duration);
    updateTimeDisplay();
    updateControls();
    emit previewChanged(m_currentAnimationId, m_currentTime);
}

void TimelinePanel::onKeyframeMoveStarted()
{
    m_interacting = true;
    emit editBegan();
}

void TimelinePanel::onKeyframeMoved(int trackIndex, int keyIndex, double time)
{
    Animation* animation = currentAnimation();
    if (!animation || trackIndex < 0 || trackIndex >= animation->tracks.size()) return;
    Track& track = animation->tracks[trackIndex];
    if (keyIndex < 0 || keyIndex >= track.keys.size()) return;
    track.keys[keyIndex].time = qBound(0.0, time, animation->duration);
    emit documentEdited();
    updateTimeDisplay();
}

void TimelinePanel::onKeyframeMoveFinished()
{
    m_interacting = false;
    Animation* animation = currentAnimation();
    if (animation) {
        for (auto& track : animation->tracks) track.sortKeys();
    }
    m_selectedTrack = -1;
    m_selectedKey = -1;
    m_timelineCanvas->clearKeySelection();
    emit documentEdited();
    emit editEnded();
    refresh();
}

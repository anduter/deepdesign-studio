#pragma once

#include "Model.h"

#include <QElapsedTimer>
#include <QStringList>
#include <QWidget>

class CanvasWidget;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QToolButton;
class QTimer;
class TimelineCanvas;

class TimelinePanel : public QWidget {
    Q_OBJECT
public:
    explicit TimelinePanel(QWidget* parent = nullptr);

    void setContext(DesignDocument* document, CanvasWidget* canvas);
    void refresh();
    void setSelection(const QStringList& ids);
    void stopPlayback();
    void playPause();
    void addAnimationAction();
    void recordKeyframesAction();
    bool isPlaying() const { return m_playing; }
    bool isInteracting() const { return m_interacting; }
    QString currentAnimationId() const { return m_currentAnimationId; }

signals:
    void editBegan();
    void editEnded();
    void documentEdited();
    void previewChanged(const QString& animationId, double time);
    void statusMessage(const QString& message);

private:
    void buildUi();
    void updateControls();
    void updateTimeDisplay();
    void setCurrentAnimation(const QString& id, bool resetTime);
    void addAnimation();
    void deleteAnimation();
    void renameCurrentAnimation();
    void recordKeyframes();
    void deleteSelectedKeyframe();
    void deleteKeyframeAt(int trackIndex, int keyIndex);
    void togglePlayback();
    void pausePlayback();
    void onTick();
    void onTimeScrubbed(double time);
    void onKeyframeMoveStarted();
    void onKeyframeMoved(int trackIndex, int keyIndex, double time);
    void onKeyframeMoveFinished();
    Animation* currentAnimation() const;

    DesignDocument* m_document = nullptr;
    CanvasWidget* m_canvas = nullptr;
    QString m_currentAnimationId;
    QStringList m_selection;
    double m_currentTime = 0.0;
    bool m_playing = false;
    bool m_updating = false;
    bool m_interacting = false;

    QComboBox* m_animationCombo = nullptr;
    QToolButton* m_addAnimationButton = nullptr;
    QToolButton* m_deleteAnimationButton = nullptr;
    QToolButton* m_playButton = nullptr;
    QToolButton* m_stopButton = nullptr;
    QLabel* m_timeLabel = nullptr;
    QDoubleSpinBox* m_durationSpin = nullptr;
    QComboBox* m_easingCombo = nullptr;
    QCheckBox* m_loopCheck = nullptr;
    QLabel* m_hintLabel = nullptr;
    QToolButton* m_recordButton = nullptr;
    QToolButton* m_deleteKeyButton = nullptr;
    TimelineCanvas* m_timelineCanvas = nullptr;

    QTimer* m_playTimer = nullptr;
    QElapsedTimer m_elapsed;
    int m_selectedTrack = -1;
    int m_selectedKey = -1;
};

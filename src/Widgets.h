#pragma once

#include "Model.h"

#include <QColor>
#include <QToolButton>
#include <QWidget>
#include <functional>

class QMouseEvent;

class ColorButton : public QToolButton {
    Q_OBJECT
public:
    explicit ColorButton(QWidget* parent = nullptr);
    void setColor(const QColor& color);
    QColor color() const { return m_color; }
    void setCompact(bool compact);
    void setTextVisible(bool visible);
    QSize sizeHint() const override;

signals:
    void colorChanged(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void chooseColor();

    QColor m_color = QColor("#6C8CFF");
    bool m_compact = true;
    bool m_textVisible = true;
};

namespace ColorSystem {
QVector<QColor> presetColors();
QVector<QColor> recentColors();
void addRecentColor(const QColor& color);
} // namespace ColorSystem

class ColorPaletteWidget : public QWidget {
public:
    explicit ColorPaletteWidget(QWidget* parent = nullptr);
    std::function<void(const QColor&)> colorPicked;
    void addRecent(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int indexAt(const QPoint& point) const;
    QVector<QColor> allColors() const;
    int m_hover = -1;
};

class GradientEditor : public QWidget {
    Q_OBJECT
public:
    explicit GradientEditor(QWidget* parent = nullptr);

    void setGradient(const GradientFill& gradient);
    GradientFill gradient() const { return m_gradient; }
    int selectedStop() const { return m_selectedStop; }
    void setSelectedStop(int index);
    void addStop();
    void removeStop();

signals:
    void gradientChanged(const GradientFill& gradient);
    void editBegan();
    void editEnded();
    void selectedStopChanged(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int stopAt(const QPoint& point) const;
    double positionForX(double x) const;
    double xForPosition(double position) const;
    int barTop() const { return 8; }
    int barHeight() const { return 22; }
    int markerY() const { return 44; }
    void openColorDialog(int index);

    GradientFill m_gradient;
    int m_selectedStop = 0;
    int m_dragStop = -1;
    int m_hoverStop = -1;
    bool m_dragging = false;
};


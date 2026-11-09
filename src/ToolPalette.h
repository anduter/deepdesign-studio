#pragma once

#include "CanvasWidget.h"

#include <QHash>
#include <QWidget>

class QButtonGroup;
class QToolButton;

class ToolPalette : public QWidget {
    Q_OBJECT
public:
    explicit ToolPalette(QWidget* parent = nullptr);

    Tool currentTool() const { return m_currentTool; }
    void setTool(Tool tool, bool emitSignal = true);
    void setDefaultFillColor(const QColor& color);

signals:
    void toolChanged(Tool tool);
    void defaultFillChanged(const QColor& color);

private:
    QToolButton* createToolButton(Tool tool, const QString& iconName,
                                  const QString& label, const QString& shortcut);
    void updateButtonIcons();

    QButtonGroup* m_group = nullptr;
    QHash<Tool, QToolButton*> m_buttons;
    QWidget* m_colorSwatch = nullptr;
    Tool m_currentTool = Tool::Select;
    QColor m_defaultFill = QColor("#2D8CEB");
};

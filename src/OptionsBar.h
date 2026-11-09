#pragma once

#include "CanvasWidget.h"

#include <QWidget>
#include <functional>

class ColorButton;
class QCheckBox;
class QDoubleSpinBox;
class QFontComboBox;
class QLabel;
class QTimer;
class QToolButton;

class OptionsBar : public QWidget {
    Q_OBJECT
public:
    explicit OptionsBar(QWidget* parent = nullptr);

    void setContext(DesignDocument* document, CanvasWidget* canvas);
    void setTool(Tool tool);
    Tool tool() const { return m_tool; }
    void refresh();
    void flushEditing();

signals:
    void editBegan();
    void editEnded();
    void documentEdited();
    void previewRequested();
    void exportRequested();
    void alignRequested(const QString& mode);
    void defaultFillChanged(const QColor& color);

private:
    void buildUi();
    void updateToolPresentation();
    void updateGroups();
    void applyToPrimary(const std::function<void(Element&)>& mutate, bool immediate = false);
    void beginPendingEdit();
    void finishPendingEditNow();
    Element* primaryElement() const;
    void setGroupVisible(QWidget* group);
    void structureVisibilityForTool();

    DesignDocument* m_document = nullptr;
    CanvasWidget* m_canvas = nullptr;
    Tool m_tool = Tool::Select;

    QLabel* m_toolIcon = nullptr;
    QLabel* m_toolName = nullptr;
    QLabel* m_docInfo = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_hintLabel = nullptr;

    QWidget* m_selectGroup = nullptr;
    QWidget* m_shapeGroup = nullptr;
    QWidget* m_textGroup = nullptr;
    QWidget* m_pathGroup = nullptr;
    QWidget* m_zoomGroup = nullptr;
    QWidget* m_hintGroup = nullptr;

    ColorButton* m_fillButton = nullptr;
    ColorButton* m_strokeButton = nullptr;
    ColorButton* m_textColorButton = nullptr;
    QDoubleSpinBox* m_strokeWidthSpin = nullptr;
    QDoubleSpinBox* m_radiusSpin = nullptr;
    QDoubleSpinBox* m_sidesSpin = nullptr;
    QFontComboBox* m_fontCombo = nullptr;
    QDoubleSpinBox* m_fontSizeSpin = nullptr;
    QToolButton* m_boldButton = nullptr;
    QCheckBox* m_closedCheck = nullptr;

    QLabel* m_fillLabel = nullptr;
    QLabel* m_strokeLabel = nullptr;
    QLabel* m_strokeWidthLabel = nullptr;
    QLabel* m_radiusLabel = nullptr;
    QLabel* m_sidesLabel = nullptr;
    QLabel* m_fontLabel = nullptr;
    QLabel* m_fontSizeLabel = nullptr;

    QTimer* m_editTimer = nullptr;
    bool m_pendingEdit = false;
    bool m_updating = false;
};

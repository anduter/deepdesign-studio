#pragma once

#include "Model.h"

#include <QHash>
#include <QWidget>
#include <functional>

class CanvasWidget;
class ColorButton;
class ColorPaletteWidget;
class GradientEditor;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QToolButton;

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void setContext(DesignDocument* document, CanvasWidget* canvas);
    void refresh();
    void flushEditing();

signals:
    void editBegan();
    void editEnded();
    void documentEdited();

private:
    void buildUi();
    QWidget* createSection(const QString& title, QWidget* parent, QWidget*& body);
    void applyToPrimary(const std::function<void(Element&)>& mutate, bool immediate = false);
    void applyToDocument(const std::function<void(DesignDocument&)>& mutate, bool immediate = false);
    Element* primaryElement() const;
    void beginPendingEdit();
    void finishPendingEditNow();
    void rebuildInteractionList(const QString& elementId);
    void addInteraction();
    void removeInteraction();
    void updateRowVisibility(Element* element);
    void setSpinValue(QDoubleSpinBox* spin, double value);
    void updateTypeIcon(Element* element);
    void updateTextAlignment(Element* element);
    void setFillMode(bool gradient);
    void updateGradientUi(Element* element);
    void updateSelectedStopColor();
    GradientFill makeDefaultGradient(const Element& element) const;
    void applyGradient(const GradientFill& gradient, bool immediate);
    void applyPaletteColor(const QColor& color);

    DesignDocument* m_document = nullptr;
    CanvasWidget* m_canvas = nullptr;
    QTimer* m_editTimer = nullptr;
    bool m_pendingEdit = false;
    bool m_updating = false;
    bool m_gradientEditing = false;
    QString m_interactionSignature;

    QLabel* m_typeIcon = nullptr;
    QLabel* m_typeLabel = nullptr;
    QLabel* m_selectionInfo = nullptr;
    QLineEdit* m_nameEdit = nullptr;

    QWidget* m_elementContainer = nullptr;
    QWidget* m_documentContainer = nullptr;
    QLabel* m_emptyLabel = nullptr;

    QWidget* m_transformSection = nullptr;
    QWidget* m_appearanceSection = nullptr;
    QWidget* m_textSection = nullptr;
    QWidget* m_interactionSection = nullptr;

    QDoubleSpinBox* m_xSpin = nullptr;
    QDoubleSpinBox* m_ySpin = nullptr;
    QDoubleSpinBox* m_wSpin = nullptr;
    QDoubleSpinBox* m_hSpin = nullptr;
    QDoubleSpinBox* m_rotationSpin = nullptr;
    QDoubleSpinBox* m_opacitySpin = nullptr;
    QDoubleSpinBox* m_radiusSpin = nullptr;
    QDoubleSpinBox* m_strokeWidthSpin = nullptr;
    QDoubleSpinBox* m_sidesSpin = nullptr;
    QDoubleSpinBox* m_fontSizeSpin = nullptr;
    ColorButton* m_fillButton = nullptr;
    ColorButton* m_strokeButton = nullptr;
    ColorButton* m_textColorButton = nullptr;
    QWidget* m_fillRow = nullptr;
    QWidget* m_strokeRow = nullptr;
    QWidget* m_radiusRow = nullptr;
    QWidget* m_sidesRow = nullptr;
    QWidget* m_closedRow = nullptr;
    QCheckBox* m_closedCheck = nullptr;

    QWidget* m_fillModeRow = nullptr;
    QToolButton* m_solidModeButton = nullptr;
    QToolButton* m_gradientModeButton = nullptr;
    QWidget* m_gradientControls = nullptr;
    GradientEditor* m_gradientEditor = nullptr;
    QComboBox* m_gradientTypeCombo = nullptr;
    QDoubleSpinBox* m_gradientAngleSpin = nullptr;
    QLabel* m_gradientAngleLabel = nullptr;
    ColorButton* m_stopColorButton = nullptr;
    QToolButton* m_addStopButton = nullptr;
    QToolButton* m_removeStopButton = nullptr;
    QWidget* m_paletteRow = nullptr;
    ColorPaletteWidget* m_colorPalette = nullptr;

    QPlainTextEdit* m_textEdit = nullptr;
    QFontComboBox* m_fontCombo = nullptr;
    QToolButton* m_boldButton = nullptr;
    QHash<QString, QToolButton*> m_textAlignButtons;

    QLineEdit* m_docTitleEdit = nullptr;
    QDoubleSpinBox* m_canvasWidthSpin = nullptr;
    QDoubleSpinBox* m_canvasHeightSpin = nullptr;
    ColorButton* m_canvasBgButton = nullptr;

    QListWidget* m_interactionList = nullptr;
    QComboBox* m_triggerCombo = nullptr;
    QComboBox* m_actionCombo = nullptr;
    QComboBox* m_animationCombo = nullptr;
    QPushButton* m_addInteractionButton = nullptr;
    QPushButton* m_removeInteractionButton = nullptr;
    QLabel* m_interactionHint = nullptr;
};

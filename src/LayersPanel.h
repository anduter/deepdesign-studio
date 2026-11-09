#pragma once

#include "Model.h"

#include <QHash>
#include <QListWidget>
#include <QWidget>
#include <functional>

class CanvasWidget;
class QDoubleSpinBox;
class QLabel;
class QListWidgetItem;
class QTimer;
class QToolButton;

class LayerListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit LayerListWidget(QWidget* parent = nullptr);
    std::function<void(const QString& id, bool visible)> visibilityToggle;
    std::function<void(const QString& id, bool locked)> lockToggle;

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

class LayersPanel : public QWidget {
    Q_OBJECT
public:
    explicit LayersPanel(QWidget* parent = nullptr);

    void setContext(DesignDocument* document, CanvasWidget* canvas);
    void reload();
    void syncSelection(const QStringList& ids);

signals:
    void selectionRequested(const QStringList& ids);
    void editBegan();
    void editEnded();
    void documentEdited();

private:
    void buildUi();
    void updateOpacityFromSelection();
    void updateFooter();
    void emitLayerEdit(const std::function<void()>& mutate);
    void applyOrderFromList();
    void moveSelection(int direction);
    void duplicateSelection();
    void deleteSelection();
    void renameCurrentItem();
    QString signature() const;
    QListWidgetItem* itemForId(const QString& id) const;

    DesignDocument* m_document = nullptr;
    CanvasWidget* m_canvas = nullptr;
    QListWidget* m_list = nullptr;
    QLabel* m_footer = nullptr;
    QDoubleSpinBox* m_opacitySpin = nullptr;
    QTimer* m_opacityTimer = nullptr;
    QString m_lastSignature;
    bool m_syncing = false;
    bool m_opacityEditing = false;
    QStringList m_selection;
};

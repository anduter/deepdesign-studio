#pragma once

#include "Model.h"

#include <QHash>
#include <QJsonObject>
#include <QMainWindow>
#include <functional>

class QAction;
class QCloseEvent;
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QSplitter;
class QUndoStack;
class QWidget;
class CanvasWidget;
class LayersPanel;
class OptionsBar;
class PreviewDialog;
class PropertiesPanel;
class TimelinePanel;
class ToolPalette;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void openDocument(const QString& path);
    void applySnapshot(const QJsonObject& json);
    bool loadDocumentFile(const QString& path, bool addToRecent = true);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void buildUi();
    void createActions();
    void buildMenus();
    void setupConnections();
    void initializeDocument();
    QAction* addAction(const QString& key, const QString& text, const QKeySequence& shortcut,
                       const std::function<void()>& handler, bool canvasShortcut = false,
                       const QString& iconName = QString());
    QAction* action(const QString& key) const;

    void newDocument();
    void openDialog();
    bool saveDocument();
    bool saveDocumentAs();
    bool saveToPath(const QString& path);
    bool maybeSave();
    void exportAiPackage();
    void exportPng();
    void showPreview();
    void resizeCanvasDialog();
    void chooseCanvasBackground();
    void createGuideDialog(bool horizontal);
    void openHelpDocument(const QString& fileName);

    void beginEdit();
    void endEdit();
    void flushEdits();
    void onDocumentEdited();
    void onSelectionChanged(const QStringList& ids);
    void refreshAll();
    void updateTitle();
    void updateDocumentInfo();
    void updateActions();
    void updateZoomUi(double zoom);
    void applyZoomFromCombo();

    void copySelection();
    void cutSelection();
    void pasteSelection();
    void duplicateSelection();
    void deleteSelection();
    void invertSelection();
    void alignSelection(const QString& mode);
    void distributeSelection(bool horizontal);
    void reorderSelection(int mode);
    void selectExample(const QString& exampleFileName);

    ToolPalette* m_toolPalette = nullptr;
    OptionsBar* m_optionsBar = nullptr;
    CanvasWidget* m_canvas = nullptr;
    LayersPanel* m_layersPanel = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    TimelinePanel* m_timeline = nullptr;
    QSplitter* m_mainSplitter = nullptr;
    QSplitter* m_horizontalSplitter = nullptr;
    QSplitter* m_rightSplitter = nullptr;
    QLabel* m_documentTab = nullptr;
    QLabel* m_docInfoLabel = nullptr;
    QLabel* m_hintLabel = nullptr;
    QLabel* m_coordLabel = nullptr;
    QLabel* m_canvasSizeLabel = nullptr;
    QComboBox* m_zoomCombo = nullptr;

    QUndoStack* m_undo = nullptr;
    QJsonObject m_editBefore;
    DesignDocument m_document;
    QString m_filePath;
    QHash<QString, QAction*> m_actions;
    bool m_updatingZoom = false;
    bool m_initialized = false;
};

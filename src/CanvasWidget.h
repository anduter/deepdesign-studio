#pragma once

#include "Model.h"

#include <QHash>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QSet>
#include <QWidget>

class QTextEdit;

enum class Tool {
    Select,
    Rectangle,
    Ellipse,
    Line,
    Polygon,
    Star,
    Pen,
    Text,
    Hand,
    Zoom
};

class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget* parent = nullptr);

    void setDocument(DesignDocument* document);
    DesignDocument* document() const { return m_document; }

    QStringList selection() const { return m_selection; }
    void setSelection(const QStringList& ids, bool notify = true);
    void clearSelection();
    void selectAll();
    void pruneSelection();
    Element* primaryElement() const;

    Tool tool() const { return m_tool; }
    void setTool(Tool tool);

    double zoom() const { return m_zoom; }
    void setZoom(double zoom);
    void zoomIn();
    void zoomOut();
    void fitToView();
    void actualSize();

    bool gridVisible() const { return m_showGrid; }
    bool snapEnabled() const { return m_snapEnabled; }
    bool rulersVisible() const { return m_showRulers; }
    bool guidesVisible() const { return m_showGuides; }
    void setGridVisible(bool visible);
    void setSnapEnabled(bool enabled);
    void setRulersVisible(bool visible);
    void setGuidesVisible(bool visible);
    void setDefaultFillColor(const QColor& color);
    QColor defaultFillColor() const { return m_defaultFill; }
    QColor defaultStrokeColor() const { return m_defaultStroke; }
    double defaultStrokeWidth() const { return m_defaultStrokeWidth; }
    double defaultRadius() const { return m_defaultRadius; }
    int defaultSides() const { return m_defaultSides; }
    void setDefaultShapeStyle(const QColor& fill, const QColor& stroke,
                              double strokeWidth, double radius, int sides);

    void setPreviewAnimation(const QString& animationId, double time);
    void clearPreviewAnimation();
    void flushEditing();

    void deleteSelection();
    void nudgeSelection(const QPointF& delta);

signals:
    void selectionChanged(const QStringList& ids);
    void documentEdited();
    void editBegan();
    void editEnded();
    void zoomChanged(double zoom);
    void cursorMoved(const QPointF& documentPosition);
    void statusHint(const QString& hint);
    void toolRequested(Tool tool);
    void selectionInfoChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    enum class DragMode {
        None,
        Pan,
        Move,
        Resize,
        Rotate,
        Marquee,
        Create,
        Pen,
        Guide
    };

    struct SnapResult {
        QPointF offset;
        QVector<QLineF> guides;
    };

    // Geometry helpers
    QPointF docToScreen(const QPointF& point) const;
    QPointF screenToDoc(const QPointF& point) const;
    QRectF docToScreen(const QRectF& rect) const;
    QPolygonF docToScreen(const QPolygonF& polygon) const;
    QPointF contentTopLeft() const;
    QSizeF contentSize() const;
    QPointF handlePoint(const Element& element, int index) const;
    QPointF rotationHandlePoint(const Element& element) const;
    QPointF localPointForHandle(const Element& element, int index) const;
    int hitHandle(const QPointF& screenPoint) const;
    bool hitRotationHandle(const QPointF& screenPoint) const;
    Element* topElementAt(const QPointF& screenPoint, bool includeLocked = false) const;
    bool elementHit(const Element& element, const QPointF& documentPoint) const;
    QRectF selectionBounds() const;

    // Painting helpers
    void paintWorkspace(QPainter& painter);
    void paintArtboard(QPainter& painter);
    void paintSelection(QPainter& painter);
    void paintRulers(QPainter& painter);
    void paintEmptyState(QPainter& painter);
    void paintPenPreview(QPainter& painter);
    void paintInteractionBadge(QPainter& painter);
    void paintGuides(QPainter& painter);
    void paintBadge(QPainter& painter, const QPointF& screenPoint, const QString& text);

    // Actions
    void beginMove();
    void beginResize(int handle);
    void beginRotate();
    void beginCreate(const QPointF& documentPoint);
    void beginPan(const QPointF& screenPoint);
    void beginMarquee();
    void finishDrag(bool cancel = false);
    void updateCreate(const QPointF& documentPoint, bool shift);
    void updateResize(const QPointF& documentPoint, bool shift);
    void updateRotate(const QPointF& documentPoint, bool shift);
    void updateMove(const QPointF& documentPoint, bool shift);
    void updateMarquee();
    void finishPen(bool cancel = false);
    void addPenPoint(const QPointF& documentPoint);
    void cancelPen();

    int guideAt(const QPointF& screenPoint) const;
    void beginGuideCreation(bool horizontal, const QPointF& documentPoint);
    void beginGuideDrag(int index);
    void updateGuide(const QPointF& documentPoint);
    void finishGuide(bool cancel = false);

    SnapResult snapRect(QRectF rect, const QSet<QString>& ignore) const;
    void updateHover(const QPointF& screenPoint);
    void updateCursor();
    void applyZoom(double zoom, const QPointF& anchorScreen);
    void updateEditorGeometry();
    void commitTextEditing(bool cancel = false);
    void beginTextEditing(const QString& id, bool isNew);
    bool eventFilter(QObject* watched, QEvent* event) override;

    Element makeNewElement(ElementType type, const QRectF& frame) const;
    QString addElement(Element element);
    void notifyEdited();

    DesignDocument* m_document = nullptr;

    double m_zoom = 1.0;
    QPointF m_origin = QPointF(80, 80);
    bool m_pendingFit = true;
    QSize m_lastSize;
    bool m_showGrid = true;
    bool m_snapEnabled = true;
    bool m_showRulers = true;
    bool m_showGuides = true;
    int m_rulerSize = 22;
    QColor m_defaultFillColor = QColor("#2D8CEB");
    QColor m_defaultFill = QColor("#2D8CEB");
    QColor m_defaultStroke = QColor("#5AA9F0");
    double m_defaultStrokeWidth = 1.0;
    double m_defaultRadius = 14.0;
    int m_defaultSides = 6;

    Tool m_tool = Tool::Select;
    QStringList m_selection;
    QString m_hoverId;
    bool m_spaceDown = false;

    DragMode m_dragMode = DragMode::None;
    QPointF m_dragStartScreen;
    QPointF m_dragStartDoc;
    QPointF m_panStartOrigin;
    QHash<QString, QRectF> m_startFrames;
    QHash<QString, double> m_startRotations;
    QString m_activeId;
    int m_activeHandle = -1;
    QRectF m_marqueeRect = QRectF();
    QStringList m_marqueeBaseSelection;
    QRectF m_createStartFrame;
    QPointF m_createStartDoc;
    QPointF m_createCurrentDoc;
    bool m_creatingDefault = false;
    QString m_creationId;
    QString m_sizeBadge;
    QPointF m_badgeScreenPoint;
    bool m_badgeVisible = false;
    QVector<QLineF> m_guides;

    int m_activeGuide = -1;
    bool m_guideIsNew = false;
    double m_guideStartPosition = 0.0;

    QVector<QPointF> m_penPoints;
    QPointF m_penHover;
    bool m_penNearStart = false;

    QTextEdit* m_textEditor = nullptr;
    QString m_textEditId;
    QString m_textEditOriginal;
    bool m_textEditIsNew = false;
    bool m_textEditCanceled = false;
    bool m_commitInProgress = false;

    QString m_previewAnimationId;
    double m_previewTime = 0.0;

    QPointF m_lastMouse;
    bool m_hasLastMouse = false;
};

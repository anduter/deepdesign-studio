#include "LayersPanel.h"
#include "CanvasWidget.h"
#include "Icons.h"
#include "Theme.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QSet>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>

namespace {
enum LayerRole {
    IdRole = Qt::UserRole + 1,
    TypeRole,
    VisibleRole,
    LockedRole,
    NameRole,
    InteractionRole
};

QString typeIconName(ElementType type)
{
    switch (type) {
    case ElementType::Rectangle: return QStringLiteral("rect");
    case ElementType::Ellipse: return QStringLiteral("ellipse");
    case ElementType::Line: return QStringLiteral("line");
    case ElementType::Polygon: return QStringLiteral("polygon");
    case ElementType::Star: return QStringLiteral("star");
    case ElementType::Path: return QStringLiteral("pen");
    case ElementType::Text: return QStringLiteral("text");
    }
    return QStringLiteral("rect");
}

class LayerDelegate : public QStyledItemDelegate {
public:
    explicit LayerDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return QSize(180, 36);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QPainter& p = *painter;
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRect r = option.rect;
        const bool selected = option.state & QStyle::State_Selected;
        const bool hover = option.state & QStyle::State_MouseOver;
        const bool visible = index.data(VisibleRole).toBool();
        const bool locked = index.data(LockedRole).toBool();
        const bool hasInteraction = index.data(InteractionRole).toBool();
        const auto type = static_cast<ElementType>(index.data(TypeRole).toInt());
        const QString name = index.data(NameRole).toString();

        p.fillRect(r, selected ? QColor("#3E3E3E")
                                : (hover ? QColor("#343434") : QColor("#2B2B2B")));
        if (selected) {
            p.fillRect(QRect(r.left(), r.top(), 2, r.height()), QColor("#2D8CEB"));
        }
        p.setPen(QPen(QColor("#383838"), 1));
        p.drawLine(r.bottomLeft(), r.bottomRight());

        const QRect iconRect(r.left() + 10, r.top() + 9, 18, 18);
        const QColor iconColor = visible ? (selected ? QColor("#FFFFFF") : QColor("#B4B4B4"))
                                         : QColor("#6A6A6A");
        Icons::draw(p, typeIconName(type), iconRect, iconColor, 1.5);

        if (hasInteraction) {
            QPainterPath triangle;
            const QPointF c(r.right() - 80, r.center().y());
            triangle.moveTo(c + QPointF(-3.2, -3.5));
            triangle.lineTo(c + QPointF(3.2, 0));
            triangle.lineTo(c + QPointF(-3.2, 3.5));
            triangle.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#2D8CEB"));
            p.drawPath(triangle);
        }

        QFont font = option.font;
        font.setPointSize(9);
        p.setFont(font);
        p.setPen(visible ? (selected ? QColor("#F2F2F2") : QColor("#D0D0D0"))
                         : QColor("#777777"));
        const QRect nameRect(r.left() + 36, r.top(), r.width() - 36 - 92, r.height());
        const QString elided = p.fontMetrics().elidedText(name, Qt::ElideRight, nameRect.width());
        p.drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

        const QRect eyeRect(r.right() - 62, r.top() + 7, 24, 22);
        Icons::draw(p, visible ? QStringLiteral("eye") : QStringLiteral("eyeOff"), eyeRect,
                    visible ? QColor("#BEBEBE") : QColor("#6E6E6E"), 1.4);
        const QRect lockRect(r.right() - 34, r.top() + 7, 24, 22);
        Icons::draw(p, locked ? QStringLiteral("lock") : QStringLiteral("unlock"), lockRect,
                    locked ? QColor("#D6A35C") : QColor("#777777"), 1.4);
        p.restore();
    }
};
} // namespace

LayerListWidget::LayerListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setItemDelegate(new LayerDelegate(this));
    setMouseTracking(true);
    setUniformItemSizes(true);
    setSpacing(0);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet(QStringLiteral("QListWidget { background: #2B2B2B; border: none; }"));
}

void LayerListWidget::mousePressEvent(QMouseEvent* event)
{
    const QModelIndex index = indexAt(event->position().toPoint());
    if (index.isValid()) {
        const QRect row = visualRect(index);
        const int x = qRound(event->position().x());
        if (x >= viewport()->width() - 62 && x <= viewport()->width() - 38) {
            if (visibilityToggle) {
                visibilityToggle(index.data(IdRole).toString(),
                                 !index.data(VisibleRole).toBool());
            }
            return;
        }
        if (x >= viewport()->width() - 36) {
            if (lockToggle) {
                lockToggle(index.data(IdRole).toString(), !index.data(LockedRole).toBool());
            }
            return;
        }
        Q_UNUSED(row);
    }
    QListWidget::mousePressEvent(event);
}

LayersPanel::LayersPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();

    m_opacityTimer = new QTimer(this);
    m_opacityTimer->setSingleShot(true);
    connect(m_opacityTimer, &QTimer::timeout, this, [this]() {
        if (!m_opacityEditing) return;
        m_opacityEditing = false;
        emit editEnded();
    });
}

void LayersPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QFrame(this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    header->setFixedHeight(30);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(7, 0, 5, 0);
    headerLayout->setSpacing(1);

    auto* collapse = new QToolButton(header);
    collapse->setIcon(Icons::icon(QStringLiteral("chevronDown"), QColor("#BEBEBE"), 14));
    collapse->setFixedSize(20, 20);
    collapse->setToolTip(QStringLiteral("折叠 / 展开面板"));
    headerLayout->addWidget(collapse);

    auto* title = new QLabel(QStringLiteral("图层"), header);
    title->setObjectName(QStringLiteral("PanelTitle"));
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);

    auto makeHeaderButton = [&](const QString& icon, const QString& tooltip) {
        auto* button = new QToolButton(header);
        button->setIcon(Icons::icon(icon, QColor("#C8C8C8"), 15));
        button->setFixedSize(22, 22);
        button->setToolTip(tooltip);
        headerLayout->addWidget(button);
        return button;
    };
    auto* duplicateButton = makeHeaderButton(QStringLiteral("duplicate"), QStringLiteral("复制图层"));
    auto* upButton = makeHeaderButton(QStringLiteral("chevronUp"), QStringLiteral("上移一层"));
    auto* downButton = makeHeaderButton(QStringLiteral("chevronDown"), QStringLiteral("下移一层"));
    auto* deleteButton = makeHeaderButton(QStringLiteral("trash"), QStringLiteral("删除图层"));
    root->addWidget(header);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto* opacityRow = new QWidget(body);
    opacityRow->setFixedHeight(31);
    opacityRow->setStyleSheet(QStringLiteral("background: #333333;"));
    auto* opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(9, 4, 9, 4);
    opacityLayout->setSpacing(7);
    auto* opacityLabel = new QLabel(QStringLiteral("不透明度"), opacityRow);
    opacityLabel->setObjectName(QStringLiteral("FieldLabel"));
    opacityLayout->addWidget(opacityLabel);
    m_opacitySpin = new QDoubleSpinBox(opacityRow);
    m_opacitySpin->setRange(0, 100);
    m_opacitySpin->setDecimals(0);
    m_opacitySpin->setSuffix(QStringLiteral("%"));
    m_opacitySpin->setFixedWidth(64);
    opacityLayout->addWidget(m_opacitySpin);
    opacityLayout->addStretch(1);
    auto* blendLabel = new QLabel(QStringLiteral("正常"), opacityRow);
    blendLabel->setStyleSheet(QStringLiteral("color: #8A8A8A;"));
    opacityLayout->addWidget(blendLabel);
    bodyLayout->addWidget(opacityRow);

    m_list = new LayerListWidget(body);
    bodyLayout->addWidget(m_list, 1);

    auto* footer = new QWidget(body);
    footer->setFixedHeight(24);
    footer->setStyleSheet(QStringLiteral("background: #303030; border-top: 1px solid #1A1A1A;"));
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(9, 0, 9, 0);
    m_footer = new QLabel(QStringLiteral("0 个图层"), footer);
    m_footer->setStyleSheet(QStringLiteral("color: #8A8A8A; font-size: 11px;"));
    footerLayout->addWidget(m_footer);
    footerLayout->addStretch(1);
    bodyLayout->addWidget(footer);

    root->addWidget(body, 1);

    connect(collapse, &QToolButton::clicked, this, [=]() {
        body->setVisible(!body->isVisible());
        collapse->setIcon(Icons::icon(body->isVisible() ? QStringLiteral("chevronDown")
                                                        : QStringLiteral("chevronRight"),
                                      QColor("#BEBEBE"), 14));
    });
    connect(duplicateButton, &QToolButton::clicked, this, &LayersPanel::duplicateSelection);
    connect(upButton, &QToolButton::clicked, this, [this]() { moveSelection(1); });
    connect(downButton, &QToolButton::clicked, this, [this]() { moveSelection(-1); });
    connect(deleteButton, &QToolButton::clicked, this, &LayersPanel::deleteSelection);

    connect(m_list, &QListWidget::itemSelectionChanged, this, [this]() {
        if (m_syncing) return;
        QStringList ids;
        for (QListWidgetItem* item : m_list->selectedItems()) {
            ids.append(item->data(IdRole).toString());
        }
        m_selection = ids;
        updateOpacityFromSelection();
        emit selectionRequested(ids);
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        renameCurrentItem();
    });

    auto* layerList = static_cast<LayerListWidget*>(m_list);
    layerList->visibilityToggle = [this](const QString& id, bool visible) {
        emitLayerEdit([this, id, visible]() {
            if (Element* element = m_document->findElement(id)) element->visible = visible;
        });
    };
    layerList->lockToggle = [this](const QString& id, bool locked) {
        emitLayerEdit([this, id, locked]() {
            if (Element* element = m_document->findElement(id)) element->locked = locked;
        });
    };
    connect(m_list->model(), &QAbstractItemModel::rowsMoved, this, [this]() {
        QTimer::singleShot(0, this, &LayersPanel::applyOrderFromList);
    });

    connect(m_opacitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_syncing || !m_document) return;
                if (m_selection.isEmpty()) return;
                if (!m_opacityEditing) {
                    m_opacityEditing = true;
                    emit editBegan();
                }
                m_opacityTimer->start(450);
                for (const auto& id : m_selection) {
                    if (Element* element = m_document->findElement(id)) {
                        element->opacity = qBound(0.0, value / 100.0, 1.0);
                    }
                }
                emit documentEdited();
            });
}

void LayersPanel::setContext(DesignDocument* document, CanvasWidget* canvas)
{
    m_document = document;
    m_canvas = canvas;
    m_lastSignature.clear();
    reload();
    if (m_canvas) syncSelection(m_canvas->selection());
}

QString LayersPanel::signature() const
{
    if (!m_document) return {};
    QSet<QString> sources;
    for (const auto& interaction : m_document->interactions) sources.insert(interaction.sourceId);
    QString sig;
    for (const auto& element : m_document->elements) {
        sig += QStringLiteral("%1|%2|%3|%4|%5|%6;")
                   .arg(element.id, element.name)
                   .arg(static_cast<int>(element.type))
                   .arg(element.visible ? 1 : 0)
                   .arg(element.locked ? 1 : 0)
                   .arg(sources.contains(element.id) ? 1 : 0);
    }
    return sig;
}

void LayersPanel::reload()
{
    if (!m_document) return;
    const QString sig = signature();
    if (sig == m_lastSignature) {
        updateFooter();
        updateOpacityFromSelection();
        return;
    }
    m_lastSignature = sig;

    const int scroll = m_list->verticalScrollBar()->value();
    QSignalBlocker blocker(m_list);
    m_syncing = true;
    m_list->clear();
    QSet<QString> sources;
    for (const auto& interaction : m_document->interactions) sources.insert(interaction.sourceId);

    for (int i = m_document->elements.size() - 1; i >= 0; --i) {
        const Element& element = m_document->elements[i];
        auto* item = new QListWidgetItem(m_list);
        item->setData(IdRole, element.id);
        item->setData(TypeRole, static_cast<int>(element.type));
        item->setData(VisibleRole, element.visible);
        item->setData(LockedRole, element.locked);
        item->setData(NameRole, element.name);
        item->setData(InteractionRole, sources.contains(element.id));
        item->setSizeHint(QSize(100, 36));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
                       Qt::ItemIsDropEnabled);
    }
    m_syncing = false;
    blocker.unblock();

    m_list->verticalScrollBar()->setValue(scroll);
    syncSelection(m_canvas ? m_canvas->selection() : m_selection);
    updateFooter();
}

void LayersPanel::syncSelection(const QStringList& ids)
{
    if (!m_list) return;
    m_selection = ids;
    m_syncing = true;
    QSignalBlocker blocker(m_list);
    m_list->clearSelection();
    QListWidgetItem* first = nullptr;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (ids.contains(item->data(IdRole).toString())) {
            item->setSelected(true);
            if (!first) first = item;
        }
    }
    if (first) {
        m_list->setCurrentItem(first, QItemSelectionModel::NoUpdate);
        m_list->scrollToItem(first, QAbstractItemView::EnsureVisible);
    }
    m_syncing = false;
    blocker.unblock();
    updateOpacityFromSelection();
    updateFooter();
}

void LayersPanel::updateOpacityFromSelection()
{
    if (!m_document || !m_opacitySpin) return;
    const bool has = !m_selection.isEmpty();
    m_opacitySpin->setEnabled(has);
    if (!has) return;
    const Element* element = m_document->findElement(m_selection.first());
    if (!element) return;
    QSignalBlocker blocker(m_opacitySpin);
    m_opacitySpin->setValue(element->opacity * 100.0);
}

void LayersPanel::updateFooter()
{
    if (!m_footer || !m_document) return;
    const int count = m_document->elements.size();
    if (m_selection.isEmpty()) m_footer->setText(QStringLiteral("%1 个图层").arg(count));
    else m_footer->setText(QStringLiteral("%1 个图层 · 已选 %2").arg(count).arg(m_selection.size()));
}

void LayersPanel::emitLayerEdit(const std::function<void()>& mutate)
{
    if (!m_document) return;
    emit editBegan();
    mutate();
    emit documentEdited();
    emit editEnded();
    reload();
    if (m_canvas) syncSelection(m_canvas->selection());
}

void LayersPanel::moveSelection(int direction)
{
    if (!m_document || m_selection.isEmpty()) return;
    emit editBegan();
    if (direction > 0) m_document->bringForward(m_selection);
    else m_document->sendBackward(m_selection);
    emit documentEdited();
    emit editEnded();
    m_lastSignature.clear();
    reload();
    if (m_canvas) syncSelection(m_canvas->selection());
}

void LayersPanel::duplicateSelection()
{
    if (!m_document || m_selection.isEmpty()) return;
    emit editBegan();
    QStringList newIds;
    for (const auto& id : m_selection) {
        const QString newId = m_document->duplicateElement(id);
        if (!newId.isEmpty()) newIds.append(newId);
    }
    emit documentEdited();
    emit editEnded();
    m_lastSignature.clear();
    reload();
    if (m_canvas && !newIds.isEmpty()) {
        m_canvas->setSelection(newIds);
        syncSelection(newIds);
    }
}

void LayersPanel::deleteSelection()
{
    if (!m_document || m_selection.isEmpty()) return;
    emit editBegan();
    m_document->removeElements(m_selection);
    m_selection.clear();
    emit documentEdited();
    emit editEnded();
    m_lastSignature.clear();
    if (m_canvas) m_canvas->clearSelection();
    reload();
}

void LayersPanel::renameCurrentItem()
{
    if (!m_document || !m_list->currentItem()) return;
    const QString id = m_list->currentItem()->data(IdRole).toString();
    Element* element = m_document->findElement(id);
    if (!element) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("重命名图层"),
                                               QStringLiteral("图层名称："), QLineEdit::Normal,
                                               element->name, &ok);
    if (!ok || name.trimmed().isEmpty() || name == element->name) return;
    emit editBegan();
    element->name = name.trimmed();
    emit documentEdited();
    emit editEnded();
    m_lastSignature.clear();
    reload();
    if (m_canvas) syncSelection(m_canvas->selection());
}

void LayersPanel::applyOrderFromList()
{
    if (!m_document || !m_list) return;
    QStringList idsTopToBottom;
    for (int i = 0; i < m_list->count(); ++i) {
        idsTopToBottom.append(m_list->item(i)->data(IdRole).toString());
    }
    QStringList idsBottomToTop = idsTopToBottom;
    std::reverse(idsBottomToTop.begin(), idsBottomToTop.end());
    if (idsBottomToTop.size() != m_document->elements.size()) {
        reload();
        return;
    }
    bool same = true;
    for (int i = 0; i < idsBottomToTop.size(); ++i) {
        if (m_document->elements[i].id != idsBottomToTop[i]) {
            same = false;
            break;
        }
    }
    if (same) return;

    QHash<QString, Element> byId;
    for (const auto& element : m_document->elements) byId.insert(element.id, element);
    QVector<Element> reordered;
    for (const auto& id : idsBottomToTop) {
        if (!byId.contains(id)) {
            reload();
            return;
        }
        reordered.append(byId.value(id));
    }
    emit editBegan();
    m_document->elements = reordered;
    emit documentEdited();
    emit editEnded();
    m_lastSignature.clear();
    reload();
}

QListWidgetItem* LayersPanel::itemForId(const QString& id) const
{
    if (!m_list) return nullptr;
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(IdRole).toString() == id) return m_list->item(i);
    }
    return nullptr;
}

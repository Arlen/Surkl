/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "NodeItem.hpp"
#include "FileSystemScene.hpp"
#include "SessionManager.hpp"
#include "theme.hpp"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QVariantAnimation>

#include <numbers>
#include <ranges>
#include <stack>
#include <unordered_set>


using namespace core;
using namespace std;

namespace
{
    constexpr qreal GOLDEN = 1.0 / std::numbers::phi;
    /// These don't change, because the scene view offers zoom feature.
    constexpr qreal NODE_OPEN_RADIUS           = 32.0;
    constexpr qreal NODE_OPEN_DIAMETER         = NODE_OPEN_RADIUS * 2.0;
    //constexpr qreal NODE_CLOSED_RADIUS       = NODE_OPEN_RADIUS * GOLDEN;
    constexpr qreal NODE_CLOSED_DIAMETER       = NODE_OPEN_DIAMETER * GOLDEN;
    constexpr qreal NODE_HALF_CLOSED_DIAMETER  = NODE_OPEN_DIAMETER * (1.0 - GOLDEN*GOLDEN*GOLDEN);
    constexpr qreal EDGE_WIDTH                 = 4.0;
    constexpr qreal NODE_OPEN_PEN_WIDTH        = 4.0;
    constexpr qreal NODE_CLOSED_PEN_WIDTH      = EDGE_WIDTH * GOLDEN;
    constexpr qreal NODE_HALF_CLOSED_PEN_WIDTH = NODE_OPEN_PEN_WIDTH * (1.0 - GOLDEN*GOLDEN*GOLDEN);

    /// fixed for now.
    constexpr int NODE_CHILD_COUNT = 16;


    std::deque<QLineF> circle(int sides, qreal startAngle = 0.0)
    {
        sides = std::max(1, sides);

        const auto anglePerSide = 360.0 / sides;
        auto angle = startAngle - anglePerSide * 0.5;
        auto line1 = QLineF(QPointF(0, 0), QPointF(1, 0));
        auto line2 = line1;

        std::deque<QLineF> result;
        for (int i = 0; i < sides; ++i, angle += anglePerSide) {
            line1.setAngle(angle);
            line2.setAngle(angle + anglePerSide);
            result.emplace_back(line2.p2(), line1.p2());
        }

        return result;
    }

    bool isRoot(const QGraphicsItem* node)
    {
        return qgraphicsitem_cast<const RootItem*>(node) != nullptr;
    }

    /// TODO rename
    std::vector<std::pair<QGraphicsItem*, QPointF>> getAncestorPos(const NodeItem* node)
    {
        Q_ASSERT(node != nullptr);
        Q_ASSERT(!isRoot(node));

        std::vector<std::pair<QGraphicsItem*, QPointF>> result;
        auto* parent = node->parentEdge()->source();

        /// (0,0) is always the center of the (boundingRect) node.
        while (!isRoot(parent)) {
            result.emplace_back(parent, parent->mapToScene(QPointF(0, 0)));
            parent = asNodeItem(parent)->parentEdge()->source();
        }
        result.emplace_back(parent, parent->mapToScene(QPointF(0, 0)));
        return result;
    }

    std::deque<QPersistentModelIndex> gatherIndices(const QPersistentModelIndex& start, int count,
        const std::unordered_set<int>& excludedRows)
    {
        Q_ASSERT(start.isValid());
        Q_ASSERT(count > 0);
        Q_ASSERT(!excludedRows.contains(start.row()));

        auto isExcluded = [&excludedRows](const QPersistentModelIndex& index)
        {
            return excludedRows.contains(index.row());
        };

        deque result {start};

        auto next = start;
        while (result.size() < count) {
            next = next.sibling(next.row() + 1, 0);
            if (!next.isValid()) {
                /// reached the end
                break;
            }
            if (isExcluded(next)) {
                continue;
            }
            result.push_back(next);
        }
        next = start;
        while (result.size() < count) {
            next = next.sibling(next.row() - 1, 0);
            if (!next.isValid()) {
                /// reach the end
                break;
            }
            if (isExcluded(next)) {
                continue;
            }
            result.push_front(next);
        }

        Q_ASSERT(ranges::all_of(result, &QPersistentModelIndex::isValid));
        Q_ASSERT(ranges::none_of(result, isExcluded));

        return result;
    }

    QPainterPath closedNodeShape(const NodeItem* node, const QRectF& rec)
    {
        const auto center = rec.center();
        const auto angle  = node->parentEdge()->line().angle() + 180;

        auto guide = QLineF(center, center + QPointF(rec.width()*0.5, 0));
        guide.setAngle(angle);
        auto path = QPainterPath();

        /// in CCW order
        guide.setAngle(guide.angle() + 25);
        path.moveTo(guide.p2());
        guide.setAngle(guide.angle() + 110);
        path.lineTo(guide.p2());
        guide.setAngle(guide.angle() + 90);
        path.lineTo(guide.p2());
        guide.setAngle(guide.angle() + 110);
        path.lineTo(guide.p2());

        return path;
    }

    QPainterPath fileNodeShape(const NodeItem* node, const QRectF& rec)
    {
        const auto center = rec.center();
        const auto angle  = node->parentEdge()->line().angle() + 180;

        auto guide = QLineF(center, center + QPointF(rec.width()*0.5, 0));
        guide.setAngle(angle);
        auto path = QPainterPath();

        /// in CCW order
        path.moveTo(guide.p2());
        guide.setAngle(guide.angle() + 45);
        path.lineTo(guide.p2());
        guide.setAngle(guide.angle() + 135);
        path.lineTo(guide.p2());
        guide.setAngle(guide.angle() + 135);
        path.lineTo(guide.p2());
        guide.setAngle(guide.angle() + 45);
        path.lineTo(guide.p2());

        return path;
    }

    void paintClosedFolder(QPainter* p, const NodeItem* node)
    {
        Q_ASSERT(node->isClosed());
        Q_ASSERT(node->shape().elementCount() == 4);

        const auto* tm = SessionManager::tm();

        const auto rec    = node->boundingRect();
        const auto center = rec.center();
        const auto shape  = node->shape();
        const auto top    = QLineF(shape.elementAt(1), shape.elementAt(2));
        const auto bot    = QLineF(shape.elementAt(0), shape.elementAt(3));
        const auto spine  = QLineF(bot.pointAt(0.5), top.pointAt(0.5));
        const auto tri    = QPolygonF() << shape.elementAt(1)
                                        << center
                                        << shape.elementAt(2);

        const auto color = node->isSelected()
            ? tm->closedNodeColor().lighter()
            : tm->closedNodeColor();

        p->setBrush(tm->closedNodeBorderColor());
        p->setPen(Qt::NoPen);
        p->drawPath(shape);

        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(color.darker(), 2));
        p->drawLine(QLineF(spine.pointAt(0.1), spine.pointAt(0.5)));

        p->setBrush(tm->closedNodeBorderColor().lighter(400));
        //p->setPen(Qt::NoPen);
        const auto rec2 = QRectF(-6, -6, 12, 12);
        p->drawEllipse(rec2);

        p->setPen(Qt::NoPen);
        p->setBrush(color);
        p->drawPolygon(tri);
    }

    void paintFile(QPainter* p, const NodeItem* node)
    {
        const auto* tm = SessionManager::tm();

        p->setBrush(tm->closedNodeBorderColor());
        p->setPen(Qt::NoPen);
        p->drawPath(node->shape());
    }
}





////////////////
/// RootItem ///
////////////////
RootItem::RootItem(QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
{
    const auto* tm = SessionManager::tm();

    setRect(QRectF(-12, -12, 24, 24));
    setPen(Qt::NoPen);
    setBrush(tm->edgeColor());
    setFlags(ItemIsSelectable | ItemIsMovable |  ItemSendsScenePositionChanges);
}

void RootItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget);

    const auto rec = boundingRect();

    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(brush());
    p->drawEllipse(rec);

    auto bg = brush().color();
    int r = 0, g = 0, b = 0;
    bg.getRgb(&r, &g, &b);
    bg.setRgb(255 - r, 255 - g, 255 - b);

    p->setBrush(bg);
    p->drawEllipse(rec.center(), rec.width() * 0.25, rec.height() * 0.25);
}

QVariant RootItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case ItemScenePositionHasChanged:
        if (auto* pe = qgraphicsitem_cast<EdgeItem*>(parentItem())) {
            pe->adjust();
        }
        break;

    default:
        break;
    };

    return QGraphicsEllipseItem::itemChange(change, value);
}


////////////////
/// KnotItem ///
////////////////
KnotItem::KnotItem(QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
{
    setRect(QRectF(-2, -2, 4, 4));
}

void KnotItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget);

    const auto* tm = SessionManager::tm();

    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(tm->openNodeColor());
    p->drawEllipse(boundingRect());
}


////////////
/// Node ///
////////////
NodeItem::NodeItem(const QPersistentModelIndex& index)
{
    setIndex(index);
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsScenePositionChanges);

    _knot = new KnotItem(this);
    _knot->setPos(QPointF(NODE_OPEN_RADIUS, 0));
    _knot->hide();
}

NodeItem* NodeItem::createRootItem(FileSystemScene* scene)
{
    Q_ASSERT(scene);

    auto* root = new RootItem();
    scene->addItem(root);

    auto* node = createNode(scene, root);
    node->setIndex(scene->rootIndex());
    node->parentEdge()->setText(node->name());
    root->setParentItem(node->parentEdge());
    root->setPos(-128, 0);

    return node;
}

NodeItem* NodeItem::createNode(QGraphicsScene* scene, QGraphicsItem* parent)
{
    Q_ASSERT(scene);
    Q_ASSERT(parent);
    Q_ASSERT(scene == parent->scene());

    auto* node = new NodeItem(QModelIndex());
    auto* edge = new EdgeItem(parent, node);

    scene->addItem(node);
    scene->addItem(edge);

    node->_parentEdge = edge;

    return node;
}

NodeItem::~NodeItem()
{

}

void NodeItem::init()
{
    Q_ASSERT(scene());
    Q_ASSERT(_childEdges.empty());
    Q_ASSERT(_index.isValid());

    const auto count = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    for (auto _ : std::views::iota(0, count)) {
        _childEdges.emplace_back(createNode(scene(), this)->parentEdge());
    }
}

void NodeItem::reload(int start, int end)
{
    if (_state == FolderState::Closed) {
       return;
    }

    const auto rowCount = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    /// room to grow
    if (_childEdges.size() < rowCount) {
        const auto sizeBefore = _childEdges.size();
        NodeVector nodes; nodes.reserve(rowCount - _childEdges.size());
        while (_childEdges.size() < rowCount) {
            nodes.push_back(createNode(scene(), this));
            _childEdges.push_back(nodes.back()->parentEdge());
        }

        const auto sizeAfter = _childEdges.size();
        if (sizeBefore != sizeAfter) {
            if (_state == FolderState::Open) {
                spread();
            }
            else if (_state == FolderState::HalfClosed) {
                for (auto* n : nodes) {
                    n->parentEdge()->setState(EdgeItem::CollapsedState);
                }
                /// open(), then skiptTo(start), and then halfClose() to
                /// update the index of each newly created node in nodes.
                /// Don't need the all the details of those functions mentioned,
                /// so those parts are commented out to show what is used.
                ///
                //spread();
                setAllEdgeState(this, EdgeItem::ActiveState);
                //adjustAllEdges(this);

                //animator->disable();
                skipTo(start);
                //animator->enable();

                setAllEdgeState(this, EdgeItem::CollapsedState);
                //setState(FolderState::HalfClosed);
                spread();
                adjustAllEdges(this);
            }
        }
    }

    if (_state == FolderState::Open) {
        skipTo(start);
    }
}

void NodeItem::unload(int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);

    auto all = _childEdges | asTargetNodes;

    /// 1. close all invalid nodes that are not closed.
    for (auto* node : all) {
        if (!node->index().isValid() && !node->isClosed()) {
            /// This is close() without the internalRotationAfterClose();
            node->destroyChildren();
            node->setState(FolderState::Closed);
            shrink(node);
        }
    }

    const auto ghostNodes = static_cast<int>(_childEdges.size()) -
                            _index.model()->rowCount(_index);

    /// 2. destroy excessive nodes.
    if (EdgeDeque edges; ghostNodes > 0) {
        for (int deletedNodes = 0; auto* node : all) {
            if (deletedNodes >= ghostNodes) {
                edges.push_back(node->parentEdge());
                continue;
            }
            if (!node->index().isValid()) {
                deletedNodes++;
                Q_ASSERT(node->isClosed());

                scene()->removeItem(node);
                scene()->removeItem(node->parentEdge());
                delete node->parentEdge();
                delete node;
            } else {
                edges.push_back(node->parentEdge());
            }
        }
        _childEdges.swap(edges);
        if (_childEdges.empty()) {
            return close();
        }
        spread();
    }

    const auto openOrHalfClosedRows = _childEdges
        | asNotClosedTargetNodes
        | asIndexRow
        | ranges::to<std::unordered_set>()
        ;

    auto availableNodes = _childEdges | asFilesOrClosedTargetNodes;
    auto availableIndices = availableNodes | asIndex;
    if (ranges::all_of(availableIndices, &QPersistentModelIndex::isValid)) {
        return;
    }

    if (const auto count = ranges::distance(availableIndices); count > 0) {
        auto startIndex = _index.model()->sibling(0, 0, _index);
        if (auto betterStart = ranges::find_if(availableIndices, &QPersistentModelIndex::isValid);
            betterStart != availableIndices.end()) {
            startIndex = *betterStart;
        }
        auto rebuilt = gatherIndices(startIndex, count, openOrHalfClosedRows);

        for (auto* node : availableNodes) {
            if (rebuilt.empty()) { break; }
            /// set index even if node has valid index, b/c the order might
            /// have changed.
            node->setIndex(rebuilt.front());
            node->parentEdge()->setText(node->name());
            rebuilt.pop_front();
        }
    }
}

void NodeItem::setIndex(const QPersistentModelIndex& index)
{
    _index = index;
}

QString NodeItem::name() const
{
    Q_ASSERT(_index.isValid());

    const auto datum = _index.data();
    Q_ASSERT(datum.isValid());

    return datum.toString();
}

QRectF NodeItem::boundingRect() const
{
    qreal side = 1.0;

    if (!fsScene()->isDir(_index)) {
        side = NODE_CLOSED_DIAMETER + NODE_CLOSED_PEN_WIDTH;
    } else {
        switch (_state) {
        case FolderState::Open:
            side = NODE_OPEN_DIAMETER + NODE_OPEN_PEN_WIDTH;
            break;
        case FolderState::Closed:
            side = NODE_CLOSED_DIAMETER + NODE_CLOSED_PEN_WIDTH;
            break;
        case FolderState::HalfClosed:
            side = NODE_HALF_CLOSED_DIAMETER + NODE_HALF_CLOSED_PEN_WIDTH;
            break;
        }
    }

    /// half of the pen is drawn inside the shape and the other half is drawn
    /// outside of the shape, so pen-width plus diameter equals the total side length.
    auto rec = QRectF(0, 0, side, side);
    rec.moveCenter(rec.topLeft());

    return rec;
}

QPainterPath NodeItem::shape() const
{
    QPainterPath path;

    if (!fsScene()->isDir(_index)) {
        path = fileNodeShape(this, boundingRect());
    } else {
        switch (_state) {
        case FolderState::Open:
            path.addEllipse(boundingRect());
            break;
        case FolderState::Closed:
            path = closedNodeShape(this, boundingRect());
            break;
        case FolderState::HalfClosed:
            path.addEllipse(boundingRect());
            break;
        }
    }

    return path;
}

bool NodeItem::hasOpenOrHalfClosedChild() const
{
    const auto children = _childEdges | asTargetNodes;

    return ranges::any_of(children, [](auto* item) -> bool
    {
        return asNodeItem(item)->isOpen() || asNodeItem(item)->isHalfClosed();
    });
}

bool NodeItem::isDir() const
{
    return fsScene()->isDir(_index);
}

void NodeItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const auto* tm  = SessionManager::tm();
    const auto& rec = boundingRect();
    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(isSelected() ? tm->openNodeColor().lighter() : tm->openNodeColor());

    qreal radius = 0;
    if (!fsScene()->isDir(_index)) {
        paintFile(p, this);
    } else if (_state == FolderState::Open) {
        radius = rec.width() * 0.5 - NODE_OPEN_PEN_WIDTH * 0.5;
        p->setPen(QPen(tm->openNodeBorderColor(), NODE_OPEN_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
    } else if (_state == FolderState::Closed) {
        paintClosedFolder(p, this);
    } else if (_state == FolderState::HalfClosed) {
        radius = rec.width() * 0.5 - NODE_HALF_CLOSED_PEN_WIDTH * 0.5;
        p->setPen(QPen(tm->closedNodeBorderColor(), NODE_HALF_CLOSED_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
        p->setPen(QPen(tm->openNodeBorderColor(), NODE_HALF_CLOSED_PEN_WIDTH, Qt::SolidLine));
        p->setBrush(Qt::NoBrush);
        /// draw a 20 degree arc indicator for every child edge that is visible.
        auto rec2 = QRectF(0, 0, radius * 2, radius * 2);
        rec2.moveCenter(rec.center());
        constexpr auto span1 = -20 * 16;
        for (const auto* edge : _childEdges) {
            if (edge->target()->isVisible()) {
                const auto startAngle = qRound((edge->line().angle() + 10) * 16.0);
                p->drawArc(rec2, startAngle, span1);
            }
        }
    }
    if (auto *pr = asNodeItem(parentEdge()->source()); pr && pr->_state == FolderState::HalfClosed) {
        /// need to update the half-closed parent to avoid tearing of the
        /// 20-degree arc. A node can be Open, Closed, or Half-Closed and have
        /// a parent that's HalfClosed, so this update is needed for all.
        pr->update();
    }
}

void NodeItem::close()
{
    destroyChildren();
    setState(FolderState::Closed);

    shrink(this);

    if (auto* pr = asNodeItem(_parentEdge->source())) {
        pr->internalRotationAfterClose(_parentEdge);
    }
}

void NodeItem::halfClose()
{
    Q_ASSERT(hasOpenOrHalfClosedChild());

    setAllEdgeState(this, EdgeItem::CollapsedState);
    setState(FolderState::HalfClosed);
    adjustAllEdges(this);
}

void NodeItem::closeOrHalfClose(bool forceClose)
{
    _knot->hide();
    if (hasOpenOrHalfClosedChild()) {
        /// pick half closing as it is less destructive, unless the user is
        /// force closing.
        if (forceClose) {
            close();
        } else {
            halfClose();
        }
    } else {
        close();
    }
}

void NodeItem::open()
{
    Q_ASSERT(fsScene()->isDir(_index));

    if (_state == FolderState::Closed) {
        Q_ASSERT(_childEdges.empty());
        _knot->show();

        setState(FolderState::Open);
        extend(this);
        init();
        skipTo(0);
        spread();

        auto indices = _childEdges | asTargetNodeIndex;
        Q_ASSERT(std::ranges::all_of(indices, &QPersistentModelIndex::isValid));

    } else if (_state == FolderState::HalfClosed) {
        setState(FolderState::Open);
        spread();
        setAllEdgeState(this, EdgeItem::ActiveState);
        adjustAllEdges(this);
    }
}

void NodeItem::rotate(Rotation rot)
{
    auto targetNodes = _childEdges | asFilesOrClosedTargetNodes;

    if (ranges::distance(targetNodes) > 0) {
        animator->animateRotation(this, rot);
    }
}

void NodeItem::rotatePage(Rotation rot)
{
    auto targetNodes    = _childEdges | asFilesOrClosedTargetNodes;
    const auto pageSize = ranges::distance(targetNodes);

    animator->animatePageRotation(this, rot, pageSize);
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemScenePositionHasChanged:
        adjustAllEdges(this);
        break;

    case ItemSelectedChange:
        Q_ASSERT(value.canConvert<bool>());
        if (value.toBool()) { setZValue(1); } else { setZValue(0); }
        break;

    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}

void NodeItem::keyPressEvent(QKeyEvent *event)
{
    const auto mod = event->modifiers() == Qt::ShiftModifier;

    if (event->key() == Qt::Key_A) {
        if (mod) { rotatePage(Rotation::CCW); }
        else { rotate(Rotation::CCW); }
    } else if (event->key() == Qt::Key_D) {
        if (mod) { rotatePage(Rotation::CW); }
        else { rotate(Rotation::CW); }
    }
}

void NodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    Q_ASSERT(fsScene()->isDir(_index));

    switch (_state) {
    case FolderState::Open:
        closeOrHalfClose(event->modifiers() & Qt::ShiftModifier);
        break;
    case FolderState::Closed:
        open();
        break;
    case FolderState::HalfClosed:
        open();
        break;
    }
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (scene()->mouseGrabberItem() == this) {
        if (!_ancestorPos.empty()) {
            _ancestorPos.clear();
        }
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (scene()->mouseGrabberItem() == this) {
        if (event->modifiers() & Qt::ShiftModifier) {
            if (_ancestorPos.empty()) {
                _ancestorPos = getAncestorPos(this);
            }
            auto currPos = event->scenePos();
            auto lastPos = event->lastScenePos();
            spread(currPos - lastPos);

            for (auto& [item, pos] : _ancestorPos) {
                const auto lastVec = QLineF(lastPos, pos);
                const auto currVec = QLineF(currPos, pos);

                const auto len = lastVec.length();
                const auto dir = currVec.unitVector();

                const auto newPos = currPos + QPointF(dir.dx(), dir.dy()) * len;

                item->setPos(newPos);
                if (auto* node = asNodeItem(item); node) {
                    node->spread(newPos - pos);
                }

                lastPos = pos;
                currPos = newPos;
                pos     = newPos;
            }
        } else {
            /// 1. spread the parent node.
            if (auto* node = asNodeItem(_parentEdge->source())) {
                node->spread();
            }
            /// 2. spread this node, which is moving.
            const auto dxy = event->scenePos() - event->lastScenePos();
            spread(dxy);
            /// 3. spread the child nodes.
            for (const auto* edge : _childEdges) {
                if (auto* node = asNodeItem(edge->target()); !node->isClosed()) {
                    node->spread();
                }
            }
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void NodeItem::setState(FolderState state)
{
    if (_state != state) {
        prepareGeometryChange();
        _state = state;
    }
}

FileSystemScene* NodeItem::fsScene() const
{
    Q_ASSERT(scene());

    return qobject_cast<FileSystemScene*>(scene());
}

/// recursively destroys all child nodes and edges.
void NodeItem::destroyChildren()
{
    //animator->stop(this);

    /// QGraphicsScene will remove an item from the scene upon delete, but
    /// "it is more efficient to remove the item from the QGraphicsScene
    /// before destroying the item." -- Qt docs

    std::stack stack(_childEdges.begin(), _childEdges.end());

    while (!stack.empty()) {
        auto* edge = stack.top(); stack.pop();
        auto* node = asNodeItem(edge->target());
        Q_ASSERT(node);
        for (auto* childEdge : node->childEdges()) {
            Q_ASSERT(childEdge->source() == node);
            auto* childNode = asNodeItem(childEdge->target());
            Q_ASSERT(childNode);
            if (childNode->isOpen() || childNode->isHalfClosed()) {
                stack.emplace(childEdge);
            } else {
                Q_ASSERT(scene()->items().contains(childNode));
                scene()->removeItem(childNode);
                delete childNode;
                Q_ASSERT(scene()->items().contains(childEdge));
                scene()->removeItem(childEdge);
                delete childEdge;
            }
        }
        node->_childEdges.clear();

        Q_ASSERT(scene()->items().contains(node));
        scene()->removeItem(node);
        delete node;

        Q_ASSERT(scene()->items().contains(edge));
        scene()->removeItem(edge);
        delete edge;
    }

    _childEdges.clear();
}

void NodeItem::internalRotationAfterClose(EdgeItem* closedEdge)
{
    //animator->beginAnimation(this, 250);

    doInternalRotationAfterClose(closedEdge);
    updateAllChildNodes(this);
    update();

   // animator->endAnimation(this);
}

/// This is basically a sorting of all closed edges.
/// NOTE: after a few open/close and rotations, there will be gaps in between
/// edges, b/c we don't perform any kind of compacting, and that's okay.  The
/// user may not want the edge they just closed to change and point to a
/// different "folder".  Instead, we can offer the user an option to perform
/// the compacting whenever they want.
/// Node::rotate partially solves the problem by continuing to rotate until
/// there is no gap between two edges.
void NodeItem::doInternalRotationAfterClose(EdgeItem* closedEdge)
{
    Q_ASSERT(asNodeItem(closedEdge->target())->isClosed());
    Q_ASSERT(asNodeItem(closedEdge->target())->index().isValid());

    auto closedIndices = _childEdges
        | views::filter([closedEdge](EdgeItem* edge) -> bool
            { return edge != closedEdge; })
        | asClosedTargetNodes
        | views::transform(&NodeItem::index)
        | ranges::to<std::deque>()
        ;

    if (_state == FolderState::HalfClosed) {
        Q_ASSERT(closedEdge->state() == EdgeItem::ActiveState);
        closedEdge->setState(EdgeItem::CollapsedState);
        /// need to call both adjustAllEdges() and spread(). closedEdge is about
        /// to go into collapsed state, and without adjustAllEdges(), parts of
        /// it will remain un-collapsed.  spread() will indirectly trigger a
        /// call to adjustAllEdges(), but only if causes a change in position.
        /// a call to spread() is needed for when closedEdge doesn't line up
        /// with reset of the collapsed edges.
        spread();
        /// revisit this; there is some tearing when zooming in and performing close
        /// these two lines are also used in ::reload() in very similar use case.
        adjustAllEdges(this);
    }
    if (closedIndices.empty()) { return; }

    const auto openOrHalfClosedRows = _childEdges
        | asNotClosedTargetNodes
        | asIndexRow
        | ranges::to<std::unordered_set>()
        ;

    auto isGap = [](const QPersistentModelIndex& lhs, const QPersistentModelIndex& rhs)
        { return lhs.row() + 1 < rhs.row(); };
    const auto first = closedIndices.begin();
    const auto last  = closedIndices.end() - 1;

    for (auto gap = ranges::adjacent_find(closedIndices, isGap); gap != closedIndices.end(); ) {
        /// if there is one or more gaps, then there might be a suitable index,
        /// but the gap(s) could be wide and partially or completely be filled
        /// with open/half-closed nodes: try to find a suitable index.
        const auto end = (gap+1)->row();
        Q_ASSERT(gap->row() + 1 < end);
        for (auto start = gap->row() + 1; start != end; ++start) {
            if (const auto sibling = gap->sibling(start, 0); sibling.isValid() && !openOrHalfClosedRows.contains(sibling.row())) {
                closedIndices.insert(gap + 1, sibling);
                goto RELAYOUT;
            }
        }
        gap = std::ranges::adjacent_find(gap + 1, closedIndices.end(), isGap);
    }

    for (int k = 0, i = -1; k < NODE_CHILD_COUNT; ++k, --i) {
        if (auto before = first->sibling(first->row() + i, 0);
            before.isValid() && !openOrHalfClosedRows.contains(before.row())) {
            closedIndices.push_front(before);
            goto RELAYOUT;
        }
    }
    for (int k = 0, i = 1; k < NODE_CHILD_COUNT; ++k, ++i) {
        if (auto after = last->sibling(last->row() + i, 0);
            after.isValid() && !openOrHalfClosedRows.contains(after.row())) {
            closedIndices.push_back(after);
            goto RELAYOUT;
        }
    }

    RELAYOUT:

    auto closedNodes = _childEdges | asClosedTargetNodes;

    Q_ASSERT(std::ranges::all_of(closedIndices, &QPersistentModelIndex::isValid));
    Q_ASSERT(ranges::distance(closedNodes) == closedIndices.size());

    for (int k = 0; auto* node : closedNodes) {
        auto newIndex = closedIndices[k++];
        Q_ASSERT(newIndex.isValid());
        auto oldIndex = node->index();
        if (newIndex == oldIndex) { continue; }
        node->setIndex(newIndex);
        // animator->addRotation(node->parentEdge()
        //     , newIndex.row() > oldIndex.row() ? Rotation::CCW : Rotation::CW
        //     , node->name());
    }
}

/// performs CCW or CW rotation.
/// CW means going forward (i.e., the new node index is greater than the
/// previous).
InternalRotation NodeItem::doInternalRotation(Rotation rot)
{
    auto targetNodes = _childEdges
        | asFilesOrClosedTargetNodes
        | ranges::to<std::deque>();

    if (targetNodes.empty()) { return {}; }

    const auto openOrHalfClosedRows = _childEdges
        | asNotClosedTargetNodes
        | asIndexRow
        | ranges::to<std::unordered_set>();

    auto isAvailable = [&openOrHalfClosedRows](const QModelIndex& index) -> bool
    {
        return !openOrHalfClosedRows.contains(index.row());
    };

    if (rot == Rotation::CCW) { std::ranges::reverse(targetNodes); }

    const auto Inc       = rot == Rotation::CW ? 1 : -1;
    const auto lastIndex = targetNodes.back()->index();

    auto candidates = views::iota(1, NODE_CHILD_COUNT+1)
        | views::transform([Inc](int x) -> int { return x * Inc; })
        | views::transform([lastIndex](int i) -> QModelIndex
            { return lastIndex.sibling(lastIndex.row() + i, 0); })
        | views::filter(&QModelIndex::isValid)
        | views::filter(isAvailable)
        ;

    if (candidates.empty()) { return {}; }

    auto sibling = candidates.front();
    asNodeItem(_extra->target())->setIndex(sibling);
    _extra->setText(asNodeItem(_extra->target())->name());

    EdgeItem* toGrow   = nullptr;
    EdgeItem* toShrink = nullptr;

    const EdgeItem* toErase   = targetNodes.front()->parentEdge();
    const EdgeItem* insertPos = targetNodes.back()->parentEdge();

    QHash<QGraphicsItem*, qreal> angularDisplacements;
    QHash<QGraphicsItem*, qreal> angles;
    for (int i = 1; i < targetNodes.size(); ++i) {
        auto* a = targetNodes[i-1];
        auto* b = targetNodes[i  ];
        const auto la = QLineF(scenePos(), a->scenePos());
        const auto lb = QLineF(scenePos(), b->scenePos());
        angularDisplacements.insert(b, la.angle() - lb.angle());
        angles.insert(b, lb.angle());
    }

    Q_ASSERT(isFileOrClosed(_extra));
    if (auto found = ranges::find(_childEdges, insertPos); found != _childEdges.end()) {
        _extra->target()->setPos((*found)->target()->scenePos());
        toGrow = _extra;

        /// insert after if going CW, or insert before if going CCW.
        _childEdges.insert(rot==Rotation::CW ? std::next(found) : found, toGrow);
    } else { Q_ASSERT(false); }

    if (auto found = ranges::find(_childEdges, toErase); found != _childEdges.end()) {
        _extra = *found;
        _childEdges.erase(found);
        asNodeItem(_extra->target())->setIndex(QModelIndex());
        toShrink = _extra;
    } else { Q_ASSERT(false); }
    Q_ASSERT(isFileOrClosed(_extra));

    return { rot, this, toGrow, toShrink, angularDisplacements, angles };
}

void NodeItem::skipTo(int row)
{
    //animator->beginAnimation(this, 250);

    doSkipTo(row);

    //animator->endAnimation(this);
}

void NodeItem::doSkipTo(int row)
{
    const auto rowCount = _index.model()->rowCount(_index);

    Q_ASSERT(_childEdges.size() <= rowCount);

    auto closedNodes = _childEdges | asClosedTargetNodes;

    if (closedNodes.empty()) {
        return;
    }

    auto target = _index.model()->index(row, 0, _index);

    if (!target.isValid()) {
        return;
    }

    const auto openOrHalfClosedRows = _childEdges
        | asNotClosedTargetNodes
        | asIndexRow
        | ranges::to<std::unordered_set>()
        ;

    const auto rows = ranges::distance(closedNodes);

    std::deque<QPersistentModelIndex> newIndices;

    do {
        if (!target.isValid()) { break; }
        if (openOrHalfClosedRows.contains(target.row())) {
            target = target.sibling(target.row() + 1, 0);
            continue;
        }
        newIndices.push_back(target);
        target = target.sibling(target.row() + 1, 0);
    } while (newIndices.size() < rows);

    target = _index.model()->index(row - 1, 0, _index);
    while (newIndices.size() < rows) {
        if (!target.isValid()) { break; }
        if (openOrHalfClosedRows.contains(target.row())) {
            target = target.sibling(target.row() - 1, 0);
            continue;
        }
        newIndices.push_front(target);
        target = target.sibling(target.row() - 1, 0);
    }

    Q_ASSERT(newIndices.size() == rows);

    for (auto* node : closedNodes) {
        if (auto newIndex = newIndices.front(); node->index() != newIndex) {
            const auto rot = node->index().row() < newIndex.row()
                ? Rotation::CCW : Rotation::CW;
            node->setIndex(newIndex);
           // animator->addRotation(node->parentEdge(), rot, node->name());
            node->parentEdge()->setText(node->name());
        }
        newIndices.pop_front();
    }
}

/// dxy is used only for the node that is being moved by the mouse.  Without
/// dxy, the child nodes get scattered all over when the parent node is moved
/// too quickly or rapidly.
void NodeItem::spread(QPointF dxy)
{
    if (_childEdges.empty()) { return; }

    const auto* grabber = scene()->mouseGrabberItem();
    const auto center   = mapToScene(QPointF(0, 0));
    const auto sides    = _childEdges.size() + (_parentEdge ? 1 : 0);
    auto guides         = circle(center, sides);

    auto intersectsWith = [](const QLineF& line)
    {
        return [line](const QLineF& other) -> bool
        {
            return other.intersects(line) == QLineF::BoundedIntersection;
        };
    };
    auto isIncluded = [grabber](const NodeItem* node) -> bool
    {
        return (!node->isDir() || node->isClosed()) && node != grabber;
    };
    auto isExcluded = [isIncluded](const NodeItem* node) -> bool
    {
        return !isIncluded(node);
    };

    if (_parentEdge) {
        const auto* ps        = _parentEdge->source();
        const auto parentEdge = QLineF(center, ps->mapToScene(QPointF(0, 0)));
        const auto ignored    = ranges::find_if(guides, intersectsWith(parentEdge));
        if (ignored != guides.end()) {
            /// only remove one guide edge per intersecting edge.  E.g., When
            /// guides contains only two lines, parentEdge will most likely
            /// intersect with two.
            guides.erase(ignored);
        }
    }

    auto excludedNodes = _childEdges | asTargetNodes | views::filter(isExcluded);

    for (const auto* node : excludedNodes) {
        const auto nodeLine = QLineF(center, node->mapToScene(QPointF(0, 0)));
        const auto ignored  = std::ranges::find_if(guides, intersectsWith(nodeLine));
        if (ignored != guides.end()) {
            guides.erase(ignored);
        }
    }

    auto includedNodes = _childEdges | asTargetNodes | views::filter(isIncluded);

    for (auto* node : includedNodes) {
        if (guides.empty()) {
            break;
        }
        const auto guide = guides.front();
        const auto norm  = guide.normalVector();

        /// TODO: node lengths are unique to each File/Folder, and have default
        /// value of 144 for now.  If the edge length is changed by the user, it
        /// is saved in the Model and then in the DB.
        auto nodeLine = QLineF(pos(), pos() + QPointF(1, 1));
        nodeLine.setLength(144);

        nodeLine.setAngle(norm.angle());
        node->setPos(nodeLine.p2() + dxy);
        guides.pop_front();
    }
}

void core::extend(NodeItem* node, qreal distance)
{
    const auto* pe = node->parentEdge();

    /// pos()/setPos() are in scene coordinates if there is no parent.
    Q_ASSERT(node->parentItem() == nullptr);
    Q_ASSERT(pe->target() == node);

    const auto& source = pe->source()->pos();
    const auto& target = pe->target()->pos();
    auto line = QLineF(source, target);
    line.setLength(line.length() + distance);
    node->setPos(line.p2());
}

void core::shrink(NodeItem* node, qreal distance)
{
    const auto* pe = node->parentEdge();

    /// pos()/setPos() are in scene coordinates if there is no parent.
    Q_ASSERT(node->parentItem() == nullptr);
    Q_ASSERT(pe->target() == node);

    const auto& source = pe->source()->pos();
    const auto& target = pe->target()->pos();
    auto line = QLineF(source, target);
    line.setLength(qMax(distance, line.length() - distance));
    node->setPos(line.p2());
}

void core::adjustAllEdges(const NodeItem* node)
{
    node->parentEdge()->adjust();
    ranges::for_each(node->childEdges(), &EdgeItem::adjust);
}

void core::updateAllChildNodes(const NodeItem* node)
{
    for (auto* child : node->childEdges() | asTargetNodes) {
        child->update();
    }
}

/// only used on closed nodes, but can be made more general if needed.
void core::setAllEdgeState(const NodeItem* node, EdgeItem::State state)
{
    for (auto* edge : node->childEdges() | asClosedEdges) {
        edge->setState(state);
    }
}


////////////////
/// Animator ///
////////////////
void Animator::animateRotation(NodeItem* node, Rotation rot)
{
    auto* seq  = getSeq(node);
    auto* anim = createVariantAnimation(200);
    addRotation(node, rot, anim);
    seq->addAnimation(anim);
    fastforward(seq);
    startAnimation(node);
}

void Animator::animatePageRotation(NodeItem* node, Rotation rot, int page)
{
    auto* seq = getSeq(node);

    for (int i = 0; i< page; ++i) {
        auto* anim = createVariantAnimation(25);
        addRotation(node, rot, anim);
        seq->addAnimation(anim);
    }
    fastforward(seq);
    startAnimation(node);
}

QVariantAnimation *Animator::createAnimation(const NodeItem *node, int duration)
{

}

void Animator::startAnimation(const NodeItem* node)
{
    Q_ASSERT(_seqs.contains(node));

    if (const auto found = _seqs.find(node); found != _seqs.end()) {
        if (auto* seq = found->second; seq->state() == QAbstractAnimation::Stopped) {
            seq->start();
        }
    }
}

void Animator::addRotation(NodeItem* node, const Rotation& rot, QVariantAnimation* va)
{
    Q_ASSERT(node);
    Q_ASSERT(va);

    connect(va, &QVariantAnimation::stateChanged,
    [this, node, rot, va](QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
        if (oldState == QAbstractAnimation::Stopped && newState == QAbstractAnimation::Running) {
            /// startRotation must be called only once at the beginning; therefore,
            /// disconnect to make sure that it never gets called again.  This
            /// can happen when rapid rotation is triggered (e.g., rotation key
            /// is pressed down), and causes all kinds of issues.
            /// I'm not exactly sure what causes the state to go from Stopped to
            /// Running a second time.
            disconnect(va, &QVariantAnimation::stateChanged, nullptr, nullptr);
            startRotation(node, rot, va);
        }
    });

    connect(va, &QVariantAnimation::valueChanged,
    [this, va] (const QVariant& value)
    {
        Q_ASSERT(va->state() == QAbstractAnimation::Running);
        Q_ASSERT(_varData.contains(va));

        auto ok         = false;
        const auto t    = value.toReal(&ok); Q_ASSERT(ok);
        const auto data = _varData[va].value<InternalRotation>();

        interpolate(t, data);
    });
}

template <class Mapping>
void Animator::addTransition(NodeItem* node, const Mapping& mapping)
{
    // auto* va = _seqAnimations[node];
    // Q_ASSERT(va);
    //
    // va->setStartValue(0.0);
    // va->setEndValue(1.0);
    //
    // connect(va, &QVariantAnimation::valueChanged,
    //     [mapping] (const QVariant& value)
    //     {
    //         auto ok = false;
    //         const qreal t = value.toReal(&ok);
    //         Q_ASSERT(ok);
    //
    //         for (auto [k,v] : mapping) {
    //             const auto line = QLineF(v.before, v.after);
    //             k->setPos(line.pointAt(t));
    //         }
    //     });
}


void Animator::startRotation(NodeItem* node, Rotation rot, QVariantAnimation* va)
{
    Q_ASSERT(!_varData.contains(va));

    const auto data = node->doInternalRotation(rot);

    if (data.node == nullptr) {
        disconnect(va, &QVariantAnimation::valueChanged, nullptr, nullptr);
        va->stop();

        return;
    }

    auto* toGrow = data.toGrow;

    toGrow->setOpacity(0.0);
    toGrow->target()->setOpacity(0.0);
    toGrow->show();
    toGrow->target()->show();

    _varData.emplace(va, QVariant::fromValue(data));
}

QSequentialAnimationGroup* Animator::getSeq(const NodeItem* node)
{
    if (auto found = _seqs.find(node); found == _seqs.end()) {
        auto* seq = new QSequentialAnimationGroup(this);

        connect(seq, &QAbstractAnimation::finished,
        [this, node]
        {
            clearSequence(node);
        });

        _seqs.emplace(node, seq);
    }

    return _seqs[node];
}

void Animator::clearSequence(const NodeItem* node)
{
    Q_ASSERT(_seqs.contains(node));

    auto* seq = _seqs[node];

    for (int i = 0; i < seq->animationCount(); ++i) {
        if (const auto* anim = qobject_cast<QVariantAnimation*>(seq->animationAt(i)); anim) {
            /// _varData may not contain 'anim' because it was never added in startRotation.
            if (auto found = _varData.find(anim); found != _varData.end()) {
                _varData.erase(found);
            }
        }
    }

    seq->clear();
}

/// (progressively) shortent the duration of animations as they get added
/// to the queue.  This can happen when rotation is repeatedly triggered,
/// and we don't want them to pile up.
void Animator::fastforward(QSequentialAnimationGroup* seq)
{
    if (const auto count = seq->animationCount(); count > 0) {
        const auto head  = seq->indexOfAnimation(seq->currentAnimation()) + 1;
        const auto len   = count - head;
        const auto fast  = qMax(10, 125 / qMax(1, len));

        for (int i = head; i < seq->animationCount(); ++i) {
            if (auto* va = qobject_cast<QVariantAnimation*>(seq->animationAt(i)); va) {
                Q_ASSERT(va->state() == QAbstractAnimation::Stopped);
                va->setDuration(fast);
                if (i + 1 == count) {
                    va->setEasingCurve(QEasingCurve::OutSine);
                } else {
                    va->setEasingCurve(QEasingCurve::Linear);
                }
            }
        }
    }
}

QVariantAnimation* Animator::createVariantAnimation(int duration)
{
    auto* va = new QVariantAnimation(this);

    va->setLoopCount(1);
    va->setDuration(duration);
    va->setStartValue(0.0);
    va->setEndValue(1.0);
    va->setEasingCurve(QEasingCurve::OutSine);

    return va;
}

void Animator::interpolate(qreal t, const InternalRotation& data)
{
    const auto* node  = data.node;
    const auto deltas = data.angularDisplacement;
    const auto angles = data.angles;
    auto* toGrow      = data.toGrow;
    auto* toShrink    = data.toShrink;

    QHashIterator ca(angles);

    while (ca.hasNext()) {
        ca.next();

        auto* child = ca.key();
        auto ang    = ca.value();
        Q_ASSERT(deltas.contains(child));
        const qreal dir = t * deltas.value(child, 0.0);

        auto line = QLineF(node->scenePos(), child->scenePos());
        line.setAngle(ang + dir);
        child->setPos(line.p2());
    }

    auto line = QLineF(node->scenePos(), toGrow->target()->scenePos());
    line.setLength(t * 144);
    toGrow->target()->setPos(line.p2());
    if (t >= 0.4) {
        toGrow->setOpacity(t);
        toGrow->target()->setOpacity(t);
    }

    if (toShrink->isVisible() && t >= 0.6) {
        toShrink->hide();
        toShrink->target()->hide();
        toShrink->setOpacity(1.0);
        toShrink->target()->setOpacity(1.0);
    } else {
        const auto t1 = 1.0 - t;
        line = QLineF(node->scenePos(), toShrink->target()->scenePos());
        line.setLength(t1 * 144);
        toShrink->target()->setPos(line.p2());
        toShrink->setOpacity(t1);
        toShrink->target()->setOpacity(t1);
    }
}

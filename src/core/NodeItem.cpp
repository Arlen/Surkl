/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "NodeItem.hpp"
#include "FileSystemScene.hpp"
#include "SessionManager.hpp"
#include "theme.hpp"
#include "layout.hpp"

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

/// TODO: this is very similar to NodeItem::spread().
auto core::spreadWithAnimation(const NodeItem* node)
{
    auto edgeOf = [node](const QGraphicsItem* item) {
        return QLineF(QPointF(0, 0), node->mapFromItem(item, QPointF(0, 0)));
    };
    const auto sides = node->childEdges().size() + (node->parentEdge() ? 1 : 0) + 1;
    auto guides      = circle(static_cast<int>(sides), edgeOf(node->knot()).angle());

    auto intersectsWith = [](const QLineF& line)
    {
        return [line](const QLineF& other) -> bool
        {
            return other.intersects(line) == QLineF::BoundedIntersection;
        };
    };
    auto removeIfIntersects = [&guides, edgeOf, intersectsWith](const QGraphicsItem* item) {
        /// using find_if b/c only one guide edge per intersecting edge should
        /// be removed. e.g., when 'guides' contains only two lines, parentEdge
        /// will most likely intersect with two.
        const auto ignored = ranges::find_if(guides, intersectsWith(edgeOf(item)));
        if (ignored != guides.end()) {
            guides.erase(ignored);
        }
    };

    if (node->parentEdge()) { removeIfIntersects(node->parentEdge()->source()); }
    if (node->knot()) { removeIfIntersects(node->knot()); }

    for (const auto* child : node->childEdges() | asNotClosedTargetNodes) {
        removeIfIntersects(child);
    }

    auto includedNodes = node->childEdges() | asFilesOrClosedTargetNodes;

    SpreadAnimationData result;

    for (auto* child : includedNodes) {
        if (guides.empty()) {
            break;
        }
        const auto oldPos = child->scenePos();
        const auto guide  = guides.front();
        const auto norm   = guide.normalVector();

        /// TODO: node lengths are unique to each File/Folder, and have default
        /// value of 144 for now.  If the edge length is changed by the user, it
        /// is saved in the Model and then in the DB.
        auto childLine = QLineF(node->pos(), node->pos() + QPointF(1, 1));
        childLine.setLength(144);
        childLine.setAngle(norm.angle());

        if (const auto newPos = childLine.p2(); oldPos != newPos) {
            result.movement.insert(child, {oldPos, newPos});
        }
        guides.pop_front();
    }

    return result;
}

Q_GLOBAL_STATIC(core::Animator, animator)

////////////////
/// RootItem ///
////////////////
RootItem::RootItem()
{
    const auto* tm = SessionManager::tm();

    setRect(QRectF(-12, -12, 24, 24));
    setPen(Qt::NoPen);
    setBrush(tm->edgeColor());
    setFlags(ItemIsSelectable | ItemIsMovable |  ItemSendsScenePositionChanges);
}

void RootItem::setChildEdge(EdgeItem* edge)
{
    Q_ASSERT(_childEdge == nullptr);

    _childEdge = edge;
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
        _childEdge->adjust();
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
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsScenePositionChanges);

    _index = index;
    _knot  = new KnotItem(this);
    _knot->setPos(QPointF(NODE_OPEN_RADIUS, 0));
    _knot->hide();
}

NodeItem::~NodeItem()
{

}

EdgeItem* NodeItem::createNode(const QPersistentModelIndex& targetIndex, QGraphicsItem* source)
{
    Q_ASSERT(source);

    auto* target        = new NodeItem(targetIndex);
    target->_parentEdge = new EdgeItem(source, target);

    if (targetIndex.isValid()) {
        target->_parentEdge->setText(target->name());
    }

    return target->_parentEdge;
}

EdgeItem* NodeItem::createRootNode(const QPersistentModelIndex& index)
{
    auto* root = new RootItem();
    auto* edge = createNode(index, root);
    root->setChildEdge(edge);
    root->setPos(-128, 0);

    return edge;
}

void NodeItem::createChildNodes()
{
    Q_ASSERT(parentEdge());
    Q_ASSERT(knot());

    const auto* model = _index.model();
    const auto count  = std::min(NODE_CHILD_COUNT, model->rowCount(_index));

    const auto sides = count
        + 1  // for parentEdge()
        + 1; // for knot()

    std::vector<const QGraphicsItem*> excludedItems;
    excludedItems.push_back(parentEdge());
    excludedItems.push_back(knot());
    auto gl = guideLines(this, sides, excludedItems);

    QList<NodeData> data;

    for (auto i : std::views::iota(0, count)) {
        const auto index = model->index(i, 0, _index);
        const auto norm  = gl.front().normalVector();

        auto nodeLine = QLineF(pos(), pos() + QPointF(1, 1));
        nodeLine.setLength(144);
        nodeLine.setAngle(norm.angle());

        data.push_back({index, NodeType::ClosedNode, nodeLine.p2(), 0});
        gl.pop_front();
    }

    createChildNodes(data);
}

void NodeItem::createChildNodes(QList<NodeData>& data)
{
    Q_ASSERT(scene());
    Q_ASSERT(_childEdges.empty());
    Q_ASSERT(_index.isValid());
    Q_ASSERT(_extra == nullptr);

    _knot->show();
    setNodeType(NodeType::OpenNode);

    const auto count = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    for (auto& d : data | views::take(count)) {
        d.edge = createNode(d.index, this);
        scene()->addItem(d.edge->target());
        scene()->addItem(d.edge);
        d.edge->target()->setPos(d.pos);
        d.edge->adjust();
        _childEdges.emplace_back(d.edge);
    }

    _extra = createNode(QModelIndex(), this);
    scene()->addItem(_extra->target());
    scene()->addItem(_extra);
    _extra->target()->hide();
    _extra->hide();
}

void NodeItem::reload(int start, int end)
{
    if (_nodeType == NodeType::ClosedNode) {
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
            if (_nodeType == NodeType::OpenNode) {
                spread();
            }
            else if (_nodeType == NodeType::HalfClosedNode) {
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

                skipTo(start);

                setAllEdgeState(this, EdgeItem::CollapsedState);
                //setNodeType(NodeType::HalfClosed);
                spread();
                adjustAllEdges(this);
            }
        }
    }

    if (_nodeType == NodeType::OpenNode) {
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
            node->setNodeType(NodeType::ClosedNode);
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

    if (fsScene()->isDir(index)) {
        _nodeType = NodeType::ClosedNode;
    } else {
        _nodeType = NodeType::FileNode;
    }
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

    switch (_nodeType) {
    case NodeType::FileNode:
        side = NODE_CLOSED_DIAMETER + NODE_CLOSED_PEN_WIDTH;
        break;
    case NodeType::OpenNode:
        side = NODE_OPEN_DIAMETER + NODE_OPEN_PEN_WIDTH;
        break;
    case NodeType::ClosedNode:
        side = NODE_CLOSED_DIAMETER + NODE_CLOSED_PEN_WIDTH;
        break;
    case NodeType::HalfClosedNode:
        side = NODE_HALF_CLOSED_DIAMETER + NODE_HALF_CLOSED_PEN_WIDTH;
        break;
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

    switch (_nodeType) {
    case NodeType::FileNode:
        path = fileNodeShape(this, boundingRect());
        break;
    case NodeType::OpenNode:
        path.addEllipse(boundingRect());
        break;
    case NodeType::ClosedNode:
        path = closedNodeShape(this, boundingRect());
        break;
    case NodeType::HalfClosedNode:
        path.addEllipse(boundingRect());
        break;
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

void NodeItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const auto* tm  = SessionManager::tm();
    const auto& rec = boundingRect();
    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(isSelected() ? tm->openNodeColor().lighter() : tm->openNodeColor());

    qreal radius = 0;
    if (_nodeType == NodeType::FileNode) {
        paintFile(p, this);
    } else if (_nodeType == NodeType::OpenNode) {
        radius = rec.width() * 0.5 - NODE_OPEN_PEN_WIDTH * 0.5;
        p->setPen(QPen(tm->openNodeBorderColor(), NODE_OPEN_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
    } else if (_nodeType == NodeType::ClosedNode) {
        paintClosedFolder(p, this);
    } else if (_nodeType == NodeType::HalfClosedNode) {
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
    if (auto *pr = asNodeItem(parentEdge()->source()); pr && pr->_nodeType == NodeType::HalfClosedNode) {
        /// need to update the half-closed parent to avoid tearing of the
        /// 20-degree arc. A node can be Open, Closed, or Half-Closed and have
        /// a parent that's HalfClosed, so this update is needed for all.
        pr->update();
    }
}

void NodeItem::close()
{
    Q_ASSERT(isDir() && (isOpen() || isHalfClosed()));

    animator->clearAnimations(this);

    destroyChildren();
    setNodeType(NodeType::ClosedNode);

    shrink(this);

    if (auto* pr = asNodeItem(_parentEdge->source())) {
        animator->animateRelayout(pr, _parentEdge);
    }
}

void NodeItem::halfClose()
{
    Q_ASSERT(hasOpenOrHalfClosedChild());

    setAllEdgeState(this, EdgeItem::CollapsedState);
    setNodeType(NodeType::HalfClosedNode);
    adjustAllEdges(this);
}

void NodeItem::closeOrHalfClose(bool forceClose)
{
    Q_ASSERT(isDir() && (isOpen() || isHalfClosed()));

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

    if (_nodeType == NodeType::ClosedNode) {
        Q_ASSERT(_childEdges.empty());
        _knot->show();
        setNodeType(NodeType::OpenNode);
        extend(this);
        Q_ASSERT(_extra == nullptr);
        init();
        Q_ASSERT(_extra != nullptr);
        skipTo(0);
        spread();

        auto indices = _childEdges | asTargetNodeIndex;
        Q_ASSERT(std::ranges::all_of(indices, &QPersistentModelIndex::isValid));

    } else if (_nodeType == NodeType::HalfClosedNode) {
        setNodeType(NodeType::OpenNode);
        spread();
        setAllEdgeState(this, EdgeItem::ActiveState);
        adjustAllEdges(this);
    }
}

void NodeItem::rotate(Rotation rot)
{
    if (!isOpen()) {
        return;
    }

    auto availableNodes = _childEdges | asFilesOrClosedTargetNodes;

    if (ranges::distance(availableNodes) > 0) {
        animator->animateRotation(this, rot);
    }
}

void NodeItem::rotatePage(Rotation rot)
{
    if (!isOpen()) {
        return;
    }

    auto availableNodes = _childEdges | asFilesOrClosedTargetNodes;
    const auto pageSize = ranges::distance(availableNodes);

    if (pageSize > 0) {
        animator->animatePageRotation(this, rot, pageSize);
    }
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

    switch (_nodeType) {
    case NodeType::FileNode:
        fsScene()->openFile(this);
        break;
    case NodeType::OpenNode:
        closeOrHalfClose(event->modifiers() & Qt::ShiftModifier);
        break;
    case NodeType::ClosedNode:
        open();
        break;
    case NodeType::HalfClosedNode:
        open();
        break;
    }
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
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

void NodeItem::setNodeType(NodeType type)
{
    if (_nodeType != type) {
        prepareGeometryChange();
        _nodeType = type;
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

    auto destroyEdge = [this](EdgeItem* edge) {
        auto* node = edge->target();

        Q_ASSERT(scene()->items().contains(node));
        scene()->removeItem(node);
        delete node;

        Q_ASSERT(scene()->items().contains(edge));
        scene()->removeItem(edge);
        delete edge;
    };

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
                destroyEdge(childEdge);
            }
        }
        node->_childEdges.clear();

        if (node->_extra) {
            destroyEdge(node->_extra);
        }
        destroyEdge(edge);
    }

    _childEdges.clear();

    destroyEdge(_extra);
    _extra = nullptr;
}

/// repositions a closed node.
/// If possible, any existing gaps are filled first; otherwise, the closed node
/// is placed either in the beginning or end of the list of child nodes.
void NodeItem::repositionAfterClose(EdgeItem* closed)
{
    Q_ASSERT(asNodeItem(closed->target())->isClosed());
    Q_ASSERT(asNodeItem(closed->target())->index().isValid());

    auto allButClosedEdge = _childEdges
        | views::filter([closed](EdgeItem* edge) -> bool
            { return edge != closed; });

    const auto fileOrClosedIndices = allButClosedEdge
        | asFilesOrClosedTargetNodes
        | asIndex
        | ranges::to<std::deque>()
        ;

    if (_nodeType == NodeType::HalfClosedNode) {
        Q_ASSERT(closed->state() == EdgeItem::ActiveState);
        closed->setState(EdgeItem::CollapsedState);
        /// need to call both adjustAllEdges() and spread(). closedEdge is about
        /// to go into collapsed state, and without adjustAllEdges(), parts of
        /// it will remain un-collapsed.  spread() will indirectly trigger a
        /// call to adjustAllEdges(), but only if it causes a change in position.
        /// a call to spread() is needed for when closedEdge doesn't line up
        /// with reset of the collapsed edges.
        spread();
        /// revisit this; there is some tearing when zooming in and performing close
        /// these two lines are also used in ::reload() in very similar use case.
        adjustAllEdges(this);
    }
    if (fileOrClosedIndices.empty()) { return; }

    const auto usedRows = allButClosedEdge
        | asTargetNodes | asIndexRow
        | ranges::to<std::unordered_set>();

    auto isGap = [](const QPersistentModelIndex& lhs, const QPersistentModelIndex& rhs)
        { return lhs.row() + 1 < rhs.row(); };
    const auto first = fileOrClosedIndices.begin();
    const auto last  = fileOrClosedIndices.end() - 1;

    auto assignIndex = [](EdgeItem* edge, const QPersistentModelIndex& index) {
        asNodeItem(edge->target())->setIndex(index);
        edge->setText(asNodeItem(edge->target())->name());
    };

    auto sortByRows = [](EdgeDeque& edges) {
        std::ranges::sort(edges, [](const EdgeItem* a, const EdgeItem* b) {
            return asNodeItem(a->target())->index().row() <
                asNodeItem(b->target())->index().row();
        });
    };

    for (auto gap = ranges::adjacent_find(fileOrClosedIndices, isGap); gap != fileOrClosedIndices.end(); ) {
        /// if there is one or more gaps, then there might be a suitable index,
        /// but the gap(s) could be wide and partially or completely be filled
        /// with open/half-closed nodes: try to find a suitable index.

        Q_ASSERT(std::next(gap) != fileOrClosedIndices.end());
        const auto end = std::next(gap)->row();
        Q_ASSERT(gap->row() + 1 < end);
        for (auto start = gap->row() + 1; start != end; ++start) {
            if (const auto sibling = gap->sibling(start, 0);
                sibling.isValid() && !usedRows.contains(sibling.row())) {
                assignIndex(closed, sibling);
                return sortByRows(_childEdges);
            }
        }
        gap = std::ranges::adjacent_find(std::next(gap), fileOrClosedIndices.end(), isGap);
    }

    for (int k = 0, i = -1; k < NODE_CHILD_COUNT; ++k, --i) {
        if (auto before = first->sibling(first->row() + i, 0);
            before.isValid() && !usedRows.contains(before.row())) {
            assignIndex(closed, before);
            return sortByRows(_childEdges);
        }
    }
    for (int k = 0, i = 1; k < NODE_CHILD_COUNT; ++k, ++i) {
        if (auto after = last->sibling(last->row() + i, 0);
            after.isValid() && !usedRows.contains(after.row())) {
            assignIndex(closed, after);
            return sortByRows(_childEdges);
        }
    }
    Q_ASSERT(false);
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
    const auto rowCount = _index.model()->rowCount(_index);

    Q_ASSERT(_childEdges.size() <= rowCount);

    auto availableNodes = _childEdges | asFilesOrClosedTargetNodes;

    if (availableNodes.empty()) {
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

    const auto rows = ranges::distance(availableNodes);

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

    for (auto* node : availableNodes) {
        if (auto newIndex = newIndices.front(); node->index() != newIndex) {
            node->setIndex(newIndex);
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

    auto includedNodes = _childEdges | asFilesOrClosedTargetNodes
        | views::filter([grabber](const NodeItem* node)
            { return node != grabber; })
        ;

    for (auto gl = guideLines(this); auto* node : includedNodes) {
        if (gl.empty()) {
            break;
        }

        const auto norm = gl.front().normalVector();

        /// TODO: node lengths are unique to each File/Folder, and have default
        /// value of 144 for now.  If the edge length is changed by the user, it
        /// is saved in the Model and then in the DB.
        auto nodeLine = QLineF(pos(), pos() + QPointF(1, 1));
        nodeLine.setLength(144);
        nodeLine.setAngle(norm.angle());
        node->setPos(nodeLine.p2() + dxy);
        gl.pop_front();
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
    for (auto* edge : node->childEdges() | asFilesOrClosedEdges) {
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

void Animator::animateRelayout(NodeItem* node, EdgeItem* closedEdge)
{
    auto* seq  = getSeq(node);
    auto* anim = createVariantAnimation(200);
    addRelayout(node, closedEdge, anim);

    if (const auto* current = seq->currentAnimation(); current) {
        seq->insertAnimation(seq->indexOfAnimation(seq->currentAnimation()) + 1, anim);
    } else {
        seq->insertAnimation(0, anim);
    }
    startAnimation(node);
}

void Animator::clearAnimations(NodeItem* node)
{
    if (auto foundSeq = _seqs.find(node); foundSeq != _seqs.end()) {
        auto* seq = foundSeq->second;
        seq->stop();
        for (int i = 0; i < seq->animationCount(); ++i) {
            if (const auto* anim = qobject_cast<QVariantAnimation*>(seq->animationAt(i)); anim) {
                /// _varData may not contain 'anim' because it was never added in start#().
                if (auto foundData = _animData.find(anim); foundData != _animData.end()) {
                    _animData.erase(foundData);
                }
            }
        }

        seq->clear();
        _seqs.erase(node);
        delete seq;
    }
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
        Q_ASSERT(_animData.contains(va));

        auto ok         = false;
        const auto t    = value.toReal(&ok); Q_ASSERT(ok);
        const auto data = _animData[va].value<InternalRotation>();

        interpolate(t, data);
    });
}

void Animator::addRelayout(NodeItem* node, EdgeItem* closedEdge, QVariantAnimation* va)
{
    Q_ASSERT(node);
    Q_ASSERT(va);

    connect(va, &QVariantAnimation::stateChanged,
    [this, node, closedEdge, va](QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
        if (oldState == QAbstractAnimation::Stopped && newState == QAbstractAnimation::Running) {
            disconnect(va, &QVariantAnimation::stateChanged, nullptr, nullptr);
            startRelayout(node, closedEdge, va);
        }
    });

    connect(va, &QVariantAnimation::valueChanged,
    [this, va] (const QVariant& value)
    {
        Q_ASSERT(va->state() == QAbstractAnimation::Running);
        Q_ASSERT(_animData.contains(va));

        auto ok         = false;
        const auto t    = value.toReal(&ok); Q_ASSERT(ok);
        const auto data = _animData[va].value<SpreadAnimationData>();

        for (QHashIterator it(data.movement); it.hasNext(); ) {
            it.next();
            const auto line = QLineF(it.value().oldPos, it.value().newPos);
            it.key()->setPos(line.pointAt(t));
        }
    });
}

void Animator::startRotation(NodeItem* node, Rotation rot, QVariantAnimation* va)
{
    Q_ASSERT(!_animData.contains(va));

    const auto data = node->doInternalRotation(rot);

    if (data.node == nullptr) {
        disconnect(va, &QVariantAnimation::valueChanged, nullptr, nullptr);
        va->stop();

        return;
    }

    auto* toGrow = data.toGrow;
    Q_ASSERT(toGrow != nullptr);

    toGrow->setOpacity(0.0);
    toGrow->target()->setOpacity(0.0);
    toGrow->show();
    toGrow->target()->show();

    _animData.emplace(va, QVariant::fromValue(data));
}

void Animator::startRelayout(NodeItem *node, EdgeItem *closedEdge, QVariantAnimation *va)
{
    Q_ASSERT(!_animData.contains(va));

    node->repositionAfterClose(closedEdge);
    const auto data = spreadWithAnimation(node);

    if (data.movement.empty()) {
        disconnect(va, &QVariantAnimation::valueChanged, nullptr, nullptr);
        va->stop();

        return;
    }

    _animData.emplace(va, QVariant::fromValue(data));
}


QSequentialAnimationGroup* Animator::getSeq(const NodeItem* node)
{
    if (const auto found = _seqs.find(node); found == _seqs.end()) {
        auto* seq = new QSequentialAnimationGroup(this);

        connect(seq, &QAbstractAnimation::finished,
        [this, node]
        {
            clearSequence(node);

#ifdef TEST_ANIMATIONS
            emit node->fsScene()->sequenceFinished();
#endif
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
            /// _varData may not contain 'anim' because it was never added in start#().
            if (auto found = _animData.find(anim); found != _animData.end()) {
                _animData.erase(found);
            }
        }
    }

    seq->clear();
}

QVariantAnimation* Animator::createVariantAnimation(int duration)
{
    auto* va = new QVariantAnimation(this);

#ifdef TEST_ANIMATIONS
    duration = 1;
#endif

    va->setLoopCount(1);
    va->setDuration(duration);
    va->setStartValue(0.0);
    va->setEndValue(1.0);
    va->setEasingCurve(QEasingCurve::OutSine);

    return va;
}

/// (progressively) shortent the duration of animations as they get added
/// to the queue.  This can happen when rotation is repeatedly triggered,
/// and we don't want them to pile up.
void Animator::fastforward(const QSequentialAnimationGroup* seq)
{
    if (const auto count = seq->animationCount(); count > 0) {
        const auto head  = seq->indexOfAnimation(seq->currentAnimation()) + 1;
        const auto len   = count - head;

#ifdef TEST_ANIMATIONS
        const auto fast  = 1;
#else
        const auto fast  = qMax(10, 125 / qMax(1, len));
#endif

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
        const auto angle  = ca.value();
        Q_ASSERT(deltas.contains(child));

        const qreal displacement = t * deltas.value(child, 0.0);
        const auto newAngle = angle + displacement;
        auto line = QLineF(node->scenePos(), child->scenePos());

        line.setAngle(newAngle);
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

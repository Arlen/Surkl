/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "NodeItem.hpp"
#include "FileSystemScene.hpp"
#include "SceneStorage.hpp"
#include "SessionManager.hpp"
#include "layout.hpp"
#include "gui/theme/theme.hpp"

#include <QDir>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QStyleOptionGraphicsItem>
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
        while (std::ssize(result) < count) {
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
        while (std::ssize(result) < count) {
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

        auto guide = QLineF(center, center + QPointF(rec.width()*0.4, 0));
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

    void paintClosedFolder(QPainter* p, const QStyleOptionGraphicsItem *option, const NodeItem* node)
    {
        Q_ASSERT(node->isClosed());
        Q_ASSERT(node->shape().elementCount() == 4);

        const auto* tm = SessionManager::tm();

        const auto rec    = node->boundingRect();
        const auto center = rec.center();
              auto shape  = node->shape();
        const auto top    = QLineF(shape.elementAt(1), shape.elementAt(2));
        const auto bot    = QLineF(shape.elementAt(0), shape.elementAt(3));
        const auto spine  = QLineF(bot.pointAt(0.5), top.pointAt(0.5));
        const auto tri    = QPolygonF() << shape.elementAt(1)
                                        << center
                                        << shape.elementAt(2);

        const auto color1 = node->isSelected() || (option->state & QStyle::State_MouseOver)
            ? tm->closedNodeMidlightColor()
            : tm->closedNodeColor();
        const auto color2 = node->isSelected() || (option->state & QStyle::State_MouseOver)
            ? tm->closedNodeMidlightColor()
            : tm->closedNodeMidarkColor();

        p->setBrush(tm->closedNodeDarkColor());
        p->setPen(Qt::NoPen);
        p->drawPath(shape);

        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(color2, 2));
        p->drawLine(QLineF(spine.pointAt(0.1), spine.pointAt(0.5)));

        p->setBrush(tm->closedNodeDarkColor());
        constexpr auto rec2 = QRectF(-6, -6, 12, 12);
        p->drawEllipse(rec2);

        p->setPen(Qt::NoPen);
        p->setBrush(color1);
        p->drawPolygon(tri);

        if (node->isLink()) {
            p->setBrush(Qt::NoBrush);
            p->setPen(QPen(tm->closedNodeMidlightColor(), 1, Qt::DotLine));
            shape.closeSubpath();
            p->drawPath(shape);
        }
    }

    void paintFile(QPainter* p, const QStyleOptionGraphicsItem *option, const NodeItem* node)
    {
        const auto* tm   = SessionManager::tm();
        const auto shape = node->shape();
        const auto axis  = QLineF(shape.elementAt(2), shape.elementAt(0));

        /// 1. draw spine
        p->setPen(QPen(tm->fileNodeDarkColor(), 4, Qt::SolidLine, Qt::SquareCap));
        p->setBrush(Qt::NoBrush);
        auto spine = axis;
        spine.setLength(spine.length() + 2);
        spine = QLineF(axis.p2(), spine.p2());
        p->drawLine(spine);

        /// 2. draw body
        p->setBrush(tm->fileNodeDarkColor());
        auto sizeColor = tm->fileNodeLightColor();

        if (option->state & QStyle::State_Selected) {
            p->setBrush(tm->fileNodeMidlightColor());
            sizeColor = tm->fileNodeDarkColor();
        } else if (option->state & QStyle::State_MouseOver) {
            p->setBrush(tm->fileNodeMidarkColor());
        }
        p->setPen(node->isLink() ? QPen(tm->fileNodeMidlightColor(), 1, Qt::DotLine) : Qt::NoPen);
        p->drawPath(shape);

        /// 3. draw file size indicator
        const auto axisLen = 1.0 / axis.length();

        const auto lhs = QLineF(shape.elementAt(2), shape.elementAt(1)).normalVector().unitVector();
        const auto rhs = QLineF(shape.elementAt(3), shape.elementAt(2)).normalVector().unitVector();

        const auto lhsDxy = QPointF(lhs.dx(), lhs.dy());
        const auto rhsDxy = QPointF(rhs.dx(), rhs.dy());

        p->setBrush(Qt::NoBrush);

        auto ok = false;
        if (const auto sizel2 = node->data(NodeItem::FileSizeKey).toDouble(&ok); ok && sizel2 > 0.0) {
            const int full = std::floor(sizel2 * 0.1);
            auto t = 0.15;
            auto p1 = axis.pointAt(t);

            for (int i = 0; i < full; ++i) {
                QPainterPath path;
                path.moveTo(p1 + lhsDxy * (i+1.5));
                path.lineTo(p1);
                path.lineTo(p1 + rhsDxy * (i+1.5));

                p->setPen(QPen(sizeColor, i+1, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));
                p->drawPath(path);

                t += (i+4) * axisLen;
                p1 = axis.pointAt(t);
            }
            if (const auto rem = std::fmod(sizel2, 10.0) * 0.1; rem > 0.0) {
                QPainterPath path;
                path.moveTo(p1 + lhsDxy * (full+1.5) * rem);
                path.lineTo(p1);
                path.lineTo(p1 + rhsDxy * (full+1.5) * rem);

                p->setPen(QPen(sizeColor, full+1, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));
                p->drawPath(path);
            }
        }
    }
}

SpreadAnimationData core::spreadWithAnimation(const NodeItem* parent)
{
    Q_ASSERT(parent->parentEdge());
    Q_ASSERT(parent->knot());

    SpreadAnimationData result;

    auto includedNodes = parent->childEdges() | asFilesOrClosedTargetNodes;

    const auto gl = getGuides(parent);

    for (int i = 0; auto* child : includedNodes) {
        while (i < gl.size() && gl[i].norm.isNull()) {
            ++i;
        }
        if (i >= gl.size()) {
            break;
        }

        const auto norm = gl[i].norm;

        /// TODO: node lengths are unique to each File/Folder, and have default
        /// value of 144 for now.  If the edge length is changed by the user, it
        /// is saved in the Model and then in the DB.
        auto childLine = QLineF(parent->pos(), parent->pos() + QPointF(norm.dx(), norm.dy()));
        childLine.setLength(144);
        const auto oldPos = child->scenePos();

        if (const auto newPos = childLine.p2(); oldPos != newPos) {
            result.movement.insert(child, {oldPos, newPos});
        }
        ++i;
    }

    return result;
}

Q_GLOBAL_STATIC(core::Animator, animator)

////////////////
/// RootItem ///
////////////////
RootItem::RootItem()
{
    setRect(QRectF(-12, -12, 24, 24));
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

    const auto* tm = SessionManager::tm();

    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(tm->closedNodeDarkColor(), 5));
    p->setBrush(tm->closedNodeColor());
    p->drawEllipse(boundingRect().adjusted(5, 5, -5, -5));
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
NodeItem::NodeItem()
{
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);

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

    auto* target        = new NodeItem();
    target->_parentEdge = new EdgeItem(source, target);

    if (targetIndex.isValid()) {
        target->setIndex(targetIndex);
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
    Q_ASSERT(_index.isValid());

    const auto* model = _index.model();
    const auto count  = std::min(NODE_CHILD_COUNT, model->rowCount(_index));

    const auto sides = count
        + 1  // for parentEdge()
        + 1; // for knot()

    auto gl = guideLines(this, sides, true);

    QList<NodeData> data;

    for (auto i : std::views::iota(0, count)) {
        const auto index = model->index(i, 0, _index);
        const auto norm  = gl.front().normalVector();

        auto nodeLine = QLineF(pos(), pos() + QPointF(1, 1));
        nodeLine.setLength(144);
        nodeLine.setAngle(norm.angle());

        data.push_back(
            { .index    = index,
              .type     = NodeType::ClosedNode,
              .firstRow = 0,
              .pos      = nodeLine.p2(),
              .length   = 0
            });
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
    Q_ASSERT(!_nodeFlags.testAnyFlag(FileNode));

    _knot->show();
    setNodeFlags(_nodeFlags & NodeFlags(LinkNode) | NodeFlags(OpenNode));

    const auto count = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    for (auto& d : data | views::take(count)) {
        d.edge = createNode(d.index, this);
        scene()->addItem(d.edge->target());
        scene()->addItem(d.edge);
        d.edge->target()->setPos(d.pos);
        d.edge->adjust();
        _childEdges.emplace_back(d.edge);
    }

    updateFirstRow();

    _extra = createNode(QModelIndex(), this);
    scene()->addItem(_extra->target());
    scene()->addItem(_extra);
    _extra->target()->hide();
    _extra->hide();
}

void NodeItem::reload(int start, int end)
{
    Q_UNUSED(end)

    if (_nodeType == NodeType::ClosedNode) {
       open();
       return;
    }

    const auto rowCount = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    if (const int growth = rowCount - _childEdges.size(); growth > 0) {
        NodeVector nodes;
        nodes.reserve(growth);
        for (auto /*[[maybe_unused]]*/ i : std::views::iota(0, growth)) {
            auto* edge = createNode(QModelIndex(), this);
            scene()->addItem(edge->target());
            scene()->addItem(edge);
            nodes.push_back(asNodeItem(edge->target()));
            _childEdges.push_back(edge);
            Q_UNUSED(i)
        }

        if (isOpen()) {
            skipTo(start);
            spread();
        }
        else if (isHalfClosed()) {
            for (const auto* n : nodes) {
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
            //setNodeFlags(HalfClosed);
            spread();
            adjustAllEdges(this);
        }
    }

    if (isOpen()) {
        auto rows = _childEdges
            | asFilesOrClosedTargetNodes
            | asIndexRow
            ;
        const auto lowest  = *std::ranges::min_element(rows);
        const auto highest = *std::ranges::max_element(rows);

        /// only skipt to if the new rows are within range of existing rows; it's
        /// less annonying, and it prevents a reload() from quickly overwriting
        /// SceneStorage::loadScene.
        if (lowest == -1 || (start >= lowest && start <= highest)) {
            skipTo(start);
            adjustAllEdges(this);
        }
    }
}

void NodeItem::onRowsRemoved(int start, int end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)

    if (isClosed()) {
        return;
    }

    auto targetNodes = _childEdges | asTargetNode;

    const auto ghostNodes = static_cast<int>(_childEdges.size()) - _index.model()->rowCount(_index);

    /// 2. destroy excess nodes.
    if (EdgeDeque edges; ghostNodes > 0) {
        for (auto* node : targetNodes) {
            if (!node->index().isValid()) {
                Q_ASSERT(node->isClosed() || node->isFile());
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
            close();
            relayoutParent();
            return;
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
            SessionManager::ss()->saveNode(node);
            node->parentEdge()->adjust();
            rebuilt.pop_front();
        }
    }
    updateFirstRow();
}

void NodeItem::onRowsAboutToBeRemoved(int start, int end)
{
    if (isClosed()) {
        return;
    }

    /// clear animations, if any, to avoid deleting child nodes that are being
    /// animated.
    animator->clearAnimations(this);

    const auto targetNodes = _childEdges | asTargetNode;
    EdgeDeque edges;

    for (auto* node : targetNodes) {
        Q_ASSERT(node->index().isValid());
        if (const auto row = node->index().row(); row >= start && row <= end) {
            if (node->isDir() && !node->isClosed()) {
                node->close();
            }
            SessionManager::ss()->deleteNode(node);
        }
    }
}

void NodeItem::setIndex(const QPersistentModelIndex& index)
{
    Q_ASSERT(index.isValid());

    _index = index;

    if (SessionManager::scene()->isDir(index)) {
        _nodeFlags = NodeType::ClosedNode;
    } else {
        _nodeFlags = NodeType::FileNode;
        const auto size = SessionManager::scene()->fileSize(_index);
        setData(FileSizeKey, size > 0 ? std::log2(size) : 0.0);
    }

    if (SessionManager::scene()->isLink(index)) {
        _nodeFlags |= NodeType::LinkNode;
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

    if (isFile()) {
        side = NODE_CLOSED_DIAMETER + NODE_CLOSED_PEN_WIDTH;
    } else if (isOpen()) {
        side = NODE_OPEN_DIAMETER + NODE_OPEN_PEN_WIDTH;
    } else if (isClosed()) {
        side = NODE_CLOSED_DIAMETER + NODE_CLOSED_PEN_WIDTH;
    } else if (isHalfClosed()) {
        side = NODE_HALF_CLOSED_DIAMETER + NODE_HALF_CLOSED_PEN_WIDTH;
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

    if (isFile()) {
        path = fileNodeShape(this, boundingRect());
    } else if (isOpen()) {
        path.addEllipse(boundingRect());
    } else if (isClosed()) {
        path = closedNodeShape(this, boundingRect());
    } else if (isHalfClosed()) {
        path.addEllipse(boundingRect());
    }

    return path;
}

bool NodeItem::hasOpenOrHalfClosedChild() const
{
    const auto children = _childEdges | asTargetNode;

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
    p->setBrush(isSelected() || (option->state & QStyle::State_MouseOver) ?
        tm->openNodeMidlightColor() : tm->openNodeColor());

    qreal radius = 0;
    if (isFile()) {
        paintFile(p, option, this);
    } else if (isOpen()) {
        radius = rec.width() * 0.5 - NODE_OPEN_PEN_WIDTH * 0.5;
        p->setPen(QPen(tm->openNodeLightColor(), NODE_OPEN_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
    } else if (isClosed()) {
        paintClosedFolder(p, option, this);
    } else if (isHalfClosed()) {
        radius = rec.width() * 0.5 - NODE_HALF_CLOSED_PEN_WIDTH * 0.5;
        p->setPen(QPen(tm->closedNodeDarkColor(), NODE_HALF_CLOSED_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
        p->setPen(QPen(tm->openNodeLightColor(), NODE_HALF_CLOSED_PEN_WIDTH, Qt::SolidLine));
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
    if (auto *pr = asNodeItem(parentEdge()->source()); pr && pr->isHalfClosed()) {
        /// need to update the half-closed parent to avoid tearing of the
        /// 20-degree arc. A node can be Open, Closed, or Half-Closed and have
        /// a parent that's HalfClosed, so this update is needed for all.
        pr->update();
    }
}

void NodeItem::close()
{
    Q_ASSERT(isDir() && (isOpen() || isHalfClosed()));
    Q_ASSERT(!_nodeFlags.testAnyFlag(FileNode));

    _knot->hide();
    animator->clearAnimations(this);

    destroyChildren();
    setNodeFlags(_nodeFlags & NodeFlags(LinkNode) | NodeFlags(ClosedNode));

    shrink(this);
}

void NodeItem::halfClose()
{
    Q_ASSERT(hasOpenOrHalfClosedChild());
    Q_ASSERT(!_nodeFlags.testAnyFlag(FileNode));

    _knot->hide();
    setAllEdgeState(this, EdgeItem::CollapsedState);
    setNodeFlags(_nodeFlags & NodeFlags(LinkNode) | NodeFlags(HalfClosedNode));

    adjustAllEdges(this);

    SessionManager::ss()->saveNode(this);
}

void NodeItem::closeOrHalfClose(bool forceClose)
{
    Q_ASSERT(isDir() && (isOpen() || isHalfClosed()));

    if (hasOpenOrHalfClosedChild()) {
        /// pick half closing as it is less destructive, unless the user is
        /// force closing.
        if (forceClose) {
            close();
            relayoutParent();
        } else {
            halfClose();
        }
    } else {
        close();
        relayoutParent();
    }
}

void NodeItem::open()
{
    Q_ASSERT(fsScene()->isDir(_index));
    Q_ASSERT(!_nodeFlags.testAnyFlag(FileNode));

    if (isClosed()) {
        Q_ASSERT(_childEdges.empty());
        extend(this);
        createChildNodes();
        spread();
        adjustAllEdges(this);
        fsScene()->fetchMore(_index);

        Q_ASSERT(std::ranges::all_of(_childEdges | asTargetNodeIndex,
            &QPersistentModelIndex::isValid));

    } else if (isHalfClosed()) {
        setNodeFlags(_nodeFlags & NodeFlags(LinkNode) | NodeFlags(OpenNode));
        spread();
        setAllEdgeState(this, EdgeItem::ActiveState);
        adjustAllEdges(this);
    }

    SessionManager::ss()->saveNode(this);
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
            SessionManager::ss()->saveNode(this);
            break;

        case ItemSelectedChange:
            Q_ASSERT(value.canConvert<bool>());
            if (value.toBool()) { setZValue(1); } else { setZValue(0); }

            if (isFile()) {
                const auto size = fsScene()->fileSize(_index);
                setData(FileSizeKey, size > 0 ? std::log2(size) : 0.0);
            }
            break;

        default:
            break;
    };

    return QGraphicsItem::itemChange(change, value);
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
                if (auto* node = asNodeItem(edge->target()); node->isOpen() || node->isHalfClosed()) {
                    node->spread();
                }
            }
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void NodeItem::setNodeFlags(const NodeFlags flags)
{
    if (_nodeFlags != flags) {
        prepareGeometryChange();
        _nodeFlags = flags;
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
    /// QGraphicsScene will remove an item from the scene upon delete, but
    /// "it is more efficient to remove the item from the QGraphicsScene
    /// before destroying the item." -- Qt docs

    auto destroyEdge = [this](EdgeItem* edge) {
        auto* node = edge->target();

        Q_ASSERT(scene()->items().contains(node));
        Q_ASSERT(scene()->items().contains(edge));

        scene()->removeItem(node);
        scene()->removeItem(edge);
        delete node;
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
    _firstRow = -1;

    destroyEdge(_extra);
    _extra = nullptr;

    /// removes this directory and all the child file/directories from the
    /// Nodes table.
    SessionManager::ss()->deleteNode(this);
}

/// finds the row number of the first child node that is either a file or a
/// closed folder.
/// This is only used in storage when loading the scene to reposition the child
/// nodes at the last-saved start position.
void NodeItem::updateFirstRow()
{
    _firstRow = -1;

    if (auto rows = _childEdges | asFilesOrClosedTargetNodes | asIndexRow; !rows.empty()) {
        _firstRow = *rows.begin();
    }

    SessionManager::ss()->saveNode(this);
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

    if (isHalfClosed()) {
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
    if (fileOrClosedIndices.empty()) {
        _firstRow = -1;
        return;
    }

    const auto usedRows = allButClosedEdge
        | asTargetNode | asIndexRow
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
                sortByRows(_childEdges);
                updateFirstRow();
                return;
            }
        }
        gap = std::ranges::adjacent_find(std::next(gap), fileOrClosedIndices.end(), isGap);
    }

    for (int k = 0, i = -1; k < NODE_CHILD_COUNT; ++k, --i) {
        if (auto before = first->sibling(first->row() + i, 0);
            before.isValid() && !usedRows.contains(before.row())) {
            assignIndex(closed, before);
            sortByRows(_childEdges);
            updateFirstRow();
            return;
        }
    }
    for (int k = 0, i = 1; k < NODE_CHILD_COUNT; ++k, ++i) {
        if (auto after = last->sibling(last->row() + i, 0);
            after.isValid() && !usedRows.contains(after.row())) {
            assignIndex(closed, after);
            sortByRows(_childEdges);
            updateFirstRow();
            return;
        }
    }
    Q_ASSERT(false);
}

/// performs CCW or CW rotation.
/// CW means going forward (i.e., the new node index is greater than the
/// previous).
InternalRotationAnimationData NodeItem::doInternalRotation(Rotation rot)
{
    auto targetNodes = _childEdges
        | asFilesOrClosedTargetNodes
        | ranges::to<std::deque>();

    if (targetNodes.empty()) {
        _firstRow = -1;
        return {};
    }

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

    if (candidates.empty()) {
        return {};
    }

    auto sibling = candidates.front();
    asNodeItem(_extra->target())->setIndex(sibling);
    _extra->setText(asNodeItem(_extra->target())->name());

    EdgeItem* toGrow   = nullptr;
    EdgeItem* toShrink = nullptr;

    const EdgeItem* toErase   = targetNodes.front()->parentEdge();
    const EdgeItem* insertPos = targetNodes.back()->parentEdge();

    QHash<QGraphicsItem*, qreal> angularDisplacements;
    QHash<QGraphicsItem*, qreal> angles;
    for (int i = 1; i < std::ssize(targetNodes); ++i) {
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
        _childEdges.insert(rot==Rotation::CW ? std::next(found) : found, _extra);
        _extra = nullptr;
    } else { Q_ASSERT(false); }

    if (auto found = ranges::find(_childEdges, toErase); found != _childEdges.end()) {
        _extra = *found;
        asNodeItem(_extra->target())->_index = QModelIndex();
        _childEdges.erase(found);
        toShrink = _extra;
    } else { Q_ASSERT(false); }
    Q_ASSERT(isFileOrClosed(_extra));
    updateFirstRow();

    return { rot, this, toGrow, toShrink, angularDisplacements, angles };
}

void NodeItem::skipTo(int row)
{
    const auto rowCount = _index.model()->rowCount(_index);

    Q_ASSERT(std::ssize(_childEdges) <= rowCount);

    auto availableNodes = _childEdges | asFilesOrClosedTargetNodes;

    if (availableNodes.empty()) {
        _firstRow = -1;
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
    } while (std::ssize(newIndices) < rows);

    target = _index.model()->index(row - 1, 0, _index);

    while (std::ssize(newIndices) < rows) {
        if (!target.isValid()) { break; }
        if (openOrHalfClosedRows.contains(target.row())) {
            target = target.sibling(target.row() - 1, 0);
            continue;
        }
        newIndices.push_front(target);
        target = target.sibling(target.row() - 1, 0);
    }

    Q_ASSERT(std::ssize(newIndices) == rows);

    for (auto* node : availableNodes) {
        if (auto newIndex = newIndices.front(); node->index() != newIndex) {
            node->setIndex(newIndex);
            node->parentEdge()->setText(node->name());
        }
        newIndices.pop_front();
    }
    updateFirstRow();
}

/// dxy is used only for the node that is being moved by the mouse.  Without
/// dxy, the child nodes get scattered all over when the parent node is moved
/// too quickly or rapidly.
void NodeItem::spread(const QPointF& dxy)
{
    if (_childEdges.empty()) { return; }

    const auto* grabber = scene()->mouseGrabberItem();
    auto includedNodes = _childEdges | asFilesOrClosedTargetNodes;

    const auto guides = getGuides(this);

    for (int i = 0; auto* node : includedNodes) {
        while (i < guides.size() && guides[i].norm.isNull()) {
            ++i;
        }
        if (i >= guides.size()) {
            break;
        }

        const auto norm = guides[i].norm;

        auto nodeLine = QLineF(pos(), pos() + QPointF(norm.dx(), norm.dy()));
        nodeLine.setLength(144);
        node->setPos(nodeLine.p2() + dxy);
        i++;
    }
}

void NodeItem::spread(const NodeItem* child)
{

    Q_ASSERT(child);
    Q_ASSERT(!_childEdges.empty());

    auto includedNodes = _childEdges
        | asFilesOrClosedTargetNodes
        | views::filter([child](const NodeItem* node) {
            return node != child;
        });

    const auto guides = getGuides(this, child);

    for (int i = 0; auto* node : includedNodes) {
        while (i < guides.size() && guides[i].norm.isNull()) {
            ++i;
        }
        if (i >= guides.size()) {
            break;
        }

        const auto norm = guides[i].norm;

        auto nodeLine = QLineF(pos(), pos() + QPointF(norm.dx(), norm.dy()));
        nodeLine.setLength(144);
        node->setPos(nodeLine.p2());
        i++;
    }
}

void NodeItem::relayoutParent() const
{
    if (auto* parentNode = asNodeItem(_parentEdge->source())) {
        animator->animateRelayout(parentNode, _parentEdge);
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
    for (auto* child : node->childEdges() | asTargetNode) {
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
        const auto data = _animData[va].value<InternalRotationAnimationData>();

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

#ifdef TEST_ANIMATIONS
        const auto fast  = 1;
#else
        const auto len   = count - head;
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

void Animator::interpolate(qreal t, const InternalRotationAnimationData& data)
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

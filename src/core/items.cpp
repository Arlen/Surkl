#include "items.hpp"
#include "Scene.hpp"

#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QVariantAnimation>

#include <numbers>
#include <ranges>
#include <stack>
#include <unordered_set>


using namespace core;

namespace
{
    constexpr qreal GOLDEN = 1.0 / std::numbers::phi;
    /// These don't change, because the scene view offers zoom feature.
    constexpr qreal INODE_OPEN_RADIUS           = 32.0;
    constexpr qreal INODE_OPEN_DIAMETER         = INODE_OPEN_RADIUS * 2.0;
    //constexpr qreal INODE_CLOSED_RADIUS       = INODE_OPEN_RADIUS * GOLDEN;
    constexpr qreal INODE_CLOSED_DIAMETER       = INODE_OPEN_DIAMETER * GOLDEN;
    constexpr qreal INODE_HALF_CLOSED_DIAMETER  = INODE_OPEN_DIAMETER * (1.0 - GOLDEN*GOLDEN*GOLDEN);
    constexpr qreal INODEEDGE_WIDTH             = 4.0;
    constexpr qreal INODEEDGE_COLLAPSED_LEN     = INODE_HALF_CLOSED_DIAMETER;
    constexpr qreal INODE_OPEN_PEN_WIDTH        = 4.0;
    constexpr qreal INODE_CLOSED_PEN_WIDTH      = INODEEDGE_WIDTH * GOLDEN;
    constexpr qreal INODE_HALF_CLOSED_PEN_WIDTH = INODE_OPEN_PEN_WIDTH * (1.0 - GOLDEN*GOLDEN*GOLDEN);

    /// fixed for now.
    constexpr int NODE_CHILD_COUNT = 12;

    /// ---- These will need to go into theme.[hpp,cpp]
    QFont inodeFont() { return {"Adwaita Sans", 11}; }
    QColor inodeColor() { return {41, 117, 156, 255}; }
    QColor inodeEdgeColor() { return {8, 8, 8, 255}; }
    QColor inodeEdgeTextColor() { return {220, 220, 220, 255}; }
    QColor inodeOpenBorderColor() { return {220, 220, 220, 255}; }
    QColor inodeClosedBorderColor() { return {8, 8, 8, 255}; }
    /// ----

    std::deque<QLineF> circle(QLineF line1, int sides, qreal startAngle = 0.0)
    {
        sides = std::max(1, sides);

        const auto anglePerSide = 360.0 / sides;
        auto angle = startAngle;
        auto line2 = line1;

        std::deque<QLineF> result;
        for (int i = 0; i < sides; ++i, angle += anglePerSide) {
            line1.setAngle(angle);
            line2.setAngle(angle + anglePerSide);
            result.emplace_back(line2.p2(), line1.p2());
        }

        return result;
    }

    Inode* asInode(QGraphicsItem* item)
    {
        Q_ASSERT(item != nullptr);

        return qgraphicsitem_cast<Inode*>(item);
    }

    SharedVariantAnimation getVariantAnimation()
    {
        auto animation = SharedVariantAnimation::create();
        animation->setDuration(250);
        animation->setStartValue(0.0);
        animation->setEndValue(1.0);
        animation->setLoopCount(1);
        //animation->setEasingCurve(QEasingCurve::Linear);
        animation->setEasingCurve(QEasingCurve::OutSine);

        return animation;
    }

    bool isRoot(QGraphicsItem* node)
    {
        return qgraphicsitem_cast<RootNode*>(node) != nullptr;
    }

    /// TODO rename
    std::vector<std::pair<QGraphicsItem*, QPointF>> getAncestorPos(Inode* node)
    {
        Q_ASSERT(node != nullptr);
        Q_ASSERT(!isRoot(node));

        std::vector<std::pair<QGraphicsItem*, QPointF>> result;
        auto* parent = node->parentEdge()->source();

        while (!isRoot(parent)) {
            result.emplace_back(parent, parent->mapToScene(parent->boundingRect().center()));
            parent = asInode(parent)->parentEdge()->source();
        }
        result.emplace_back(parent, parent->mapToScene(parent->boundingRect().center()));
        return result;
    }

    std::deque<QPersistentModelIndex> gatherIndices(const QPersistentModelIndex& start, int count,
        const std::unordered_set<int>& excludedRows)
    {
        using namespace std;

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
}


//////////////////////
/// InodeEdgeLabel ///
//////////////////////
InodeEdgeLabel::InodeEdgeLabel(QGraphicsItem* parent)
    : QGraphicsSimpleTextItem(parent)
{
    setFont(inodeFont());
    setPen(Qt::NoPen);
    setBrush(inodeEdgeTextColor());
}

void InodeEdgeLabel::alignToAxis(const QLineF& axis)
{
    alignToAxis(axis, _text);
}

/// sets this label's text so that it fits within length the of given segment.
/// computes a normal that is perpendicular to the segment, and it marks the
/// start of this InodeEdgeLabel's shape().
void InodeEdgeLabel::alignToAxis(const QLineF& axis, const QString& newText)
{
    _axis = axis;
    _text = newText;

    auto opt = QTextOption(Qt::AlignVCenter | Qt::AlignRight);
    opt.setWrapMode(QTextOption::NoWrap);

    const auto fmt      = QFontMetricsF(font());
    const auto length   = axis.length();
    const auto advance  = fmt.horizontalAdvance(newText, opt);
    const auto leftSide = axis.angle() >= 90 && axis.angle() <= 270;
    const auto pt1      = leftSide
        ? axis.p2()
        : axis.pointAt(std::max(0.0, 1.0 - advance / length));

    if (length >= advance) {
        setText(newText);
    } else {
        setText(fmt.elidedText(newText, Qt::ElideRight, length));
    }

    const auto h   = boundingRect().height() * 0.5;
    const auto uv  = axis.normalVector().unitVector();
    const auto vec = QPointF(uv.dx(), uv.dy());
    const auto a   = pt1 - vec * h;
    const auto b   = pt1 + vec * h;

    _normal = QLineF(b, a);
}

/// This simpler version is used when there is no label animation, or when
/// the label animation is running (i.e., updatePosCCW and updatePosCW are
/// being called), but the position needs to be updated using the current
/// t value b/c an Inode was moved.
void InodeEdgeLabel::updatePos()
{
    if (_t == 0) {
        /// only update when _t == zero to avoid flickering when fade in/out
        /// animation (i.e., updatePosCCW/CW) is playing.
        const auto left = _axis.angle() >= 90 && _axis.angle() <= 270;
        const auto p1   = normal().p1();
        const auto p2   = normal().p2();

        const auto pathOfMovement = left ? QLineF(p2, p1) : QLineF(p1, p2);

        setPos(pathOfMovement.pointAt(0));
        setScale(left ? -1 : 1);
        setRotation(-_axis.angle());
    }
}

void InodeEdgeLabel::updatePosCW(qreal t, LabelFade fade)
{
    _t = t;
    const auto left    = _axis.angle() >= 90 && _axis.angle() <= 270;
    const auto p1      = normal().p1();
    const auto p2      = normal().p2();
    const auto dxy     = QPointF(normal().dx(), normal().dy());
    const auto p1Local = mapFromScene(p1);
    const auto p2Local = mapFromScene(p2);

    QLineF pathOfMovement;
    if (fade == LabelFade::FadeOut) {
        pathOfMovement = left ? QLineF(p2, p1)       : QLineF(p1, p1 - dxy);
    } else {
        pathOfMovement = left ? QLineF(p2 + dxy, p2) : QLineF(p2, p1);
    }

    setPos(pathOfMovement.pointAt(t));
    setScale(left ? -1 : 1);
    setRotation(-_axis.angle());
    setGradient(p2Local, p1Local, fade, 1.0 - t);
}

void InodeEdgeLabel::updatePosCCW(qreal t, LabelFade fade)
{
    _t = t;
    const auto left    = _axis.angle() >= 90 && _axis.angle() <= 270;
    const auto p1      = normal().p1();
    const auto p2      = normal().p2();
    const auto dxy     = QPointF(normal().dx(), normal().dy());
    const auto p1Local = mapFromScene(p1);
    const auto p2Local = mapFromScene(p2);

    QLineF pathOfMovement;
    if (fade == LabelFade::FadeOut) {
        pathOfMovement = left ? QLineF(p2, p2 + dxy) : QLineF(p1, p2);
    } else {
        pathOfMovement = left ? QLineF(p1, p2)       : QLineF(p1 - dxy, p1);
    }

    setPos(pathOfMovement.pointAt(t));
    setScale(left ? -1 : 1);
    setRotation(-_axis.angle());
    setGradient(p1Local, p2Local, fade, 1.0 - t);
}

void InodeEdgeLabel::setGradient(const QPointF& a, const QPointF& b, LabelFade fade, qreal t01)
{
    auto gradient = QLinearGradient(a, b);

    if (fade == LabelFade::FadeOut) {
        gradient.setColorAt(qBound(0.0, t01-0.05, 1.0), inodeEdgeTextColor());
        gradient.setColorAt(qBound(0.0, t01,      1.0), QColor(0, 0, 0, 0));
    } else {
        gradient.setColorAt(qBound(0.0, t01+0.05, 1.0), inodeEdgeTextColor());
        gradient.setColorAt(qBound(0.0, t01,      1.0), QColor(0, 0, 0, 0));
    }
    setBrush(QBrush(gradient));
}


/////////////////
/// InodeEdge ///
/////////////////
InodeEdge::InodeEdge(QGraphicsItem* source, QGraphicsItem* target)
    : _source(source)
    , _target(target)
{
    Q_ASSERT(source != nullptr);
    Q_ASSERT(target != nullptr);

    _currLabel = new InodeEdgeLabel(this);
    _nextLabel = new InodeEdgeLabel(this);
    _nextLabel->hide();

    setAcceptedMouseButtons(Qt::LeftButton);
    setFlags(ItemIsSelectable);
    setCursor(Qt::ArrowCursor);

    /// the color doesn't matter b/c it's set in paint(), but need to set the
    /// size so the boundingRect() produces the correct sized QRect.
    setPen(QPen(Qt::red, INODEEDGE_WIDTH, Qt::SolidLine, Qt::SquareCap));
}

void InodeEdge::setName(const QString& name) const
{
    Q_ASSERT(_currLabel->isVisible());
    Q_ASSERT(!_nextLabel->isVisible());
    _currLabel->alignToAxis(line(), name);
}

void InodeEdge::adjust()
{
    Q_ASSERT(scene());

    const auto recA     = source()->boundingRect();
    const auto recB     = target()->boundingRect();
    const auto centerA  = recA.center();
    const auto centerB  = recB.center();
    const auto pA       = mapFromItem(source(), centerA);
    const auto pB       = mapFromItem(target(), centerB);
    const auto segment  = QLineF(pA, pB);
    const auto diameter = (recA.width() + recB.width()) / 2;

    if (_state == CollapsedState) {
        /// the edge becomes a tick mark indicator when the source node is
        /// half-closed and the target node is closed.
        auto tick = QLineF(pA, pB);
        tick.setLength(INODEEDGE_COLLAPSED_LEN);
        setLine(QLineF(tick.pointAt(0.4), tick.pointAt(0.6)));
        return;
    }

    setLine(QLineF());

    /// line from the very edge of an Inode to the very edge of the other
    /// Inode, while accounting for the pen width.
    if (const auto len = segment.length(); len > diameter) {
        const auto lenInv    = 1.0 / len;
        const auto edgeWidth = INODEEDGE_WIDTH * lenInv * 0.5;
        const auto t1 = recA.width() * 0.5 * lenInv + edgeWidth;
        const auto t2 = 1.0 - (recB.width() * 0.5 * lenInv + edgeWidth);
        const auto p1 = segment.pointAt(t1);
        const auto p2 = segment.pointAt(t2);

        setLine(QLineF(p1, p2));

        _currLabel->alignToAxis(line());
        _nextLabel->alignToAxis(line());
        _currLabel->updatePos();
        _nextLabel->updatePos();
    }
}

void InodeEdge::paint(QPainter *p, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    Q_UNUSED(widget);

    if (line().isNull()) {
        /// nodes are too close to draw any edges.
        return;
    }

    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(option->state & QStyle::State_Selected
            ? inodeEdgeColor().lighter(1600)
            : inodeEdgeColor()
        , INODEEDGE_WIDTH, Qt::SolidLine, Qt::SquareCap));
    p->drawLine(line());

    if (_state == CollapsedState) {
        return;
    }

    const auto p1 = line().p1();
    const auto uv = line().unitVector();
    const auto v2 = QPointF(uv.dx(), uv.dy()) * 2.0;

    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(inodeOpenBorderColor(), INODEEDGE_WIDTH, Qt::SolidLine, Qt::SquareCap));
    p->drawLine(QLineF(p1, p1 + v2));
}

/// CollapsedState is only used when a source node is in HalfClosed state. The
/// target node is disabled and hidden, but the edge remains visible and
/// disabled. The edge is set up as a tick mark for a HalfClosed source node,
/// and this set up takes place in InodeEdge::adjust().
///
/// NOTE: visibility of nextLabel() is not and should not be set here;
/// otherwise, the assertion in animateRotation() will fail. i.e., the
/// visibility of nextLabel() is handled in animateRotation().
void InodeEdge::setState(State state)
{
    Q_ASSERT(_state != state);
    _state = state;

    auto toggleMembers = [this](bool onOff)
    {
        setEnabled(onOff);
        target()->setEnabled(onOff);

        currLabel()->setVisible(onOff);
        target()->setVisible(onOff);
    };

    switch (_state) {
    case ActiveState:
        setVisible(true);
        toggleMembers(true);
        break;

    case InactiveState:
        setVisible(false);
        toggleMembers(false);
        break;

    case CollapsedState:
        setVisible(true);
        toggleMembers(false);
        break;
    }
}

QPainterPath InodeEdge::shape() const
{
    Q_ASSERT(pen().width() > 0);

    QPainterPath path;

    if (line() == QLineF())
        return path;

    path.moveTo(line().p1());
    path.lineTo(line().p2());

    QPainterPathStroker ps;
    ps.setCapStyle(pen().capStyle());
    ps.setWidth(pen().widthF());
    ps.setJoinStyle(pen().joinStyle());
    ps.setMiterLimit(pen().miterLimit());

    return ps.createStroke(path);
}



////////////////
/// RootNode ///
////////////////
RootNode::RootNode(QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
{
    setRect(QRectF(-12, -12, 24, 24));
    setPen(Qt::NoPen);
    setBrush(inodeEdgeColor());
    setFlags(ItemIsSelectable | ItemIsMovable |  ItemSendsScenePositionChanges);
}

void RootNode::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
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

QVariant RootNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case ItemScenePositionHasChanged:
        if (auto* pe = qgraphicsitem_cast<InodeEdge*>(parentItem())) {
            pe->adjust();
        }
        break;

    default:
        break;
    };

    return QGraphicsEllipseItem::itemChange(change, value);
}



/////////////////////
/// NewFolderNode ///
/////////////////////
NewFolderNode::NewFolderNode(QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
{
    setRect(QRectF(-12, -12, 24, 24));
    setPen(Qt::NoPen);
    setBrush(inodeEdgeColor());
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
}

void NewFolderNode::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
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

    p->setPen(QPen(bg, 4));
    p->setBrush(Qt::NoBrush);
    p->drawLine(QLineF(rec.center(), rec.center() + QPointF( 6,  0)));
    p->drawLine(QLineF(rec.center(), rec.center() + QPointF( 0,  6)));
    p->drawLine(QLineF(rec.center(), rec.center() + QPointF(-6,  0)));
    p->drawLine(QLineF(rec.center(), rec.center() + QPointF( 0, -6)));
}

QVariant NewFolderNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case ItemPositionChange:
        if (!isSelected()) {
            /// _parentEdge->source() is moving, and this NewFolderNode needs
            /// to stay at -45 degrees and away from it.
            /// If it is selected, then this NewFolderNode is being moved by
            /// the user, and it needs to stay under the mouse wherever it
            /// goes, in that case, don't do anything special.
            return value.toPointF() + QPointF(48, 48);
        }
        break;

    case ItemScenePositionHasChanged:
        _parentEdge->adjust();
        break;

    default:
        break;
    };

    return QGraphicsEllipseItem::itemChange(change, value);
}



/// animation functions
void core::animateRotation(const QVariantAnimation* animation, const EdgeStringMap& input)
{
    auto startCW    = [](InodeEdge* edge, const QString& text)
    {
        Q_ASSERT(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line(), text);
        Q_ASSERT(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->updatePosCW(0, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCW(0, LabelFade::FadeIn);
        edge->nextLabel()->show();
    };
    auto progressCW = [](InodeEdge* edge, qreal t)
    {
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line());
        edge->currLabel()->updatePosCW(t, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCW(t, LabelFade::FadeIn);
    };
    auto finishCW   = [](InodeEdge* edge)
    {
        edge->nextLabel()->updatePosCW(0, LabelFade::FadeOut);
        edge->currLabel()->hide();
        edge->swapLabels();
    };

    auto startCCW    = [](InodeEdge* edge, const QString& text)
    {
        Q_ASSERT(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line(), text);
        Q_ASSERT(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->updatePosCCW(0, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCCW(0, LabelFade::FadeIn);
        edge->nextLabel()->show();
    };
    auto progressCCW = [](InodeEdge* edge, qreal t)
    {
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line());
        edge->currLabel()->updatePosCCW(t, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCCW(t, LabelFade::FadeIn);
    };
    auto finishCCW   = [](InodeEdge* edge)
    {
        edge->nextLabel()->updatePosCCW(0, LabelFade::FadeOut);
        edge->currLabel()->hide();
        edge->swapLabels();
    };


    for (auto [edge, sr] : input) {
        const auto [text, rot] = sr;
        if (rot == Rotation::CW) {
            QObject::connect(animation, &QVariantAnimation::stateChanged,
                [edge, text, startCW, finishCW] (QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
                {
                    if (oldState == QAbstractAnimation::Stopped && newState == QAbstractAnimation::Running) {
                        startCW(edge, text);
                    } else if (oldState == QAbstractAnimation::Running && newState == QAbstractAnimation::Paused) {
                        /// This happens when the user triggers another rotation
                        /// while the current animation is still running.  The pause
                        /// comes from 'oldAnimation->pause()'.
                        finishCW(edge);
                    }
                });

            QObject::connect(animation, &QVariantAnimation::valueChanged,
                [edge, progressCW] (const QVariant& value)
                {
                    progressCW(edge, value.toReal());
                });

            QObject::connect(animation, &QVariantAnimation::finished,
                [edge, finishCW]
                {
                    finishCW(edge);
                });
        }
        if (rot == Rotation::CCW) {
            QObject::connect(animation, &QVariantAnimation::stateChanged,
                [edge, text, startCCW, finishCCW] (QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
                {
                    if (oldState == QAbstractAnimation::Stopped && newState == QAbstractAnimation::Running) {
                        startCCW(edge, text);
                    } else if (oldState == QAbstractAnimation::Running && newState == QAbstractAnimation::Paused) {
                        /// This happens when the user triggers another rotation
                        /// while the current animation is still running.  The pause
                        /// comes from 'oldAnimation->pause()'.
                        finishCCW(edge);
                    }
                });

            QObject::connect(animation, &QVariantAnimation::valueChanged,
                [edge, progressCCW] (const QVariant& value)
                {
                    progressCCW(edge, value.toReal());
                });

            QObject::connect(animation, &QVariantAnimation::finished,
                [edge, finishCCW]
                {
                    finishCCW(edge);
                });
        }
    }
}


/////////////
/// Inode ///
/////////////
Inode::Inode(const QPersistentModelIndex& index)
{
    _index = index;
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsScenePositionChanges);
}

Inode* Inode::createRootNode(core::FileSystemScene* scene)
{
    Q_ASSERT(scene);

    auto* root = new RootNode();
    scene->addItem(root);

    auto* node = createNode(scene, root);
    node->setIndex(scene->rootIndex());
    node->parentEdge()->setName(node->name());
    root->setParentItem(node->parentEdge());
    root->setPos(-128, 0);

    return node;
}

Inode* Inode::createNode(QGraphicsScene* scene, QGraphicsItem* parent)
{
    Q_ASSERT(scene);
    Q_ASSERT(parent);
    Q_ASSERT(scene == parent->scene());

    auto* node = new Inode(QModelIndex());
    auto* edge = new InodeEdge(parent, node);

    scene->addItem(node);
    scene->addItem(edge);

    node->_parentEdge = edge;

    return node;
}

Inode::~Inode()
{
    //qDebug() << "Inode::~Inode" << scene()->items().count();
    // qDebug() << "Inode::~Inode:" << name();
    // doClose();
}

void Inode::init()
{
    Q_ASSERT(scene());
    Q_ASSERT(_childEdges.empty());
    Q_ASSERT(_index.isValid());

    const auto count = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    _childEdges.reserve(count);
    for (auto _ : std::views::iota(0, count)) {
        _childEdges.emplace_back(createNode(scene(), this)->parentEdge());
    }
}

void Inode::reload(int start, int end)
{
    if (_state != FolderState::Open) {
        return;
    }

    const auto rowCount = std::min(NODE_CHILD_COUNT, _index.model()->rowCount(_index));

    /// room to grow
    if (_childEdges.size() < rowCount) {
        const auto sizeBefore = _childEdges.size();
        while (_childEdges.size() < rowCount) {
            _childEdges.emplace_back(createNode(scene(), this)->parentEdge());
        }
        const auto sizeAfter = _childEdges.size();
        if (sizeBefore != sizeAfter) {
            spread();
        }
    }

    skipTo(start);
}

void Inode::unload(int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);

    using namespace std;

    auto all = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        ;

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
    if (InodeEdges edges; ghostNodes > 0) {
        edges.reserve(_childEdges.size());
        for (int deletedNodes = 0; auto* node : all) {
            if (deletedNodes >= ghostNodes) {
                edges.push_back(node->parentEdge());
                continue;
            }
            if (!node->index().isValid()) {
                deletedNodes++;
                Q_ASSERT(node->isClosed());

                scene()->removeItem(node);
                delete node;
                scene()->removeItem(node->parentEdge());
                delete node->parentEdge();
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
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter([](const Inode* node) -> bool { return !node->isClosed(); })
        | views::transform(&Inode::index)
        | views::transform(&QPersistentModelIndex::row)
        | ranges::to<std::unordered_set>()
        ;

    auto closedNodes = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(&Inode::isClosed)
        ;

    auto closedIndices = closedNodes | views::transform(&Inode::index);
    if (ranges::all_of(closedIndices, &QPersistentModelIndex::isValid)) {
        return;
    }

    if (const auto count = ranges::distance(closedIndices); count > 0) {
        auto startIndex = _index.model()->sibling(0, 0, _index);
        if (auto betterStart = ranges::find_if(closedIndices, &QPersistentModelIndex::isValid);
            betterStart != closedIndices.end()) {
            startIndex = *betterStart;
        }
        auto rebuilt = gatherIndices(startIndex, count, openOrHalfClosedRows);

        for (auto* node : closedNodes) {
            if (rebuilt.empty()) { break; }
            /// set index even if node has valid index, b/c the oder might
            /// have changed.
            node->setIndex(rebuilt.front());
            node->parentEdge()->setName(node->name());
            rebuilt.pop_front();
        }
    }
}

void Inode::setIndex(const QPersistentModelIndex& index)
{
    _index = index;
}

QString Inode::name() const
{
    Q_ASSERT(_index.isValid());

    return _index.data().toString();
}

QRectF Inode::boundingRect() const
{
    /// closed and half-closed are smaller.
    qreal side = 1.0;

    switch (_state) {
        case FolderState::Open: side = INODE_OPEN_DIAMETER; break;
        case FolderState::Closed: side = INODE_CLOSED_DIAMETER; break;
        case FolderState::HalfClosed: side = INODE_HALF_CLOSED_DIAMETER; break;
    }

    /// half of the pen is drawn inside the shape and the other half is drawn
    /// outside of the shape, so pen-width plus diameter equals the total side length.
    auto rec = QRectF(0, 0, side, side);
    rec.moveCenter(rec.topLeft());

    return rec;
}

QPainterPath Inode::shape() const
{
    QPainterPath path;
    path.addEllipse(boundingRect());

    return path;
}

bool Inode::hasOpenOrHalfClosedChild() const
{
    const auto children = _childEdges
        | std::views::transform(&InodeEdge::target)
        | std::views::transform(&asInode);

    return std::ranges::any_of(children, [](auto* item) -> bool
    {
        return asInode(item)->isOpen() || asInode(item)->isHalfClosed();
    });
}

void Inode::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const auto& rec = boundingRect();
    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(isSelected() ? inodeColor().lighter() : inodeColor());

    qreal radius = 0;
    if (_state == FolderState::Open) {
        radius = rec.width() * 0.5 - INODE_OPEN_PEN_WIDTH * 0.5;
        //p->setPen(QPen(openNodeHighlight(), INODE_OPEN_PEN_WIDTH, Qt::SolidLine));
        p->setPen(QPen(inodeOpenBorderColor(), INODE_OPEN_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
    } else if (_state == FolderState::Closed) {
        radius = rec.width() * 0.5 - INODE_CLOSED_PEN_WIDTH * 0.5;
        p->setPen(QPen(inodeClosedBorderColor(), INODE_CLOSED_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
        p->setPen(QPen(inodeClosedBorderColor(), INODE_CLOSED_PEN_WIDTH * 0.5, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius * 0.8, radius * 0.8);
    } else if (_state == FolderState::HalfClosed) {
        radius = rec.width() * 0.5 - INODE_HALF_CLOSED_PEN_WIDTH * 0.5;
        p->setPen(QPen(inodeClosedBorderColor(), INODE_HALF_CLOSED_PEN_WIDTH, Qt::SolidLine));
        p->drawEllipse(rec.center(), radius, radius);
        p->setPen(QPen(inodeOpenBorderColor(), INODE_HALF_CLOSED_PEN_WIDTH, Qt::SolidLine));
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
    if (auto *pr = asInode(parentEdge()->source()); pr && pr->_state == FolderState::HalfClosed) {
        /// need to update the half-closed parent to avoid tearing of the
        /// 20-degree arc. A node can be Open, Closed, or Half-Closed and have
        /// a parent that's HalfClosed, so this update is needed for all.
        pr->update();
    }
}

void Inode::close()
{
    destroyChildren();
    setState(FolderState::Closed);

    shrink(this);

    if (auto* pr = asInode(_parentEdge->source())) {
        pr->internalRotationAfterClose(_parentEdge);
    }
}

void Inode::halfClose()
{
    Q_ASSERT(hasOpenOrHalfClosedChild());

    setAllEdgeState(this, InodeEdge::CollapsedState);
    setState(FolderState::HalfClosed);
    adjustAllEdges(this);
}

void Inode::closeOrHalfClose(bool forceClose)
{
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

void Inode::open()
{
    if (_state == FolderState::Closed) {
        Q_ASSERT(_childEdges.empty());

        setState(FolderState::Open);
        extend(this);
        init();
        spread();
        skipTo(0);
    } else if (_state == FolderState::HalfClosed) {
        setState(FolderState::Open);
        spread();
        setAllEdgeState(this, InodeEdge::ActiveState);
        adjustAllEdges(this);
    }
}

void Inode::rotate(Rotation rot)
{
    if (_childEdges.empty()) {
        return;
    }

    InternalRotState result{};
    doInternalRotation(rot, result);

    if (result.changes.empty()) {
        /// TODO: process result.status and perform visual indication animation.
        return;
    }
    assert(scene());

    if (!_singleRotAnimation.isNull() && _singleRotAnimation->state() == QAbstractAnimation::Running) {
        _singleRotAnimation->pause();
    }
    _singleRotAnimation = getVariantAnimation();

    animateRotation(_singleRotAnimation.data(), result.changes);

    _singleRotAnimation->start();
}

void Inode::rotatePage(Rotation rot)
{
    if (_childEdges.empty()) {
        return;
    }

    if (!_seqRotAnimation.isNull() && _seqRotAnimation->state() == QAbstractAnimation::Running) {
        _seqRotAnimation->pause();
        _seqRotAnimation.clear();
    }
    _seqRotAnimation = SharedSequentialAnimation::create();

    for (auto _ : _childEdges) {
        InternalRotState result{};
        doInternalRotation(rot, result);

        if (result.changes.empty()) {
            /// TODO: process result.status and perform visual indication animation.
            break;
        }

        auto* animation = new QVariantAnimation();
        /// all animations except the last one are set to [0.0,0.1].
        animation->setStartValue(0.0);
        animation->setEndValue(0.1);
        animation->setEasingCurve(QEasingCurve::OutSine);
        animation->setLoopCount(1);
        animateRotation(animation, result.changes);
        _seqRotAnimation->addAnimation(animation);
    }

    constexpr int totalDuration = 250;
    int duration = totalDuration / 2;
    const int lastAnimation = _seqRotAnimation->animationCount() - 1;
    for (int i = lastAnimation; i >= 0; --i) {
        if (auto* anim = qobject_cast<QVariantAnimation*>( _seqRotAnimation->animationAt(i))) {
            if (i == lastAnimation) {
                anim->setDuration(duration);
                anim->setStartValue(0.0);
                anim->setEndValue(1.0);
                anim->setEasingCurve(QEasingCurve::OutSine);
            } else {
                anim->setDuration(duration);
            }
            duration = qMax(2, duration /= 2);
        }
    }
    _seqRotAnimation->start();
}

QVariant Inode::itemChange(GraphicsItemChange change, const QVariant &value)
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

void Inode::keyPressEvent(QKeyEvent *event)
{
    const auto mod = event->modifiers() == Qt::ShiftModifier;

    if (event->key() == Qt::Key_A) {
        if (mod) { rotatePage(Rotation::CW); }
        else { rotate(Rotation::CW); }
    } else if (event->key() == Qt::Key_D) {
        if (mod) { rotatePage(Rotation::CCW); }
        else { rotate(Rotation::CCW); }
    }
}

void Inode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);

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

void Inode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (scene()->mouseGrabberItem() == this) {
        if (!_ancestorPos.empty()) {
            _ancestorPos.clear();
        }
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void Inode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (scene()->mouseGrabberItem() == this) {
        if (event->modifiers() & Qt::ShiftModifier) {
            if (_ancestorPos.empty()) {
                _ancestorPos = getAncestorPos(this);
            }
            auto currPos = event->scenePos();
            auto lastPos = event->lastScenePos();
            spread(currPos - lastPos);

            for (auto& [node, pos] : _ancestorPos) {
                const auto lastVec = QLineF(lastPos, pos);
                const auto currVec = QLineF(currPos, pos);

                const auto len = lastVec.length();
                const auto dir = currVec.unitVector();

                const auto newPos = currPos + QPointF(dir.dx(), dir.dy()) * len;

                node->setPos(newPos);
                if (auto* inode = asInode(node); inode) {
                    inode->spread(newPos - pos);
                }

                lastPos = pos;
                currPos = newPos;
                pos     = newPos;
            }
        } else {
            /// 1. spread the parent node.
            if (auto* inode = asInode(_parentEdge->source())) {
                inode->spread();
            }
            /// 2. spread this node, which is moving.
            const auto dxy = event->scenePos() - event->lastScenePos();
            spread(dxy);
            /// 3. spread the child nodes.
            for (const auto* edge : _childEdges) {
                if (auto* inode = asInode(edge->target()); !inode->isClosed()) {
                    inode->spread();
                }
            }
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void Inode::setState(FolderState state)
{
    if (_state != state) {
        prepareGeometryChange();
        _state = state;
    }
}

/// recursively destroys all child nodes and edges.
void Inode::destroyChildren()
{
    /// QGraphicsScene will remove an item from the scene upon delete, but
    /// "it is more efficient to remove the item from the QGraphicsScene
    /// before destroying the item." -- Qt docs

    std::stack stack(_childEdges.begin(), _childEdges.end());

    while (!stack.empty()) {
        auto* edge = stack.top(); stack.pop();
        auto* node = asInode(edge->target());
        Q_ASSERT(node);
        for (auto* childEdge : node->childEdges()) {
            Q_ASSERT(childEdge->source() == node);
            auto* childInode = asInode(childEdge->target());
            Q_ASSERT(childInode);
            if (childInode->isOpen() || childInode->isHalfClosed()) {
                stack.emplace(childEdge);
            } else {
                Q_ASSERT(scene()->items().contains(childInode));
                scene()->removeItem(childInode);
                delete childInode;
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

void Inode::internalRotationAfterClose(InodeEdge* closedEdge)
{
    const auto result = doInternalRotationAfterClose(closedEdge);

    if (result.changes.empty()) {
        return;
    }
    assert(scene());

    if (!_singleRotAnimation.isNull() && _singleRotAnimation->state() == QAbstractAnimation::Running) {
        _singleRotAnimation->pause();
    }
    _singleRotAnimation = getVariantAnimation();

    animateRotation(_singleRotAnimation.data(), result.changes);

    _singleRotAnimation->start();
}

/// This is basically a sorting of all closed edges.
/// NOTE: after a few open/close and rotations, there will be gaps in between
/// edges, b/c we don't perform any kind of compacting, and that's okay.  The
/// user may not want the edge they just closed to change and point to a
/// different "folder".  Instead, we can offer the user an option to perform
/// the compacting whenever they want.
/// Inode::rotate partially solves the problem by continuing to rotate until
/// there is no gap between two edges.
InternalRotState Inode::doInternalRotationAfterClose(InodeEdge* closedEdge)
{
    Q_ASSERT(asInode(closedEdge->target())->isClosed());
    Q_ASSERT(asInode(closedEdge->target())->index().isValid());

    using namespace std;

    auto closedIndices = _childEdges
        | views::filter([closedEdge](InodeEdge* edge) -> bool
            { return edge != closedEdge; })
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(&Inode::isClosed)
        | views::transform(&Inode::index)
        | ranges::to<std::deque>()
        ;

    if (closedIndices.empty()) { return {}; }

    const auto openOrHalfClosedRows = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter([](const Inode* node) -> bool { return !node->isClosed(); })
        | views::transform(&Inode::index)
        | views::transform(&QPersistentModelIndex::row)
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
    auto result = InternalRotState{};

    auto closedNodes = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(&Inode::isClosed)
        ;

    Q_ASSERT(std::ranges::all_of(closedIndices, &QPersistentModelIndex::isValid));
    Q_ASSERT(ranges::distance(closedNodes) == closedIndices.size());

    for (int k = 0; auto* node : closedNodes) {
        auto newIndex = closedIndices[k++];
        Q_ASSERT(newIndex.isValid());
        auto oldIndex = node->index();
        if (newIndex == oldIndex) { continue; }
        node->setIndex(newIndex);
        result.changes[node->parentEdge()] =
            { node->name()
            , newIndex.row() > oldIndex.row() ? Rotation::CCW : Rotation::CW};
    }

    return result;
}

/// performs CCW or CW rotation.
/// CCW means going forward (i.e., the new inode index is greater than the
/// previous).
void Inode::doInternalRotation(Rotation rot, InternalRotState& result)
{
    using namespace std;

    auto closedNodes = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(&Inode::isClosed)
        | ranges::to<std::vector>();

    if (closedNodes.empty()) {
        result.status = InternalRotationStatus::MovementImpossible;
        return;
    }

    const auto openOrHalfClosedRows = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter([](const Inode* node) -> bool { return !node->isClosed(); })
        | views::transform(&Inode::index)
        | views::transform(&QPersistentModelIndex::row)
        | ranges::to<std::unordered_set>();

    auto isIndexClosed = [&openOrHalfClosedRows](const QModelIndex& index) -> bool
    {
        return !openOrHalfClosedRows.contains(index.row());
    };

    if (rot == Rotation::CW) { std::ranges::reverse(closedNodes); }
    const auto Inc = rot == Rotation::CCW ? 1 : -1;
    auto lastIndex = closedNodes.back()->index();
    auto sibling   = lastIndex;

    auto candidates = views::iota(1, NODE_CHILD_COUNT)
        | std::views::transform(std::bind(std::multiplies{}, std::placeholders::_1, Inc))
        | views::transform([lastIndex](int i) { return lastIndex.sibling(lastIndex.row() + i, 0); })
        | views::filter(&QModelIndex::isValid)
        | views::filter(isIndexClosed);

    if (candidates.empty()) {
        result.status = rot == Rotation::CCW
            ? InternalRotationStatus::EndReachedCCW
            : InternalRotationStatus::EndReachedCW;
        return;
    }

    sibling = candidates.front();

    Inode* prev = nullptr;
    for (auto* node : closedNodes) {
        if (prev) {
            Q_ASSERT(prev->isClosed());
            Q_ASSERT(prev->index() != node->index());
            prev->setIndex(node->index());
            result.changes[prev->parentEdge()] = {prev->name(), rot};
        }
        prev = node;
    }
    if (prev) {
        prev->setIndex(sibling);
        result.changes[prev->parentEdge()] = {prev->name(), rot};
    }
}

void Inode::skipTo(int row)
{
    InternalRotState result{};
    skipTo(row, result);

    if (result.changes.empty()) {
        return;
    }

    if (!_singleRotAnimation.isNull() && _singleRotAnimation->state() == QAbstractAnimation::Running) {
        _singleRotAnimation->pause();
    }
    _singleRotAnimation = getVariantAnimation();

    animateRotation(_singleRotAnimation.data(), result.changes);

    _singleRotAnimation->start();
}

void Inode::skipTo(int row, InternalRotState& result)
{
    const auto rowCount = _index.model()->rowCount(_index);

    Q_ASSERT(_childEdges.size() <= rowCount);

    using namespace std;

    auto closedNodes = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(&Inode::isClosed)
        ;

    if (closedNodes.empty()) {
        return;
    }

    auto target = _index.model()->index(row, 0, _index);

    if (!target.isValid()) {
        result.status = InternalRotationStatus::MovementImpossible;
        return;
    }

    const auto openOrHalfClosedRows = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter([](const Inode* node) -> bool { return !node->isClosed(); })
        | views::transform(&Inode::index)
        | views::transform(&QPersistentModelIndex::row)
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
            result.changes[node->parentEdge()] = {node->name(), rot};
        }
        newIndices.pop_front();
    }
}

/// dxy is used only for the node that is being moved by the mouse.  Without
/// dxy, the child nodes get scattered all over when the parent node is moved
/// too quickly or rapidly.
void Inode::spread(QPointF dxy)
{
    using namespace std;

    if (_childEdges.empty()) { return; }

    const auto* grabber = scene()->mouseGrabberItem();
    const auto center   = mapToScene(boundingRect().center());
    const auto sides    = _childEdges.size() + (_parentEdge ? 1 : 0);
    auto guides         = circle(QLineF(center, center + QPointF(1, 0)), sides);

    auto intersectsWith = [](const QLineF& line)
    {
        return [line](const QLineF& other) -> bool
        {
            return other.intersects(line) == QLineF::BoundedIntersection;
        };
    };
    auto isIncluded = [grabber](Inode* node) -> bool
    {
        return node->isClosed() && node != grabber;
    };
    auto isExcluded = [grabber](Inode* node) -> bool
    {
        return node->isOpen() || node->isHalfClosed() || node == grabber;
    };

    if (_parentEdge) {
        auto* ps        = _parentEdge->source();
        auto parentEdge = QLineF(center, ps->mapToScene(ps->boundingRect().center()));
        auto removed    = std::ranges::remove_if(guides, intersectsWith(parentEdge));
        guides.erase(removed.begin(), removed.end());
    }

    auto excludedNodes = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(isExcluded)
        ;
    for (auto* node : excludedNodes) {
        const auto nodeLine = QLineF(center, node->mapToScene(node->boundingRect().center()));
        auto ignored = std::ranges::remove_if(guides, intersectsWith(nodeLine));
        guides.erase(ignored.begin(), ignored.end());
    }

    auto includedNodes = _childEdges
        | views::transform(&InodeEdge::target)
        | views::transform(&asInode)
        | views::filter(isIncluded)
        ;
    for (auto* node : includedNodes) {
        if (guides.empty()) {
            break;
        }
        const auto guide = guides.front();
        const auto norm  = guide.normalVector();
        auto nodeLine = QLineF(center, node->mapToScene(node->boundingRect().center()));
        if (nodeLine.p2().isNull() || nodeLine.length() < boundingRect().width()) {
            nodeLine.setLength(144);
        }
        nodeLine.setAngle(norm.angle());
        node->setPos(nodeLine.p2() + dxy);
        guides.pop_front();
    }
}

void core::extend(Inode* inode, qreal distance)
{
    const auto* pe = inode->parentEdge();

    /// pos()/setPos() are in scene coordinates if there is no parent.
    Q_ASSERT(inode->parentItem() == nullptr);
    Q_ASSERT(pe->target() == inode);

    const auto& source = pe->source()->pos();
    const auto& target = pe->target()->pos();
    auto line = QLineF(source, target);
    line.setLength(line.length() + distance);
    inode->setPos(line.p2());
}

void core::shrink(Inode* inode, qreal distance)
{
    const auto* pe = inode->parentEdge();

    /// pos()/setPos() are in scene coordinates if there is no parent.
    Q_ASSERT(inode->parentItem() == nullptr);
    Q_ASSERT(pe->target() == inode);

    const auto& source = pe->source()->pos();
    const auto& target = pe->target()->pos();
    auto line = QLineF(source, target);
    line.setLength(qMax(distance, line.length() - distance));
    inode->setPos(line.p2());
}

void core::adjustAllEdges(const Inode* inode)
{
    inode->parentEdge()->adjust();
    std::ranges::for_each(inode->childEdges(), &InodeEdge::adjust);
}

/// only used on closed nodes, but can be made more general if needed.
void core::setAllEdgeState(const Inode* inode, InodeEdge::State state)
{
    using std::views::transform;
    using std::views::filter;

    for (auto* edge : inode->childEdges()
        | transform(&InodeEdge::target)
        | transform(&asInode)
        | filter(&Inode::isClosed)
        | transform(&Inode::parentEdge))
    {
        edge->setState(state);
    }
}
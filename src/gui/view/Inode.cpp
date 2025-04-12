#include "Inode.hpp"

#include <QDir>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QObject>
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>
#include <QVariantAnimation>
#include <QSequentialAnimationGroup>

#include <numbers>
#include <stack>
#include <ranges>


using namespace gui::view;

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
    constexpr qreal INODE_OPEN_PEN_WIDTH        = 4.0;
    constexpr qreal INODE_CLOSED_PEN_WIDTH      = INODEEDGE_WIDTH * GOLDEN;
    constexpr qreal INODE_HALF_CLOSED_PEN_WIDTH = INODE_OPEN_PEN_WIDTH * (1.0 - GOLDEN*GOLDEN*GOLDEN);

    /// ---- These will need to go into theme.[hpp,cpp]
    QFont inodeFont() { return {"Adwaita Sans", 11}; }
    QColor inodeColor() { return {41, 117, 156, 255}; }
    QLinearGradient openNodeHighlight()
    {
        const auto color = inodeColor();
        const auto high  = color.lighter();
        const auto low   = color.darker();

        QLinearGradient lg(QPointF(0, INODE_OPEN_RADIUS), QPointF(0, -INODE_OPEN_RADIUS));
        lg.setColorAt(0, low);
        lg.setColorAt(1, high);

        return lg;
    }

    QColor inodeEdgeColor() { return {8, 8, 8, 255}; }
    QColor inodeEdgeTextColor() { return {220, 220, 220, 255}; }
    QColor inodeOpenBorderColor() { return {220, 220, 220, 255}; }
    QColor inodeClosedBorderColor() { return {8, 8, 8, 255}; }
    /// ----


    QVector2D unitVector(const QPointF& a, const QPointF& b)
    {
        return QVector2D(b - a).normalized();
    }

    std::vector<QLineF> circle(const QPointF& center, unsigned sides, qreal radius = 1)
    {
        Q_ASSERT(sides > 1);

        const auto sideAngle = 360.0 / sides;
        auto start = sideAngle;
        auto line1 = QLineF(center, center + QPointF(radius, 0));
        auto line2 = line1;

        std::vector<QLineF> result;
        for (unsigned i = 0; i < sides; ++i) {
            line1.setAngle(start);
            line2.setAngle(start+sideAngle);
            result.emplace_back(line2.p2(), line1.p2());

            start += sideAngle;
        }

        return result;
    }

    Inode* asInode(QGraphicsItem* item)
    {
        assert(item != nullptr);
        assert(item->type() == Inode::Type);

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

    void edge_setVisible(InodeEdge* edge, bool visible)
    {
        edge->target()->setVisible(visible);
        edge->currLabel()->setVisible(visible);
        edge->nextLabel()->setVisible(visible);
        edge->setVisible(visible);
    }

    void edge_setEnabled(InodeEdge* edge, bool enabled)
    {
        edge->target()->setEnabled(enabled);
        edge->setEnabled(enabled);
    }

    void edge_enableAndShow(InodeEdge* edge)
    {
        edge_setEnabled(edge, true);
        edge_setVisible(edge, true);
    }

    void edge_disableAndHide(InodeEdge* edge)
    {
        edge_setEnabled(edge, false);
        edge_setVisible(edge, false);
    }

    /// This is only used when a source node is in HalfClose state. The target
    /// node is disabled and hidden, but the edge remains visible and disabled.
    /// The edge is set up as a tick mark for a HalfClosed source node, and
    /// this set up takes place in InodeEdge::adjust().
    void edge_disableAndHideTarget(InodeEdge* edge)
    {
        edge_setEnabled(edge, false);
        edge->target()->setVisible(false);
        edge->currLabel()->setVisible(false);
        edge->nextLabel()->setVisible(false);
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

    if (auto [sn, tn] = std::pair{asInode(source()), asInode(target())};
        sn && tn && sn->isHalfClosed() && tn->isClosed()) {
        /// the edge becomes a tick mark indicator when the source node is
        /// half-closed and the target node is closed.
        auto tick = QLineF(pA, pB);
        tick.setLength(INODE_HALF_CLOSED_DIAMETER);
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

    Q_ASSERT(source() && target());

    if (line().isNull()) {
        /// nodes are too close to draw any edges.
        return;
    }

    p->setRenderHint(QPainter::Antialiasing);
    QGraphicsLineItem::paint(p, option);
    if (const auto isHalfClosed = !target()->isVisible() && !isEnabled(); isHalfClosed) {
        return;
    }

    const auto p1 = line().p1();
    const auto uv = line().unitVector();
    const auto v2 = QPointF(uv.dx(), uv.dy()) * 2.0;

    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(inodeOpenBorderColor(), INODEEDGE_WIDTH, Qt::SolidLine, Qt::SquareCap));
    p->drawLine(QLineF(p1, p1 + v2));
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
void gui::view::animateRotation(const QVariantAnimation* animation, const EdgeStringMap& input)
{
    auto startCW    = [](InodeEdge* edge, const QString& text)
    {
        assert(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line(), text);
        assert(edge->nextLabel()->isVisible() == false);
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
        assert(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line(), text);
        assert(edge->nextLabel()->isVisible() == false);
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
Inode::Inode(const QDir& dir)
{
    setDir(dir);
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsScenePositionChanges);

    auto* nfn      = new NewFolderNode;
    _newFolderEdge = new InodeEdge(this, nfn);
    nfn->setEdge(_newFolderEdge);
}

Inode* Inode::createRoot(QGraphicsScene* scene)
{
    Q_ASSERT(scene);

    auto* inode = new Inode(QDir::root());
    auto* root  = new RootNode();
    auto* edge  = new InodeEdge(root, inode);

    root->setParentItem(edge);
    edge->setName("/");
    edge->setZValue(-1);

    scene->addItem(inode);
    scene->addItem(edge);

    root->setPos(-128, 0);
    inode->_parentEdge = edge;

    Q_ASSERT(inode->newFolderEdge() != nullptr);
    Q_ASSERT(inode->newFolderEdge()->scene() == nullptr);
    scene->addItem(inode->newFolderEdge()->target());
    scene->addItem(inode->newFolderEdge());

    edge_disableAndHide(inode->newFolderEdge());

    return inode;
}

Inode::~Inode()
{
    if (_newFolderEdge) {
        delete _newFolderEdge->target();
        delete _newFolderEdge;
    }
}

void Inode::init()
{
    Q_ASSERT(scene());
    Q_ASSERT(_childEdges.empty());

    const auto entries = _dir.entryInfoList();
    const auto count   = std::min(_winSize, entries.size());

    _childEdges.reserve(count);
    for (qsizetype i = 0; i < count; ++i) {
        auto* node = new Inode(QDir(entries.at(i).absoluteFilePath()));
        auto* edge = new InodeEdge(this, node);
        edge->setName(node->name());

        scene()->addItem(node);
        scene()->addItem(node->newFolderEdge()->target());
        scene()->addItem(node->newFolderEdge());
        scene()->addItem(edge);

        edge_disableAndHide(node->newFolderEdge());

        node->_parentEdge = edge;
        _childEdges.emplace_back(node->_parentEdge, i);
    }

    Q_ASSERT(_newFolderEdge != nullptr);
    Q_ASSERT(_newFolderEdge->scene());

    /// TODO: need proper check based on Folder permissions.
    edge_enableAndShow(_newFolderEdge);
}

void Inode::setDir(const QDir& dir)
{
    _dir = dir;
    _dir.setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
}

QString Inode::name() const
{
    return _dir.dirName();
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
    const auto children = std::views::keys(_childEdges)
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
        if (auto *pr = asInode(parentEdge()->source()); pr && pr->_state == FolderState::HalfClosed) {
            /// need to update the half-closed parent to avoid tearing of the 20-degree arc.
            pr->update();
        }
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
        for (const auto* edge : std::views::keys(_childEdges)) {
            if (edge->target()->isVisible()) {
                const auto startAngle = qRound((edge->line().angle() + 10) * 16.0);
                p->drawArc(rec2, startAngle, span1);
            }
        }
        if (auto *pr = asInode(parentEdge()->source()); pr && pr->_state == FolderState::HalfClosed) {
            /// need to update the half-closed parent to avoid tearing of the 20-degree arc.
            pr->update();
        }
    }
}

void Inode::close()
{
    doClose();
    prepareGeometryChange();
    _state = FolderState::Closed;

    shrink(this);

    if (auto* pr = asInode(_parentEdge->source())) {
        pr->onChildInodeClosed(_parentEdge);
        pr->internalRotationAfterClose(_parentEdge);
    }
}

void Inode::halfClose()
{
    Q_ASSERT(hasOpenOrHalfClosedChild());

    for (auto* edge : std::views::keys(_childEdges)) {
        if (auto* node = asInode(edge->target()); node->isClosed()) {
            edge_disableAndHideTarget(edge);
        }
    }

    prepareGeometryChange();
    _state = FolderState::HalfClosed;
    edge_disableAndHide(_newFolderEdge);
    adjustAllEdges(this);
}

void Inode::open()
{
    if (_state == FolderState::Closed) {
        Q_ASSERT(_childEdges.empty());

        extend(this);
        init();
        prepareGeometryChange();
        _state = FolderState::Open;

        spread();
        adjustAllEdges(this);

        if (auto* pr = asInode(_parentEdge->source())) {
            pr->onChildInodeOpened(_parentEdge);
        }
    } else if (_state == FolderState::HalfClosed) {

        for (auto* edge : std::ranges::views::keys(_childEdges)) {
            if (const auto* node = asInode(edge->target()); node->isClosed()) {
                edge_enableAndShow(edge);
            }
        }

        prepareGeometryChange();
        _state = FolderState::Open;
        edge_enableAndShow(_newFolderEdge);

        spread();
        adjustAllEdges(this);
    }
}

void Inode::rotate(Rotation rot)
{
    if (_childEdges.empty()) {
        return;
    }

    InternalRotState result{};
    const int lastEdge = static_cast<int>(_childEdges.size() - 1);
    doInternalRotation(0, lastEdge, rot, result);

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

    assert(scene());

    const int pageSize = static_cast<int>(_childEdges.size());
    const int lastEdge = static_cast<int>(_childEdges.size() - 1);

    if (!_seqRotAnimation.isNull() && _seqRotAnimation->state() == QAbstractAnimation::Running) {
        _seqRotAnimation->pause();
        _seqRotAnimation.clear();
    }
    _seqRotAnimation = SharedSequentialAnimation::create();

    for (int i = 0; i < pageSize; i++) {
        InternalRotState result{};
        doInternalRotation(0, lastEdge, rot, result);

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
        if (hasOpenOrHalfClosedChild()) {
            /// pick half closing as it is less destructive, unless the user is
            /// Shift closing.
            if (event->modifiers() & Qt::ShiftModifier) {
                close();
            } else {
                halfClose();
            }
        } else {
            close();
        }
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
    if (event->button() == Qt::LeftButton) {
    }

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
            for (const auto* edge : std::ranges::views::keys(_childEdges)) {
                if (auto* inode = asInode(edge->target()); !inode->isClosed()) {
                    inode->spread();
                }
            }
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void Inode::doClose()
{
    auto edges = std::ranges::views::keys(_childEdges);
    std::stack stack(edges.begin(), edges.end());

    while (!stack.empty()) {
        auto* edge = stack.top(); stack.pop();
        auto* node = asInode(edge->target());
        Q_ASSERT(node);
        for (auto* childEdge : std::ranges::views::keys(node->childEdges())) {
            Q_ASSERT(childEdge->source() == node);
            auto* childInode = asInode(childEdge->target());
            Q_ASSERT(childInode);
            if (childInode->isOpen() || childInode->isHalfClosed()) {
                stack.emplace(childEdge);
            } else {
                delete childInode;
                delete childEdge;
            }
        }
        node->_childEdges.clear();
        node->_openEdges.clear();
        delete node;
        delete edge;
    }
    /// no reason to delete _newFolderEdge when we can just disable and hide it.
    edge_disableAndHide(_newFolderEdge);

    _childEdges.clear();
    _openEdges.clear();
}

void Inode::onChildInodeOpened(const InodeEdge* edge)
{
    for (auto [e,i] : _childEdges) {
        if (e == edge) {
            _openEdges.insert(i);
            break;
        }
    }
}

void Inode::onChildInodeClosed(const InodeEdge* edge)
{
    for (auto [e,i] : _childEdges) {
        if (e == edge) {
            _openEdges.erase(i);
            break;
        }
    }
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
    Q_UNUSED(closedEdge); /// remove closeEdge since it's not used. TODO.

    auto result = InternalRotState{};
    InodeEdges xs(_childEdges.size() - _openEdges.size(), {nullptr, -1});

    if (xs.empty()) {
        return result;
    }

    std::ranges::copy_if(_childEdges, xs.begin(),
        [this](const auto& edge)
        {
            return !_openEdges.contains(edge.second);
        });
    std::ranges::sort(xs,
        [](const auto& e1, const auto& e2) { return e1.second < e2.second; });

    int k2 = 0;
    for (int k1 = 0; k1 < std::ssize(_childEdges); ++k1) {
        auto [e, currInodeIndex] = _childEdges.at(k1);
        if (_openEdges.contains(currInodeIndex)) {
            continue;
        }
        const auto newInodeIndex = xs[k2].second;
        if (currInodeIndex != newInodeIndex) {
            result.changes[e] = setEdgeInodeIndex(k1, newInodeIndex);
        }
        ++k2;
    }
    return result;
}

/// rotate the sub-range [begin,end] (both inclusive) in CCW or CW.
/// CCW means going forward (i.e., the new inode index is greater than the
/// previous).
/// Tries to keep going to fill the gaps, instead of aborting once the current
/// index can't be moved.
void Inode::doInternalRotation(int begin, int end, Rotation rot, InternalRotState& result)
{
    assert(begin <= end);
    assert(begin >= 0);
    assert(end < std::ssize(_childEdges));
    assert(result.changes.empty());

    const auto& eil  = _dir.entryInfoList();
    if (eil.isEmpty()) {
        result.status = InternalRotationStatus::MovementImpossible;
        return;
    }

    const auto Next = rot == Rotation::CCW ? -1 :  1;
    const auto Inc  = rot == Rotation::CCW ?  1 : -1;

    // EIL == QDir().entryInfoList()
    /// if moving forward (CCW), begin at _childEdges[end] and move towards
    /// the end of EIL.
    const auto Head =  rot == Rotation::CCW ? end   : begin;
    const auto Tail = (rot == Rotation::CCW ? begin : end) + Next;

    auto prevInodeIndex = rot == Rotation::CCW ? eil.size() : -1;
    for (auto k = Head; k != Tail; k += Next) {
        const auto [edge,currInodeIndex] = _childEdges[k];
        if (_openEdges.contains(currInodeIndex)) {
            /// don't touch open edges.
            continue;
        }
        auto newInodeIndex = currInodeIndex;
        do {
            newInodeIndex += Inc;
            if (std::abs(newInodeIndex - prevInodeIndex) <= 0) {
                prevInodeIndex = currInodeIndex;
                break;
            }
            if (_openEdges.contains(newInodeIndex)) {
                continue;
            }
            prevInodeIndex = newInodeIndex;
            result.status = InternalRotationStatus::Normal;
            result.changes[edge] = setEdgeInodeIndex(k, newInodeIndex);
            break;
        } while (true);
    }
    if (result.changes.empty()) {
        result.status = rot == Rotation::CCW
            ? InternalRotationStatus::EndReachedCCW
            : InternalRotationStatus::EndReachedCW;
    }
}

StringRotation Inode::setEdgeInodeIndex(int edgeIndex, qsizetype newInodeIndex)
{
    assert(!_childEdges.empty() && edgeIndex >= 0 && edgeIndex < std::ssize(_childEdges));

    const auto& eil = _dir.entryInfoList();
    assert(!eil.isEmpty() && newInodeIndex >= 0 && newInodeIndex < eil.size());

    auto& [edge, oldInodeIndex] = _childEdges[edgeIndex];
    const auto rot = newInodeIndex > oldInodeIndex ? Rotation::CCW : Rotation::CW;
    assert(edge != nullptr);
    oldInodeIndex = newInodeIndex;

    auto* target = asInode(edge->target());
    assert(target != nullptr);

    const auto& path = eil.at(newInodeIndex).absoluteFilePath();
    target->setDir(QDir(path));

    return {target->name(), rot};
}

/// dxy is used only for the node that is being moved by the mouse.  Without
/// dxy, the child nodes get scattered all over when the parent node is moved
/// too quickly or rapidly.
void Inode::spread(QPointF dxy)
{
    if (_childEdges.empty()) {
        return;
    }
    Q_ASSERT(_parentEdge);
    Q_ASSERT(_newFolderEdge);

    constexpr qreal radius = 1.0;
    const auto center      = mapToScene(boundingRect().center());

    /// NOTE: guides that interesect with _parentEdge, _newFolderEdge, any open
    /// or halfClosed edges are not used b/c those InodeEdges are not
    /// repositioned during spread.
    /// +2, one for _parentEdge and one for _newFolderEdge
    auto guides = circle(center, _childEdges.size() + 2, radius);

    auto newFolderEdge = QLineF(center, _newFolderEdge->target()->mapToScene(_newFolderEdge->target()->boundingRect().center()));
    if (newFolderEdge.isNull() || newFolderEdge.length() < radius) {
        /// I suppose this can happen when loading from DB and the content of
        /// the DB contains incorrect data.  In which case, set it to some
        /// valid line.
        /// _newFolderEdge is always at a -45 degrees angle.
        newFolderEdge = QLineF(center, center + QPointF(radius * 2, 0));
        newFolderEdge.setAngle(-45);
    }

    /// In CCW order, the first non-intersecting guide edge right after
    /// _newFolderEdge marks the beginning of the guides.  Find guide edge that
    /// intersects with _newFolderEdge, then rotate so that guide[0] is the
    /// beginning.
    auto intersectsNewFolder = [&newFolderEdge](const QLineF& x)
    { return x.intersects(newFolderEdge) == QLineF::BoundedIntersection; };
    if (const auto found = std::ranges::find_if(guides, intersectsNewFolder); found != guides.end()) {
        std::ranges::rotate(guides, std::ranges::next(found));
        guides.pop_back(); // remove guide that intersects _newFolderEdge
    }

    auto parentEdge = QLineF(center, _parentEdge->source()->mapToScene(_parentEdge->source()->boundingRect().center()));
    if (parentEdge.isNull() || parentEdge.length() < radius) {
        /// I suppose this can happen when loading from DB and the content of
        /// the DB contains incorrect data.  In which case, set it to some
        /// valid line.
        parentEdge = QLineF(center, center + QPointF(-radius * 2, 0));
    }

    /// remove guide edges that intersect with parentEdge.
    auto rmv1 = std::ranges::remove_if(guides, [&parentEdge](const QLineF& x)
        { return x.intersects(parentEdge) == QLineF::BoundedIntersection; });
    guides.erase(rmv1.begin(), rmv1.end());

    /// remove guide edges that intersect with open, half-closed or current mouse grabber.
    for (const auto* edge : std::ranges::views::keys(_childEdges)) {
        if (const auto* inode = asInode(edge->target()); inode->isOpen() || inode->isHalfClosed() || scene()->mouseGrabberItem() == inode) {
            const auto ignoredEdge = QLineF(center, inode->mapToScene(inode->boundingRect().center()));
            auto ignored = std::ranges::remove_if(guides, [ignoredEdge](QLineF x)
                { return x.intersects(ignoredEdge) == QLineF::BoundedIntersection; });
            guides.erase(ignored.begin(), ignored.end());
        }
    }

    int gi = 0;
    for (const auto* edge : std::ranges::views::keys(_childEdges)) {
        if (auto* target = asInode(edge->target()); target->isClosed() && target != scene()->mouseGrabberItem()) {
            constexpr qreal prefLen = 144.0;
            Q_ASSERT(gi < guides.size());
            const auto norm = guides[gi].normalVector();
            auto segment = target->pos().isNull()
                ? QLineF(center + dxy, center + dxy + QPointF(prefLen, 0))
                : QLineF(center + dxy, target->mapToScene(target->boundingRect().center()) + dxy);

            segment.setAngle(norm.angle());
            target->setPos(segment.p2());

            ++gi;
        }
    }
}


/// distance is float b/c QVector2D::operator* takes a float.
void gui::view::extend(Inode* inode, float distance)
{
    const auto* pe = inode->parentEdge();

    /// pos()/setPos() are in scene coordinates if there is no parent.
    assert(inode->parentItem() == nullptr);
    assert(pe->target() == inode);

    const auto& source = pe->source()->pos();
    const auto& target = pe->target()->pos();
    const auto dxy     = unitVector(source, target) * distance;

    inode->setPos(target + dxy.toPointF());
}

void gui::view::shrink(Inode* inode, float distance)
{
    const auto* pe = inode->parentEdge();

    /// pos()/setPos() are in scene coordinates if there is no parent.
    assert(inode->parentItem() == nullptr);
    assert(pe->target() == inode);

    const auto& source = pe->source()->pos();
    const auto& target = pe->target()->pos();
    const auto dxy     = unitVector(source, target) * distance;

    inode->setPos(target - dxy.toPointF());
}

void gui::view::adjustAllEdges(const Inode* inode)
{
    inode->parentEdge()->adjust();
    inode->newFolderEdge()->target()->setPos(inode->pos());
    inode->newFolderEdge()->adjust();
    std::ranges::for_each(std::views::keys(inode->childEdges()),
        &InodeEdge::adjust);
}
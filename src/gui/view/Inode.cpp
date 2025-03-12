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
    constexpr qreal INODE_OPEN_RADIUS      = 32.0;
    constexpr qreal INODE_OPEN_DIAMETER    = INODE_OPEN_RADIUS * 2.0;
    //constexpr qreal INODE_CLOSED_RADIUS    = INODE_OPEN_RADIUS * GOLDEN;
    constexpr qreal INODE_CLOSED_DIAMETER  = INODE_OPEN_DIAMETER * GOLDEN;
    constexpr qreal INODEEDGE_WIDTH        = 4.0;
    constexpr qreal INODE_OPEN_PEN_WIDTH   = 4.0;
    constexpr qreal INODE_CLOSED_PEN_WIDTH = INODEEDGE_WIDTH * GOLDEN;

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
    /// ----


    QVector2D unitVector(const QPointF& a, const QPointF& b)
    {
        return QVector2D(b - a).normalized();
    }

    QList<QLineF> circle(const QPointF& center, unsigned divisions, qreal radius = 1)
    {
        assert(divisions > 1);

        const auto apd = qreal(360) / divisions;
        auto start = apd;
        auto line1 = QLineF(center, center + QPointF(radius, 0));
        auto line2 = line1;

        QList<QLineF> result;
        for (unsigned i = 0; i < divisions; ++i) {
            line1.setAngle(start);
            line2.setAngle(start+apd);
            result.push_back(QLineF(line2.p2(), line1.p2()));

            start += apd;
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
    assert(source != nullptr);
    assert(target != nullptr);

    _currLabel = new InodeEdgeLabel(this);
    _nextLabel = new InodeEdgeLabel(this);
    _nextLabel->hide();

    setPen(QPen(inodeEdgeColor(), INODEEDGE_WIDTH, Qt::SolidLine, Qt::SquareCap));
    setAcceptedMouseButtons(Qt::NoButton);
}

void InodeEdge::setName(const QString& name) const
{
    assert(_currLabel->isVisible());
    assert(!_nextLabel->isVisible());
    _currLabel->alignToAxis(line(), name);
}

void InodeEdge::adjust()
{
    assert(scene());

    const auto recA     = source()->boundingRect();
    const auto recB     = target()->boundingRect();
    const auto centerA  = recA.center();
    const auto centerB  = recB.center();
    const auto pA       = mapFromItem(source(), centerA);
    const auto pB       = mapFromItem(target(), centerB);
    const auto segment  = QLineF(pA, pB);
    const auto diameter = (recA.width() + recB.width()) / 2;

    setLine(QLineF());

    /// line from the very edge of an Inode to the very edge of the other
    /// Inode, while accounting for the pen width.
    if (const auto len = segment.length(); len > diameter) {
        const auto lenInv    = 1.0 / len;
        const auto edgeWidth = INODEEDGE_WIDTH * lenInv * 0.5;
        const auto t1 = (recA.width() * 0.5 * lenInv) + edgeWidth;
        const auto t2 = 1.0 - ((recB.width() * 0.5 * lenInv) + edgeWidth);
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
    Q_UNUSED(option);
    Q_UNUSED(widget);

    assert(source() && target());

    if (line().isNull()) {
        /// nodes are too close to draw any edges.
        return;
    }

    p->setRenderHint(QPainter::Antialiasing);
    QGraphicsLineItem::paint(p, option);
}

QPainterPath InodeEdge::shape() const
{
    assert(pen().width() > 0);

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
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsGeometryChanges);
}

void Inode::init()
{
    assert(scene());

    const auto entries = _dir.entryInfoList();
    const auto count   = std::min(_winSize, entries.size());

    InodeEdges children; children.reserve(count);
    for (qsizetype i = 0; i < count; ++i) {
        auto* node = new Inode(QDir(entries.at(i).absoluteFilePath()));
        scene()->addItem(node);
        auto* edge = new InodeEdge(this, node);
        scene()->addItem(edge);

        edge->setName(node->name());

        node->_parentEdge = edge;

        children.emplace_back(edge, i);
    }
    _childEdges = children;

    spread();

    if (_dir.isRoot()) {
        auto* root = new RootNode;
        auto* rootEdge = new InodeEdge(root, this);
        root->setParentItem(rootEdge);
        scene()->addItem(rootEdge);
        scene()->addItem(root);
        _parentEdge = rootEdge;
        rootEdge->setZValue(-1);
    }
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
    const auto side = _state == FolderState::Open ? INODE_OPEN_DIAMETER : INODE_CLOSED_DIAMETER;

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

void Inode::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(inodeColor());

    qreal radius = 0;
    if (_state == FolderState::Open) {
        radius = boundingRect().width() * 0.5 - INODE_OPEN_PEN_WIDTH * 0.5;
        //p->setPen(QPen(openNodeHighlight(), INODE_OPEN_PEN_WIDTH, Qt::SolidLine));
        p->setPen(QPen(inodeColor().lighter(120), INODE_OPEN_PEN_WIDTH, Qt::SolidLine));

    } else if (_state == FolderState::Closed) {
        radius = boundingRect().width() * 0.5 - INODE_CLOSED_PEN_WIDTH * 0.5;
        p->setPen(QPen(inodeEdgeColor(), INODE_CLOSED_PEN_WIDTH, Qt::SolidLine));
    } else if (_state == FolderState::HalfClosed) {
        /// TODO
    }
    p->drawEllipse(boundingRect().center(), radius, radius);

    if (isSelected()) {
        p->setBrush(Qt::NoBrush);
        p->setPen(Qt::yellow);
        p->drawEllipse(boundingRect().center(), radius * 0.9, radius * 0.9);
    }
}

void Inode::close()
{
    doClose();

    if (ancestor()) { shrink(this); }

    if (auto* pr = asInode(_parentEdge->source())) {
        pr->onChildInodeClosed(_parentEdge);
        pr->internalRotationAfterClose(_parentEdge);
    }
}

void Inode::halfClose()
{

}

void Inode::open()
{
    if (_childEdges.empty()) {
        _state = FolderState::Open;
        if (ancestor()) { extend(this); }
        init();

        if (auto* pr = asInode(_parentEdge->source())) {
            pr->onChildInodeOpened(_parentEdge);
        }
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
    case ItemPositionChange:
        //qDebug() << "ItemPositionChange: " << value;
        break;

    case ItemPositionHasChanged:
        //qDebug() << "ItemPositionHasChanged: " << value;
        if (_parentEdge) {
            _parentEdge->adjust();
        }
        for (auto [edge,_] : _childEdges) { edge->adjust(); }
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
    case FolderState::Open: close(); break;
    case FolderState::Closed: open(); break;
    case FolderState::HalfClosed: break;
    }
}

void Inode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void Inode::doClose()
{
    auto edges = std::ranges::views::keys(_childEdges);
    std::stack stack(edges.begin(), edges.end());

    while (!stack.empty()) {
        auto* childEdge  = stack.top(); stack.pop();
        auto* childInode = asInode(childEdge->target());
        assert(childInode);
        for (auto [grandChildEdge,_] : childInode->childEdges()) {
            assert(grandChildEdge->source() == childInode);
            auto* grandChildInode = asInode(grandChildEdge->target());
            if (grandChildInode->isOpen()) {
                stack.emplace(grandChildEdge);
            } else {
                delete grandChildInode;
                delete grandChildEdge;
            }
        }
        delete childInode;
        delete childEdge;
    }

    _childEdges.clear();
    _openEdges.clear();
    _state = FolderState::Closed;
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
    for (int k1 = 0; k1 < _childEdges.size(); ++k1) {
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
    assert(end < _childEdges.size());
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

/// TODO: When we add the New Folder edge, place it at -45 degrees.
/// In CCW order, the first child edge after the New Folder edge is the first
/// edge in the 'ring', and the one before is the last.
void Inode::spread(InodeEdge* ignoredChild)
{
    if (_childEdges.empty()) {
        return;
    }
    if (ignoredChild) {
        //assert(std::ranges::find(_childEdges, ignoredChild) != _childEdges.end());
    }

    const auto center = mapRectToScene(boundingRect()).center();

    /// +1 for the parent edge, later add one more for the New Folder edge TODO:
    auto guideEdgeCount = _childEdges.size();
    if (_parentEdge) { guideEdgeCount += 1; }
    guideEdgeCount += 1;

    auto tmp = circle(center, guideEdgeCount);

    /// find should-not-move InodeEdges that intersect with the guide edges.
    /// Those guide edges will not be used.

    const auto parentEdge = _parentEdge == nullptr
        ? QLineF()
        : QLineF(center, _parentEdge->source()->mapRectToScene(_parentEdge->source()->boundingRect()).center());
    const auto ignoredChildEdge = ignoredChild == nullptr
        ? QLineF()
        : QLineF(center, ignoredChild->target()->mapToScene(ignoredChild->target()->boundingRect().center()));
    auto newFolderDummy = QLineF(center, center + QPointF(10, 0));
    newFolderDummy.setAngle(-45); /// negative angle means CW.

    int startIndex = -1;
    for (int i = 0; i < tmp.size(); i++) {
        if (tmp[i].intersects(newFolderDummy) == QLineF::BoundedIntersection) {
            startIndex = i;
            break;
        }
    }
    assert(startIndex >= 0);
    QList<QLineF> guide;
    for (int i = startIndex + 1; i < tmp.size(); i++) {
        guide.push_back(tmp[i]);
    }
    for (int i = 0; i < startIndex; i++) {
        guide.push_back(tmp[i]);
    }

    guide.removeIf([parentEdge](QLineF x) { return x.intersects(parentEdge) == QLineF::BoundedIntersection; });
    guide.removeIf([ignoredChildEdge](QLineF x) { return x.intersects(ignoredChildEdge) == QLineF::BoundedIntersection; });

    //assert(guide.size() == _childEdges.size());

    // const auto sector = 360.0 / _winSize;
    // const auto preferedLen = 128.0;
    // auto seg = QLineF(pos(), pos() + QPointF(preferedLen, 0));
    //
    // int angle = 0;
    // for (auto* c : _childEdges) {
    //     auto p2 = QPointF();
    //     if (c->line().length() < INODE_OPEN_RADIUS) {
    //         const auto uv = seg.unitVector();
    //         p2 = QPointF(uv.dx(), uv.dy()) * preferedLen;
    //     } else {
    //         p2 = seg.p2();
    //     }
    //     c->target()->setPos(p2);
    //     angle++;
    //     seg.setAngle(sector * angle);
    // }


    constexpr auto preferedLen = 144.0;
    auto seg = QLineF(pos(), pos() + QPointF(preferedLen, 0));

    for (auto [c,_] : _childEdges) {
        auto* target = asInode(c->target());
        if (target->isOpen()) {
            //guide.removeFirst();
            //continue;
        }
        seg.setAngle(guide.first().normalVector().angle());
        auto p2 = seg.p2();
        c->target()->setPos(p2);
        c->adjust();
        guide.removeFirst();
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

    inode->setPos(target + dxy.toPoint());
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

    inode->setPos(target - dxy.toPoint());
}


////////////////
/// RootNode ///
////////////////
RootNode::RootNode(QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
{
    setRect(0, 0, 24, 24);
    setPen(Qt::NoPen);
    setBrush(inodeEdgeColor());
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
}

void RootNode::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
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
    case ItemPositionHasChanged:
        if (auto* pe = qgraphicsitem_cast<InodeEdge*>(parentItem())) {
            pe->adjust();
        }
        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}

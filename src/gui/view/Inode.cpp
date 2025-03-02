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

#include <iostream>
#include <numbers>
#include <unordered_set>
#include <print>


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
    QFont inodeFont() { return {"Fire Coda", 10}; }
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

    std::unordered_set<qsizetype> closedEdges(Inode* node, InodeEdge* excluded)
    {
        std::unordered_set<qsizetype> result;
        for (auto [e,i] : node->childEdges()) {
            auto* target = asInode(e->target());
            if (target->isClosed() && e != excluded) {
                result.insert(i);
            }
        }
        return result;
    }

    std::unordered_set<qsizetype> allEdges(Inode* node, InodeEdge* excluded)
    {
        std::unordered_set<qsizetype> result;
        for (auto [e,i] : node->childEdges()) {
            if (e != excluded) {
                result.insert(i);
            }
        }
        return result;
    }


    qsizetype firstClosedEdge(Inode* node, qsizetype begin = 0, Order order = Order::Increasing)
    {
        const auto& edges = node->childEdges();

        assert(begin >= 0 && begin < edges.size());

        size_t end  = order == Order::Increasing ? edges.size() : -1;
        size_t step = order == Order::Increasing ? 1 : -1;

        for (auto k = begin; k != end; k += step) {
            auto& [e,i] = edges[k];
            auto* target = asInode(e->target());
            if (target->isOpen()) {
                continue;
            }
            return k;
        }

        return -1;
    }

    AnimPtr getVariantAnimation()
    {
        auto animation = AnimPtr::create();
        animation->setDuration(250);
        animation->setStartValue(0.0);
        animation->setEndValue(1.0);
        animation->setLoopCount(1);
        //animation->setEasingCurve(QEasingCurve::Linear);
        animation->setEasingCurve(QEasingCurve::OutCirc);

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
    const auto left = _axis.angle() >= 90 && _axis.angle() <= 270;
    const auto t01  = left ? 1.0 - _t : _t;

    setPos(normal().pointAt(t01));
    setScale(left ? -1 : 1);
    setRotation(-_axis.angle());
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
void gui::view::animateRotCW(QGraphicsScene* scene, const EdgeStringMap& input)
{
    assert(scene);

    if (input.empty()) {
        return;
    }

    using AnimPtr = QSharedPointer<QVariantAnimation>;
    auto oldAnimation = scene->property("InodeEdgeAnimation").value<AnimPtr>();

    if (!oldAnimation.isNull() && oldAnimation->state() == QAbstractAnimation::Running) {
        oldAnimation->pause();
    }

    auto animation = getVariantAnimation();
    scene->setProperty("InodeEdgeAnimation", QVariant::fromValue(animation));

    auto start = [](InodeEdge* edge, const QString& text)
    {
        assert(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line(), text);
        assert(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->updatePosCW(0, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCW(0, LabelFade::FadeIn);
        edge->nextLabel()->show();
    };

    auto progress = [](InodeEdge* edge, qreal t)
    {
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line());
        edge->currLabel()->updatePosCW(t, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCW(t, LabelFade::FadeIn);
    };

    auto finish = [](InodeEdge* edge)
    {
        edge->nextLabel()->updatePosCW(0, LabelFade::FadeOut);
        edge->currLabel()->hide();
        edge->swapLabels();
    };

    for (auto [edge, text] : input) {
        QObject::connect(animation.data(), &QVariantAnimation::stateChanged,
            [edge, text, start, finish] (QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
            {
                if (oldState == QAbstractAnimation::Stopped && newState == QAbstractAnimation::Running) {
                    start(edge, text);
                } else if (oldState == QAbstractAnimation::Running && newState == QAbstractAnimation::Paused) {
                    /// This happens when the user triggers another rotation
                    /// while the current animation is still running.  The pause
                    /// comes from 'oldAnimation->pause()'.
                    finish(edge);
                }
            });

        QObject::connect(animation.data(), &QVariantAnimation::valueChanged,
            [edge, progress] (const QVariant& value)
            {
                progress(edge, value.toReal());
            });

        QObject::connect(animation.data(), &QVariantAnimation::finished,
            [edge, finish]
            {
                finish(edge);
            });
    }

    animation->start();
}

void gui::view::animateRotCCW(QGraphicsScene* scene, const EdgeStringMap& input)
{
    assert(scene);

    if (input.empty()) {
        return;
    }

    using AnimPtr = QSharedPointer<QVariantAnimation>;
    auto oldAnimation = scene->property("InodeEdgeAnimation").value<AnimPtr>();

    if (!oldAnimation.isNull() && oldAnimation->state() == QAbstractAnimation::Running) {
        oldAnimation->pause();
    }

    auto animation = getVariantAnimation();
    scene->setProperty("InodeEdgeAnimation", QVariant::fromValue(animation));

    auto start = [](InodeEdge* edge, const QString& text)
    {
        assert(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line(), text);
        assert(edge->nextLabel()->isVisible() == false);
        edge->currLabel()->updatePosCCW(0, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCCW(0, LabelFade::FadeIn);
        edge->nextLabel()->show();
    };

    auto progress = [](InodeEdge* edge, qreal t)
    {
        edge->currLabel()->alignToAxis(edge->line());
        edge->nextLabel()->alignToAxis(edge->line());
        edge->currLabel()->updatePosCCW(t, LabelFade::FadeOut);
        edge->nextLabel()->updatePosCCW(t, LabelFade::FadeIn);
    };

    auto finish = [](InodeEdge* edge)
    {
        edge->nextLabel()->updatePosCCW(0, LabelFade::FadeOut);
        edge->currLabel()->hide();
        edge->swapLabels();
    };

    for (auto [edge, text] : input) {
        QObject::connect(animation.data(), &QVariantAnimation::stateChanged,
            [edge, text, start, finish] (QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
            {
                if (oldState == QAbstractAnimation::Stopped && newState == QAbstractAnimation::Running) {
                    start(edge, text);
                } else if (oldState == QAbstractAnimation::Running && newState == QAbstractAnimation::Paused) {
                    /// This happens when the user triggers another rotation
                    /// while the current animation is still running.  The pause
                    /// comes from 'oldAnimation->pause()'.
                    finish(edge);
                }
            });

        QObject::connect(animation.data(), &QVariantAnimation::valueChanged,
            [edge, progress] (const QVariant& value)
            {
                progress(edge, value.toReal());
            });

        QObject::connect(animation.data(), &QVariantAnimation::finished,
            [edge, finish]
            {
                finish(edge);
            });
    }

    animation->start();
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

        children.push_back({edge, i});
    }
    _childEdges = children;

    spread();
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

void Inode::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *)
{
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
    if (_parentEdge || true) {

        if (event->key() == Qt::Key_Left) {
            //rotateCCW(event->modifiers() == Qt::ShiftModifier);
            auto anm = doInternalRotation(Rotation::CW);
            qDebug() << to_string(anm.status);
            animateRotCW(scene(), anm.input);
        } else if (event->key() == Qt::Key_Right) {
            //rotateCW(event->modifiers() == Qt::ShiftModifier);
            auto anm = doInternalRotation(Rotation::CCW);
            qDebug() << to_string(anm.status);
            animateRotCCW(scene(), anm.input);
        }
    }
}

void Inode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
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

void Inode::close()
{
    for (auto [c,_] : _childEdges) {
        auto* target = asInode(c->target());
        if (target->isClosed()) {
            delete c->target();
        } else {
            target->close();
        }
        delete c;
    }
    _childEdges.clear();
    _state = FolderState::Closed;
    reduce();

    asInode(_parentEdge->source())->doInternalRotationAfterClose(_parentEdge);
}

void Inode::halfClose()
{

}

void Inode::open()
{
    if (_childEdges.empty()) {
        _state = FolderState::Open;
        extend();
        init();
    }
}

InternalRotState Inode::doInternalRotation(Rotation rot, qsizetype customBegin)
{
    auto result             = InternalRotState{rot};
    const auto el           = _dir.entryInfoList();
    const auto elLast = el.size() - 1;
    const auto inc = rot == Rotation::CW ? -1 : rot == Rotation::CCW ? 1 : 0;

    size_t Begin = rot == Rotation::CW ? 0 : _childEdges.size() - 1;
    size_t End   = rot == Rotation::CW ? _childEdges.size()    : -1;
    size_t Step  = rot == Rotation::CW ? 1 : -1;

    auto isWithinRange = [](qsizetype min, qsizetype val, qsizetype max)
    {
        return val >= min && val <= max;
    };

    /// _childEdges.size() should never be greater than el.size().
    /// _childEdges is filled, up to _winSize.
    assert(_childEdges.size() <= el.size());

    if (_childEdges.size() == el.size()) {
        result.status = InternalRotationStatus::MovementImpossible;
        return result;
    }

    if (customBegin != -1) {
        if (!isWithinRange(0, customBegin, _childEdges.size() - 1)) {
            result.status = InternalRotationStatus::MovementImpossible;
            return result;
        }
        Begin = customBegin;
    }

    std::unordered_set<qsizetype> inUse;
    for (auto [e,i] : _childEdges) {
        auto* target = asInode(e->target());
        if (target->isOpen()) {
            inUse.insert(i);
        }
    }

    int movements = 0;
    for (auto k = Begin; k != End; k += Step) {
        auto& [e,i] = _childEdges[k];
        auto* target = asInode(e->target());
        if (target->isOpen()) {
            continue;
        }

        auto t = i;
        do {
            t += inc;
            if (!isWithinRange(0, t, elLast)) {
                return result;
            }
        } while (inUse.contains(t));
        i = t;
        const auto& ap = el.at(i).absoluteFilePath();
        target->setDir(QDir(ap));
        result.input[e] = target->name();
        ++movements;
    }

    if (movements == 0) {
        result.status = InternalRotationStatus::MovementImpossible;
    } else {
        result.status = InternalRotationStatus::Normal;
    }

    return result;
}

InternalRotState Inode::doInternalRotationAfterClose(InodeEdge* edge)
{
    /// Even if 'edge' is the only closed edge, try to find the best location.

    assert(std::ranges::find_if(_childEdges, [edge](auto x){ return x.first == edge;}) != _childEdges.end());
    auto result     = InternalRotState{Rotation::CW};
    auto candidates = closedEdges(this, edge);
    const auto el           = _dir.entryInfoList();

    if (_childEdges.size() == el.size()) {
        result.status = InternalRotationStatus::MovementImpossible;
        std::cout << " HERERE  " << std::endl;

        return result;
    }

    auto isWithinRange = [](qsizetype min, qsizetype val, qsizetype max)
    {
        return val >= min && val <= max;
    };


    qsizetype edgeIndex = -1;
    if (auto found = std::ranges::find_if(_childEdges, [edge](auto x){ return x.first == edge;}); found != _childEdges.end()) {
        edgeIndex = std::distance(_childEdges.begin(), found);
    } else {
        /// this should not happen.
        assert(false);
        return result;
    }


    /// find the range in 'el' that 'edge' falls in.
    const auto elBegin = edgeIndex == 0                      ? 0             : _childEdges[edgeIndex - 1].second;
    const auto elEnd   = edgeIndex == _childEdges.size() - 1 ? el.size() - 1 : _childEdges[edgeIndex + 1].second;
    const auto all = allEdges(this, edge);
    const auto edgeEntry = _childEdges[edgeIndex].second;

    std::cout << ">> " << elBegin << " -> " << elEnd << "  \n";
    std::cout.flush();

    for (auto k = elBegin; k != elEnd; k++) {
        std::cout << k << ", ";std::cout.flush();
        if (!all.contains(k)) {
            std:: cout << " found "; std::cout.flush();
            if (k != edgeEntry) {
                std::cout << " different " << k ; std::cout.flush();
                _childEdges[edgeIndex].second = k;
                /// animate
                //return result;
                edgeIndex = k;
                break;
            } else {
                std::cout << " same " << k; std::cout.flush();
                /// the same, no need to animate, but we're done.
                //return result;
                break;
            }
        }
    }
    std::cout << "continue" << std::endl;std::cout.flush();

    for (auto k = edgeIndex + 1; k < _childEdges.size(); ++k) {
        auto* target = asInode(_childEdges[k].first->target());
        if (target->isClosed()) {
            std::cout << "1.closed: " << k << std::endl;
            auto newIndex = _childEdges[k].second;
            auto rr = doInternalRotation(Rotation::CW, k);
            if (rr.status == InternalRotationStatus::Normal) {
                assert(_childEdges[edgeIndex].first == edge);
                _childEdges[edgeIndex].second = newIndex;
                animateRotCW(scene(), rr.input);
                return result;
            }
            break;
        }
    }
    for (auto k = edgeIndex - 1; k >= 0; --k) {
        auto* target = asInode(_childEdges[k].first->target());
        if (target->isClosed()) {
            std::cout << "2.closed: " << k << std::endl;
            auto newIndex = _childEdges[k].second;
            auto rr = doInternalRotation(Rotation::CCW, k);
            if (rr.status == InternalRotationStatus::Normal) {
                assert(_childEdges[edgeIndex].first == edge);
                _childEdges[edgeIndex].second = newIndex;
                animateRotCCW(scene(), rr.input);
                return result;
            }
            break;
        }
    }


    qDebug() << "Here3";

    return result;
}


/// TODO: When we add the New Folder edge, place it at -45 degrees.
/// In CCW order, the first child edge after the New Folder edge is the first
/// edge in the 'ring', and the one after is the last.
/// TODO: Make InodeEdge not rely on Inode types, so we can reuse it for New Folder edge.
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


    constexpr auto preferedLen = 128.0;
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
void Inode::extend(float distance)
{
    // pos()/setPos() are in scene coordinates if there is no parent
    assert(parentItem() == nullptr);

    //assert(_parent->source() != nullptr); // fix root
    assert(_parentEdge->target() == this);

    const auto source = _parentEdge->source()->pos();
    const auto target = _parentEdge->target()->pos();
    const auto dxy    = unitVector(source, target) * distance;

    setPos(target + dxy.toPoint());
}

void Inode::reduce(float distance)
{
    // pos()/setPos() are in scene coordinates if there is no parent
    assert(parentItem() == nullptr);

    //assert(_parent->source() != nullptr); // fix root???
    assert(_parentEdge->target() == this);

    const auto source = _parentEdge->source()->pos();
    const auto target = _parentEdge->target()->pos();
    const auto dxy    = unitVector(source, target) * distance;

    setPos(target - dxy.toPoint());
}


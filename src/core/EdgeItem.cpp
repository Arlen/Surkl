/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "EdgeItem.hpp"
#include "SessionManager.hpp"
#include "gui/theme/theme.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>


using namespace core;

namespace
{
    constexpr qreal GOLDEN = 1.0 / std::numbers::phi;
    /// These don't change, because the scene view offers zoom feature.
    constexpr qreal NODE_OPEN_RADIUS           = 32.0;
    constexpr qreal NODE_OPEN_DIAMETER         = NODE_OPEN_RADIUS * 2.0;
    constexpr qreal NODE_HALF_CLOSED_DIAMETER  = NODE_OPEN_DIAMETER * (1.0 - GOLDEN*GOLDEN*GOLDEN);
    constexpr qreal EDGE_WIDTH                 = 4.0;
    constexpr qreal EDGE_TEXT_MARGIN_P1        = 6.0;
    constexpr qreal EDGE_TEXT_MARGIN_P2        = 4.0;
    constexpr qreal EDGE_COLLAPSED_LEN         = NODE_HALF_CLOSED_DIAMETER;

    QFont nodeFont() { return {"Adwaita Sans", 9}; }

    QLineF shrinkLine(const QLineF& line, qreal margin_p1, qreal margin_p2)
    {
        auto a = line;
        auto b = QLineF(a.p2(), a.p1());
        a.setLength(a.length() - margin_p2);
        b.setLength(b.length() - margin_p1);

        return QLineF(b.p2(), a.p2());
    }
}


/////////////////
/// EdgeLabel ///
/////////////////
EdgeLabelItem::EdgeLabelItem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    _rec = QRectF(0, 0, 1, 20);
}

void EdgeLabelItem::alignToAxis(const QLineF& axis)
{
    alignToAxis(axis, _text);
}

void EdgeLabelItem::alignToAxis(const QLineF& axis, const QString& newText)
{
    _axis = axis;
    _text = newText;
    _rec.setWidth(axis.length());
}

void EdgeLabelItem::updatePos()
{
    const auto leftSide = _axis.angle() >= 90 && _axis.angle() <= 270;

    auto xform = QTransform();
    if (leftSide) {
        xform.translate(_axis.p2().x(), _axis.p2().y());
        xform.rotate(-_axis.angle() + 180);
        xform.translate(0, -_rec.height()/2);
    } else {
        xform.translate(_axis.p1().x(), _axis.p1().y());
        xform.rotate(-_axis.angle());
        xform.translate(0, -_rec.height()/2);
    }

    setTransform(xform);
}

QRectF EdgeLabelItem::boundingRect() const
{
    return _rec;
}

void EdgeLabelItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const auto* tm = SessionManager::tm();
    painter->setPen(tm->edgeTextColor());
    painter->setFont(nodeFont());

    const auto left = _axis.angle() >= 90 && _axis.angle() <= 270;

    auto opt = QTextOption(Qt::AlignVCenter | (left ? Qt::AlignLeft : Qt::AlignRight));
    opt.setWrapMode(QTextOption::NoWrap);

    painter->drawText(boundingRect(), _text, opt);
}


////////////
/// Edge ///
////////////
EdgeItem::EdgeItem(QGraphicsItem* source, QGraphicsItem* target)
    : _source(source)
    , _target(target)
{
    Q_ASSERT(source != nullptr);
    Q_ASSERT(target != nullptr);

    setAcceptHoverEvents(true);

    _label = new EdgeLabelItem(this);

    setAcceptedMouseButtons(Qt::LeftButton);
    setFlags(ItemIsSelectable);
    setCursor(Qt::ArrowCursor);

    /// the color doesn't matter b/c it's set in paint(), but need to set the
    /// size so the boundingRect() produces the correct sized QRect.
    setPen(QPen(Qt::red, EDGE_WIDTH, Qt::SolidLine, Qt::FlatCap));
}

void EdgeItem::setText(const QString& text) const
{
    _label->alignToAxis(lineWithMargin(), text);
}

void EdgeItem::adjust()
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
        tick.setLength(EDGE_COLLAPSED_LEN);
        setLine(QLineF(tick.pointAt(0.4), tick.pointAt(0.6)));
        return;
    }

    /// QGraphicsScene::changed produces strange results if line() is null;
    /// therefore, set it to a valid short line.
    /// QGraphicsScene::changed isn't used, but the issue also affected
    /// performance.
    setLine(QLineF(QPointF(0,0), QPointF(1, 1)));
    _lineWithMargin = QLineF();

    /// line from the very edge of an Node to the very edge of the other
    /// Node, while accounting for the pen width.
    if (const auto len = segment.length(); len > diameter) {
        const auto lenInv = 1.0 / len;
        const auto t1 = recA.width() * 0.5 * lenInv;
        const auto t2 = 1.0 - recB.width() * 0.5 * lenInv;
        const auto p1 = segment.pointAt(t1);
        const auto p2 = segment.pointAt(t2);

        setLine(QLineF(p1, p2));
        _lineWithMargin = shrinkLine(line(), EDGE_TEXT_MARGIN_P1, EDGE_TEXT_MARGIN_P2);

        _label->alignToAxis(lineWithMargin());
        _label->updatePos();
    }
}

void EdgeItem::adjustSourceTo(const QPointF& pos)
{
    Q_ASSERT(scene());
    Q_ASSERT(_state == ActiveState);

    const auto recA     = source()->boundingRect();
    const auto recB     = target()->boundingRect();
    const auto centerB  = recB.center();
    const auto pA       = mapFromScene(pos);
    const auto pB       = mapFromItem(target(), centerB);
    const auto segment  = QLineF(pA, pB);
    const auto diameter = (recA.width() + recB.width()) / 2;

    if (const auto len = segment.length(); len > diameter) {
        const auto lenInv = 1.0 / len;
        const auto t1 = 0.0;
        const auto t2 = 1.0 - recB.width() * 0.5 * lenInv;
        const auto p1 = segment.pointAt(t1);
        const auto p2 = segment.pointAt(t2);

        setLine(QLineF(p1, p2));
        _lineWithMargin = shrinkLine(line(), EDGE_TEXT_MARGIN_P1, EDGE_TEXT_MARGIN_P2);

        _label->alignToAxis(lineWithMargin());
        _label->updatePos();
    }
}

/// CollapsedState is only used when a source node is in HalfClosed state. The
/// target node is disabled and hidden, but the edge remains visible and
/// disabled. The edge is set up as a tick mark for a HalfClosed source node,
/// and this set up takes place in Edge::adjust().
void EdgeItem::setState(State state)
{
    Q_ASSERT(_state != state);
    _state = state;

    auto toggleMembers = [this](bool onOff)
    {
        setEnabled(onOff);
        target()->setEnabled(onOff);

        label()->setVisible(onOff);
        target()->setVisible(onOff);
    };

    switch (_state) {
    case ActiveState:
        setVisible(true);
        toggleMembers(true);
        break;
    //
    // case InactiveState:
    //     setVisible(false);
    //     toggleMembers(false);
    //     break;

    case CollapsedState:
        setVisible(true);
        toggleMembers(false);
        break;
    }
}

void EdgeItem::paint(QPainter *p, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    Q_UNUSED(widget);

    const auto* tm = SessionManager::tm();

    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(Qt::NoBrush);

    if (_state == CollapsedState) {
        p->setPen(QPen(tm->edgeColor(), EDGE_WIDTH, Qt::SolidLine, Qt::FlatCap));
        p->drawLine(line());
        return;
    }

    auto pen = QPen(tm->edgeColor(), EDGE_WIDTH, Qt::SolidLine, Qt::FlatCap);

    if (option->state & QStyle::State_Selected) {
        pen.setColor(tm->edgeLightColor());
    } else if (option->state & QStyle::State_MouseOver) {
        pen.setColor(tm->edgeMidlightColor());
    }

    p->setPen(pen);
    p->drawLine(line());

    const auto p1 = line().p1();
    const auto uv = line().unitVector();
    const auto v2 = QPointF(uv.dx(), uv.dy()) * 5.0;

    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(tm->openNodeLightColor(), EDGE_WIDTH, Qt::SolidLine, Qt::FlatCap));
    p->drawLine(QLineF(p1, p1 + v2));
}

QPainterPath EdgeItem::shape() const
{
    Q_ASSERT(pen().width() > 0);

    QPainterPath path;

    if (line() == QLineF())
        return path;

    path.moveTo(line().p1());
    path.lineTo(line().p2());

    QPainterPathStroker ps;
    ps.setCapStyle(pen().capStyle());
    /// make the width a little wider to make edge selection easier.
    ps.setWidth(pen().widthF()*4);
    ps.setJoinStyle(pen().joinStyle());
    ps.setMiterLimit(pen().miterLimit());

    return ps.createStroke(path);
}

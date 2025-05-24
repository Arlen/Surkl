/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "EdgeItem.hpp"
#include "SessionManager.hpp"
#include "theme.hpp"

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
    : QGraphicsSimpleTextItem(parent)
{
    const auto* tm = SessionManager::tm();

    setPen(Qt::NoPen);
    setBrush(tm->edgeTextColor());
    setFont(nodeFont());
}

void EdgeLabelItem::alignToAxis(const QLineF& axis)
{
    alignToAxis(axis, _text);
}

/// sets this label's text so that it fits within length the of given segment.
/// computes a normal that is perpendicular to the segment, and it marks the
/// start of this EdgeLabel's shape().
void EdgeLabelItem::alignToAxis(const QLineF& axis, const QString& newText)
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

void EdgeLabelItem::updatePos()
{
    const auto left = _axis.angle() >= 90 && _axis.angle() <= 270;
    const auto p1   = _normal.p1();
    const auto p2   = _normal.p2();

    const auto pathOfMovement = left ? QLineF(p2, p1) : QLineF(p1, p2);

    setPos(pathOfMovement.pointAt(0));
    setScale(left ? -1 : 1);
    setRotation(-_axis.angle());
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

    setLine(QLineF());
    _lineWithMargin = QLineF();

    /// line from the very edge of an Node to the very edge of the other
    /// Node, while accounting for the pen width.
    if (const auto len = segment.length(); len > diameter) {
        const auto lenInv    = 1.0 / len;
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

    if (line().isNull()) {
        /// nodes are too close to draw any edges.
        return;
    }

    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(option->state & QStyle::State_Selected
            ? tm->edgeColor().lighter(1600)
            : tm->edgeColor()
        , EDGE_WIDTH, Qt::SolidLine, Qt::FlatCap));
    p->drawLine(line());

    if (_state == CollapsedState) {
        return;
    }

    const auto p1 = line().p1();
    const auto uv = line().unitVector();
    const auto v2 = QPointF(uv.dx(), uv.dy()) * 3.0;

    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(tm->openNodeBorderColor(), EDGE_WIDTH, Qt::SolidLine, Qt::SquareCap));
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
    ps.setWidth(pen().widthF());
    ps.setJoinStyle(pen().joinStyle());
    ps.setMiterLimit(pen().miterLimit());

    return ps.createStroke(path);
}

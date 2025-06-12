/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#define DRAW_ITEM_SHAPE 0
#define DRAW_ITEM_BOUNDING_RECT 0

#include "BookmarkItem.hpp"
#include "SessionManager.hpp"
#include "theme.hpp"

#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>


using namespace core;

void internal::drawItemShape(QPainter* p, QGraphicsItem* item)
{
    p->save();
    p->setPen(Qt::red);
    p->setBrush(QBrush(QColor(0, 0, 0, 0)));
    p->drawPath(item->shape());
    p->restore();
}

void internal::drawBoundingRect(QPainter* p, QGraphicsItem* item)
{
    p->save();
    p->setPen(Qt::green);
    p->setBrush(QBrush(QColor(0, 0, 0, 0)));
    p->drawRect(item->boundingRect());
    p->restore();
}

SceneBookmarkItem::SceneBookmarkItem(const QPoint& pos, const QString& name)
{
    setFlags(ItemIsSelectable);
    setCursor(Qt::ArrowCursor);
    setAcceptHoverEvents(true);

    // pen size affects the bounding box size.
    // if the bounding box size is the same as the rect size, then smearing can
    // occur.
    setPen(QPen(QColor(0, 0, 0, 0), 1));
    setBrush(Qt::NoBrush);

    setRect(QRect(0, 0, RECT_SIZE, RECT_SIZE));
    setPos(pos.x() - RECT_SIZE/2, pos.y() - RECT_SIZE/2);

    _name = new QGraphicsSimpleTextItem(this);
    _name->setText(name);
    _name->setPos(QPointF(0, 32));
    _name->hide();
}

void SceneBookmarkItem::paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
#if DRAW_ITEM_BOUNDING_RECT
    internal::drawBoundingRect(p, this);
#endif

    const auto* tm  = SessionManager::tm();

    const auto rec = rect();
    const auto cp = rec.center();
    auto mid = QRectF(0, 0, RECT_SIZE/2, RECT_SIZE/2);
    mid.moveCenter(cp);

    p->setRenderHint(QPainter::Antialiasing);

    p->setBrush(Qt::NoBrush);
    p->setPen(tm->edgeColor());
    p->drawLine(QLineF(cp, QPointF(cp.x(), rec.top())));
    p->drawLine(QLineF(cp, QPointF(cp.x(), rec.bottom())));
    p->drawLine(QLineF(cp, QPointF(rec.left(), cp.y())));
    p->drawLine(QLineF(cp, QPointF(rec.right(), cp.y())));

    if (option->state & QStyle::State_Selected) {
        p->setBrush(tm->openNodeLightColor());
    } else if (option->state & QStyle::State_MouseOver) {
        p->setBrush(tm->openNodeMidlightColor());
    } else {
        p->setBrush(tm->openNodeColor());
    }

    p->drawRect(mid);
}

void SceneBookmarkItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    const auto* tm  = SessionManager::tm();
    _name->setPen(Qt::NoPen);
    _name->setBrush(tm->sceneFgColor());

    _name->show();

    QGraphicsRectItem::hoverEnterEvent(event);
}

void SceneBookmarkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    _name->hide();

    QGraphicsRectItem::hoverLeaveEvent(event);
}
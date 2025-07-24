/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "buttons.hpp"
#include "SessionManager.hpp"
#include "gui/theme/theme.hpp"

#include <QGraphicsEffect>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>

#include <ranges>


using namespace core;

SceneButton::SceneButton(const QPointF& pos)
{
    setPos(pos);
    setFlag(ItemIsMovable);

    auto* tm = SessionManager::tm();
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setColor(tm->sceneShadowColor());
    shadow->setBlurRadius(5);
    shadow->setOffset(QPointF(0, 8));
    setGraphicsEffect(shadow);

    connect(this, &QGraphicsObject::enabledChanged, [this] {
        if (isEnabled()) {
            setOpacity(1);
        } else {
            setOpacity(0.7);
        }
    });

    setZValue(0);

    _buttons.insert(this);
}

SceneButton::~SceneButton()
{
    _buttons.erase(this);
}

void SceneButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    setZValue(1);

    const auto mod = event->modifiers();
    if (mod &  Qt::ControlModifier && mod & Qt::AltModifier && mod & Qt::ShiftModifier) {

        auto* other = clone(mapToScene(QPointF(32, 32)));
        scene()->addItem(other);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect())) {
            shadow->setOffset(QPointF(0, 4));
        }

        emit pressed();
    } else if (_deleteTimerId == 0 && event->button() == Qt::RightButton) {
        _deleteTimerId = startTimer(2000);
    }

    QGraphicsObject::mousePressEvent(event);
}

void SceneButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    setZValue(0);

    if (_deleteTimerId) {
        killTimer(_deleteTimerId);
        _deleteTimerId = 0;
    }

    if (auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect())) {
        shadow->setOffset(QPointF(0, 8));
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void SceneButton::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    const auto mod = event->modifiers();
    if (mod &  Qt::ControlModifier && mod & Qt::AltModifier) {
        if (auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect())) {
            shadow->setOffset(QPointF(0, 0));
        }
        return QGraphicsObject::mouseMoveEvent(event);
    }

    event->ignore();
}

void SceneButton::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == _deleteTimerId) {
        deleteMe();
    }

    QGraphicsObject::timerEvent(event);
}

QRectF SceneButton::boundingRect() const
{
    return QRectF(0, 0, 32, 32);
}

QPainterPath SceneButton::shape() const
{
    auto path = QPainterPath();
    path.addEllipse(boundingRect());

    return path;
}

void SceneButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    auto* tm = SessionManager::tm();

    const auto rec = boundingRect();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(tm->sceneLightColor(), 2));
    painter->setBrush(tm->sceneDarkColor());

    painter->drawEllipse(rec.adjusted(1, 1, -1, -1));
}


/// AboutButton
AboutButton::AboutButton(const QPointF &pos)
    : SceneButton(pos)
{ }

void AboutButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    SceneButton::paint(painter, option, widget);

    auto* tm = SessionManager::tm();

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(tm->sceneLightColor(), 1));
    painter->drawText(boundingRect(), Qt::AlignCenter, "i");
}

SceneButton* AboutButton::clone(const QPointF& pos) const
{
    return new AboutButton(pos);
}

void AboutButton::deleteMe()
{
    if (std::ranges::distance(siblings<AboutButton>()) > 1) {
        deleteLater();
    }
}


/// ThemeButton
ThemeButton::ThemeButton(const QPointF &pos)
    : SceneButton(pos)
{ }

void ThemeButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    SceneButton::paint(painter, option, widget);

    auto* tm = SessionManager::tm();

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(tm->sceneLightColor(), 1));
    painter->drawText(boundingRect(), Qt::AlignCenter, "TS");
}

SceneButton* ThemeButton::clone(const QPointF& pos) const
{
    return new ThemeButton(pos);
}

void ThemeButton::deleteMe()
{
    if (std::ranges::distance(siblings<ThemeButton>()) > 1) {
        deleteLater();
    }
}
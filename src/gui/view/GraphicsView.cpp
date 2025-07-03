/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "GraphicsView.hpp"
#include "QuadrantButton.hpp"
#include "core/BookmarkItem.hpp"
#include "core/FileSystemScene.hpp"

#include <QMouseEvent>
#include <QTimeLine>
#include <QVariantAnimation>


using namespace gui::view;

GraphicsView::GraphicsView(core::FileSystemScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    configure();

    connect(scene, &QGraphicsScene::selectionChanged, this, &GraphicsView::pickSceneBookmark);

    _quadrantButton = new QuadrantButton(this);
    _quadrantButton->hide();

    connect(_quadrantButton, &QuadrantButton::quad1Pressed, this, &GraphicsView::focusQuadrant1);
    connect(_quadrantButton, &QuadrantButton::quad2Pressed, this, &GraphicsView::focusQuadrant2);
    connect(_quadrantButton, &QuadrantButton::quad3Pressed, this, &GraphicsView::focusQuadrant3);
    connect(_quadrantButton, &QuadrantButton::quad4Pressed, this, &GraphicsView::focusQuadrant4);
    connect(_quadrantButton, &QuadrantButton::centerPressed, this, &GraphicsView::focusAllQuadrants);

    connect(this, &GraphicsView::sceneBookmarkRequested, scene, &core::FileSystemScene::addSceneBookmark);

    _timeline = new QTimeLine(300, this);
    _timeline->setFrameRange(0, 36);
    _timeline->setEasingCurve(QEasingCurve::OutExpo);

    setAcceptDrops(false);
}

void GraphicsView::requestSceneBookmark()
{
    if (_bookmarkAnimation) {
        destroyBookmarkAnimation();
    } else {
        _bookmarkAnimation = new QVariantAnimation(this);
        _bookmarkAnimation->setDuration(200);
        _bookmarkAnimation->setStartValue(8);
        _bookmarkAnimation->setEndValue(32);
        _bookmarkAnimation->setLoopCount(-1);
        _bookmarkAnimation->setEasingCurve(QEasingCurve::OutCubic);
        connect(_bookmarkAnimation, &QVariantAnimation::valueChanged, this,
            [this]{ viewport()->update(); });
        _bookmarkAnimation->start();
    }
}

void GraphicsView::focusQuadrant1()
{
    if (const auto sbm = selectedSceneBookmarks(); sbm.size() == 1) {
        centerTargetOn(sbm.first(), mapToScene(rect().bottomLeft()));
    }
}

void GraphicsView::focusQuadrant2()
{
    if (const auto sbm = selectedSceneBookmarks(); sbm.size() == 1) {
        centerTargetOn(sbm.first(), mapToScene(rect().bottomRight()));
    }
}

void GraphicsView::focusQuadrant3()
{
    if (const auto sbm = selectedSceneBookmarks(); sbm.size() == 1) {
        centerTargetOn(sbm.first(), mapToScene(rect().topRight()));
    }
}

void GraphicsView::focusQuadrant4()
{
    if (const auto sbm = selectedSceneBookmarks(); sbm.size() == 1) {
        centerTargetOn(sbm.first(), mapToScene(rect().topLeft()));
    }
}

void GraphicsView::focusAllQuadrants()
{
    if (const auto sbm = selectedSceneBookmarks(); sbm.size() == 1) {
        centerTargetOn(sbm.first(), mapToScene(rect().center()));
    }
}

void GraphicsView::enterEvent(QEnterEvent* event)
{
    togglePanOrZoom(Qt::NoModifier);

    QGraphicsView::enterEvent(event);
}

void GraphicsView::keyPressEvent(QKeyEvent *event)
{
    togglePanOrZoom(event->modifiers());

    QGraphicsView::keyPressEvent(event);
}

void GraphicsView::keyReleaseEvent(QKeyEvent* event)
{
    togglePanOrZoom(event->modifiers());

    QGraphicsView::keyReleaseEvent(event);
}

void GraphicsView::leaveEvent(QEvent* event)
{
    togglePanOrZoom(Qt::NoModifier);

    QGraphicsView::leaveEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    togglePanOrZoom(event->modifiers());
    if (event->modifiers() == Qt::AltModifier && _bookmarkAnimation == nullptr) {
        zoom();
    }

    saveMousePosition(event->pos());

    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (_bookmarkAnimation) {
        destroyBookmarkAnimation();

        const auto pos  = mapToScene(event->position().toPoint()).toPoint();
        const auto name = QString("(%1,%2)").arg(pos.x()).arg(pos.y());

        emit sceneBookmarkRequested(pos, name);
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);

    if (_bookmarkAnimation) {
        QPainter p(viewport());
        drawBookmarkingCursorAnimation(p);
    }
}

void GraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    auto rec = _quadrantButton->rect();
    rec.moveTopRight(rect().topRight() + QPoint(-16, 16));
    _quadrantButton->setGeometry(rec);
}

void GraphicsView::configure()
{
    setFrameShape(NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // Setting background color to Qt::NoBrush is needed so that
    // Scene::drawBackground() virtual method gets called.
    setBackgroundBrush(Qt::NoBrush);

    setProperty(MOUSE_POSITION_PROPERTY, QPoint(0, 0));
    setProperty(MOUSE_LAST_POSITION_PROPERTY, QPoint(0, 0));
}

QPoint GraphicsView::mouseMoveVelocity() const
{
    const auto last = property(MOUSE_LAST_POSITION_PROPERTY).toPoint();
    const auto curr = property(MOUSE_POSITION_PROPERTY).toPoint();

    return curr - last;
}

QPoint GraphicsView::mousePosition() const
{
    return property(MOUSE_POSITION_PROPERTY).toPoint();
}

void GraphicsView::saveMousePosition(const QPoint& pos)
{
    setProperty(MOUSE_LAST_POSITION_PROPERTY, property(MOUSE_POSITION_PROPERTY).toPoint());
    setProperty(MOUSE_POSITION_PROPERTY, pos);
}

void GraphicsView::scaleView(qreal factor)
{
    scale(factor, factor);
}

void GraphicsView::zoom()
{
    const auto velocity = mouseMoveVelocity();
    if (velocity.x() > 0) {
        zoomIn();
    } else if (velocity.x() < 0) {
        zoomOut();
    }
}

void GraphicsView::zoomIn()
{
    scaleView(1.01);
}

void GraphicsView::zoomOut()
{
    scaleView(1 / 1.01);
}

void GraphicsView::togglePanOrZoom(Qt::KeyboardModifiers modifiers)
{
    auto reset = [this] {
        viewport()->setCursor(Qt::ArrowCursor);
        setDragMode(QGraphicsView::NoDrag);
    };

    if (_bookmarkAnimation) {
        reset();
        return;
    }

    if (modifiers == Qt::AltModifier) {
        /// zooming is enabled only when Alt modifier is active.
        viewport()->setCursor(Qt::SizeAllCursor);
    } else if (modifiers == Qt::ControlModifier) {
        /// ScrollHandDrag mode is enabled only when Ctrl modifier is active.
        setDragMode(QGraphicsView::ScrollHandDrag);
    } else {
        /// otherwise, set everything to normal.
        reset();
    }
}

void GraphicsView::destroyBookmarkAnimation()
{
    assert(_bookmarkAnimation);

    connect(_bookmarkAnimation, &QVariantAnimation::destroyed, this,
        [this]{ viewport()->update(); });
    _bookmarkAnimation->stop();
    _bookmarkAnimation->deleteLater();
    _bookmarkAnimation = nullptr;
}

void GraphicsView::drawBookmarkingCursorAnimation(QPainter& p) const
{
    p.save();
    p.setPen(QPen(QColor(160, 160, 160, 255), 2));
    p.setCompositionMode(QPainter::CompositionMode_Exclusion);

    const auto len  = _bookmarkAnimation->currentValue().toInt();
    const auto line = QLine(QPoint(0, 0), QPoint(len, 0));
    const auto pos  = mousePosition();

    auto xform = QTransform();

    p.drawLine(line * xform.translate(pos.x(), pos.y()));
    p.drawLine(line * xform.rotate(90));
    p.drawLine(line * xform.rotate(90*2));
    p.drawLine(line * xform.rotate(90*3));

    const auto coord = mapToScene(pos).toPoint();
    const auto text  = QString("(%1, %2)").arg(coord.x()).arg(coord.y());
    const auto fmt   = QFontMetrics(p.font());
    const auto bbox  = fmt.boundingRect(text);

    p.drawText(pos + QPoint(-bbox.width()/2, 48), text);
    p.restore();
}

/// centers the target position on given bookmark item.
void GraphicsView::centerTargetOn(const core::SceneBookmarkItem* bm, const QPointF& target)
{
    const auto bmCenter   = bm->mapToScene(bm->rect().center());
    const auto viewCenter = mapToScene(rect().center());
    const auto path       = QLineF(QPointF(0, 0), bmCenter - target);

    if (path.length() < 2) {
        return;
    }

    if (_timeline->state() == QTimeLine::State::Running) {
        _timeline->stop();
        disconnect(_timeline, &QTimeLine::valueChanged, nullptr, nullptr);
    }

    connect(_timeline, &QTimeLine::valueChanged,
        [viewCenter, path, this](qreal t) {
            centerOn(viewCenter + path.pointAt(t));
    });

    connect(_timeline, &QTimeLine::finished, [this] {
        disconnect(_timeline, &QTimeLine::valueChanged, nullptr, nullptr);
    });

    _timeline->start();
}

void GraphicsView::pickSceneBookmark() const
{
    if (const auto sbm = selectedSceneBookmarks(); sbm.size() == 1) {
        _quadrantButton->setVisible(true);
    } else {
        _quadrantButton->setVisible(false);
    }
}

QList<core::SceneBookmarkItem*> GraphicsView::selectedSceneBookmarks() const
{
    QList<core::SceneBookmarkItem*> result;

    for (const auto selection = scene()->selectedItems(); auto* item : selection) {
        if (auto* sbm = qgraphicsitem_cast<core::SceneBookmarkItem*>(item); sbm) {
            result.push_back(sbm);
        }
    }

    return result;
}
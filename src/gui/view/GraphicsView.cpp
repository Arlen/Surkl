#include "GraphicsView.hpp"
#include "QuadrantButton.hpp"
#include "core/nodes.hpp"
#include "core/Scene.hpp"

#include <QMouseEvent>
#include <QVariantAnimation>


using namespace gui::view;

GraphicsView::GraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    configure();

    connect(scene, &QGraphicsScene::selectionChanged, this, &GraphicsView::processSelection);

    _quadrantButton = new QuadrantButton(this);
    _quadrantButton->hide();

    connect(_quadrantButton, &QuadrantButton::quad1Pressed, this, &GraphicsView::focusQuadrant1);
    connect(_quadrantButton, &QuadrantButton::quad2Pressed, this, &GraphicsView::focusQuadrant2);
    connect(_quadrantButton, &QuadrantButton::quad3Pressed, this, &GraphicsView::focusQuadrant3);
    connect(_quadrantButton, &QuadrantButton::quad4Pressed, this, &GraphicsView::focusQuadrant4);
    connect(_quadrantButton, &QuadrantButton::centerPressed, this, &GraphicsView::focusAllQuadrants);
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
    if (_selectedSceneBookmark) {
        const auto bookmarkCenter = _selectedSceneBookmark->sceneBoundingRect().center();
        auto region = mapToScene(rect()).boundingRect();
        region.moveBottomLeft(bookmarkCenter);
        ensureVisible(region, 0, 0);
    }
}

void GraphicsView::focusQuadrant2()
{
    if (_selectedSceneBookmark) {
        const auto bookmarkCenter = _selectedSceneBookmark->sceneBoundingRect().center();
        auto region = mapToScene(rect()).boundingRect();
        region.moveBottomRight(bookmarkCenter);
        ensureVisible(region, 0, 0);
    }
}

void GraphicsView::focusQuadrant3()
{
    if (_selectedSceneBookmark) {
        const auto bookmarkCenter = _selectedSceneBookmark->sceneBoundingRect().center();
        auto region = mapToScene(rect()).boundingRect();
        region.moveTopRight(bookmarkCenter);
        ensureVisible(region, 0, 0);
    }
}

void GraphicsView::focusQuadrant4()
{
    if (_selectedSceneBookmark) {
        const auto bookmarkCenter = _selectedSceneBookmark->sceneBoundingRect().center();
        auto region = mapToScene(rect()).boundingRect();
        region.moveTopLeft(bookmarkCenter);
        ensureVisible(region, 0, 0);
    }
}

void GraphicsView::focusAllQuadrants()
{
    if (_selectedSceneBookmark) {
        centerOn(_selectedSceneBookmark);
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
        addSceneBookmark(event->pos());
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

    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::NoAnchor);

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
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    constexpr auto unit         = QRectF(0, 0, 1, 1);
    constexpr auto zoomInLimit  = QRectF(0, 0, 4, 4);
    constexpr auto zoomOutLimit = QRectF(0, 0, 0.25, 0.25);

    const auto sx   = factor;
    const auto sy   = factor;
    const auto zoom = transform().scale(sx, sy).mapRect(unit).size();

    const auto candidate = QRectF(QPointF(0, 0), zoom);

    if (zoomInLimit.contains(candidate) && candidate.contains(zoomOutLimit)) {
        scale(sx, sy);
    }
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

void GraphicsView::addSceneBookmark(const QPoint& pos)
{
    if (auto* gs = qobject_cast<core::Scene*>(scene())) {
        gs->addSceneBookmark(mapToScene(pos).toPoint(), "test");
    }
}

void GraphicsView::processSelection()
{
    using namespace core::nodes;

    /// TODO: once more item types are added to the project, a momre
    /// sophisticated approach to selection will be needed to handle different
    /// sets of rules.
    /// A. only one SceneBookmarkItem can be selected at a time.
    /// B. selecting a SceneBookmarkItem when other types of items have already
    ///    been selected should unselect the other items.
    /// C. ...other rules!?

    /// 1. toggle quadrant button.
    bool visible = false;
    _selectedSceneBookmark = nullptr;

    if (auto sel = scene()->selectedItems(); !sel.isEmpty()) {
        /// always select the last; last is not always the most recent selection.
        if ((_selectedSceneBookmark = qgraphicsitem_cast<SceneBookmarkItem*>(sel.last()))) {
            visible = true;
        }
    }
    _quadrantButton->setVisible(visible);
}

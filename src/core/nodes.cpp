#define DRAW_ITEM_SHAPE 0
#define DRAW_ITEM_BOUNDING_RECT 0

#include "nodes.hpp"

#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>
#include <QTimeLine>


using namespace core::nodes;

void core::nodes::drawItemShape(QPainter* p, QGraphicsItem* item)
{
    p->save();
    p->setPen(Qt::red);
    p->setBrush(QBrush(QColor(0, 0, 0, 0)));
    p->drawPath(item->shape());
    p->restore();
}

void core::nodes::drawBoundingRect(QPainter* p, QGraphicsItem* item)
{
    p->save();
    p->setPen(Qt::green);
    p->setBrush(QBrush(QColor(0, 0, 0, 0)));
    p->drawRect(item->boundingRect());
    p->restore();
}

SceneBookmarkItem::SceneBookmarkItem(const QPoint& pos, const QString& name, bool born)
{
    setFlags(ItemIsSelectable);
    setCursor(Qt::ArrowCursor);

    // pen size affects the bounding box size.
    // if the bounding box size is the same as the rect size, then smearing can
    // occur.
    setPen(QPen(QColor(16, 16, 16, 255), 1));

    // not sure if the brush has any impact.
    setBrush(Qt::NoBrush);

    setRect(QRect(0, 0, RECT_SIZE, RECT_SIZE));
    setPos(pos.x() - RECT_SIZE/2, pos.y() - RECT_SIZE/2);

    _name = name;
    using TimeLine = geometry::SceneBookmarkIcon::TimeLine;

    if (born) {
        _frame = TimeLine::Frame0;
        auto* timeline = new QTimeLine(500);
        timeline->setFrameRange(0, POLYGONS.size());
        timeline->setEasingCurve(QEasingCurve::InBounce);

        QObject::connect(timeline, &QTimeLine::frameChanged, [this](int frame)
        {
            setFrame(frame);
        });
        QObject::connect(timeline, &QTimeLine::finished, timeline, &QObject::deleteLater);
        timeline->start();
    } else {
        _frame = TimeLine::Last;
    }

    _bigLeafColors = {
        QColor(96, 48, 48, 255),
        QColor(128, 64, 64, 255),
        QColor(48, 64, 96, 255),
        QColor(64, 80, 128, 255),
    };

    _smallLeafColors = {
        QColor(128, 128, 128, 255),
        QColor(160, 160, 160, 255),
        QColor(64, 64, 64, 255),
        QColor(96, 96, 96, 255),
    };

    constexpr auto  focalPoint = QPointF(RECT_SIZE * 0.3, RECT_SIZE * -0.3);
    _bigGradient1 = QRadialGradient(QPointF(0, 0), RECT_SIZE/2, focalPoint);
    _bigGradient1.setColorAt(1, QColor(250, 230, 230, 255));
    _bigGradient1.setColorAt(0, QColor(200, 48, 48, 255));

    _bigGradient2 = QRadialGradient(QPointF(0, 0), RECT_SIZE/2, focalPoint);
    _bigGradient2.setColorAt(1, QColor(230, 230, 250, 255));
    _bigGradient2.setColorAt(0, QColor(48, 48, 250, 255));

    _bigGradient3 = QRadialGradient(QPointF(0, 0), RECT_SIZE/2, QPointF(0, 0));
    _bigGradient3.setColorAt(1, QColor(200, 200, 200, 255));
    _bigGradient3.setColorAt(0, QColor(64, 64, 64, 255));
}

void SceneBookmarkItem::paint(QPainter* p, const QStyleOptionGraphicsItem* /*option*/,
    QWidget* /*widget*/)
{
    #if DRAW_ITEM_BOUNDING_RECT
    draw_item_bounding_rect(p, this);
    #endif

    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->translate(rect().center());

    using TimeLine = geometry::SceneBookmarkIcon::TimeLine;

    /// draw bright outlines of the four main squares until the main four
    /// leaves finish drawing.
    if (_frame < TimeLine::Frame13) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(QColor(210, 200, 190, 255), 1));
        for (int i = 0; i < std::min(_frame, static_cast<int>(TimeLine::Frame4)); ++i) {
            p->drawPolygon(POLYGONS[i]);
        }
    }

    const auto isSel = isSelected();

    /// draw the dark unit square as background after the main four leaves
    /// have finished drawing.
    if (_frame >= TimeLine::Frame13) {
        p->setBrush(isSel ? QColor(48, 48, 48, 255) : QColor(32, 32, 32, 255));
        p->setPen(Qt::NoPen);
        p->drawPolygon(POLYGONS[TimeLine::Frame12]);
    }

    if (_frame <= TimeLine::Last) {
        p->setPen(Qt::NoPen);

        /// draw the four main leaves, starting from top left, going CW.
        int color_index =  0;
        for (int i = TimeLine::Frame4; i < std::min(_frame, static_cast<int>(TimeLine::Frame12)); ++i) {
            const auto color = isSel
                ? _bigLeafColors[color_index].lighter()
                : _bigLeafColors[color_index];
            p->setBrush(color);
            p->drawPolygon(POLYGONS[i]);
            color_index = (color_index + 1) % _bigLeafColors.size();
        }

        /// draw the four smaller leaves, starting from where the main leaves
        /// left off, going CW.
        color_index = 0;
        for (int i = TimeLine::Frame13; i < std::min(_frame, static_cast<int>(TimeLine::Last)); ++i) {
            const auto color = isSel
                ? _smallLeafColors[color_index].lighter()
                : _smallLeafColors[color_index];
            p->setBrush(color);
            p->drawPolygon(POLYGONS[i]);
            color_index = (color_index + 1) % _smallLeafColors.size();
        }
    }

    if (_frame == TimeLine::Last) {
        p->setBrush(Qt::NoBrush);

        /// draw small leaf highlights
        p->setPen(QPen(QBrush(_bigGradient3), 2, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame13][3]));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame15][3]));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame17][3]));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame19][3]));

        /// draw north/south big leaf highlights
        p->setPen(QPen(QBrush(_bigGradient1), 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame0][2]));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame0][0]));

        /// draw east/west big leaf highlights
        p->setPen(QPen(QBrush(_bigGradient2), 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame0][1]));
        p->drawLine(QLineF(QPointF(0,0), POLYGONS[TimeLine::Frame0][3]));

        p->setPen(QColor(255, 128, 128, 64));
        for (int i = 0; i < TimeLine::Frame4; ++i) {
            p->drawPolygon(POLYGONS[i]);
        }
    }
    p->restore();
}

void SceneBookmarkItem::setFrame(int frame)
{
    _frame = frame;
    const auto rec = mapToScene(rect()).boundingRect();
    scene()->update(rec);
}

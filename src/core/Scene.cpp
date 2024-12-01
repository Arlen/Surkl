#include "core/Scene.hpp"

#include <QPainter>


using namespace core;

Scene::Scene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRect(-7680 * 4, -4320 * 4, 7680 * 8, 4320 * 8));
}

void Scene::drawBackground(QPainter *p, const QRectF& rec)
{
    p->save();
    constexpr auto bgColor = QColor(67, 67, 67, 255);
    p->fillRect(rec, bgColor);
    drawCrosses(p, rec);
    p->restore();;
}

void Scene::drawCrosses(QPainter* p, const QRectF& rec) const
{
    constexpr qreal grid = 512;
    const auto l = rec.left();
    const auto r = rec.right();
    const auto t = rec.top();
    const auto b = rec.bottom();

    const auto x0 = std::floor(std::abs(l) / grid) * grid * (std::signbit(l) ? -1.0 : 1.0) - grid;
    const auto x1 = std::floor(std::abs(r) / grid) * grid * (std::signbit(r) ? -1.0 : 1.0) + grid;
    const auto y0 = std::floor(std::abs(t) / grid) * grid * (std::signbit(t) ? -1.0 : 1.0) - grid;
    const auto y1 = std::floor(std::abs(b) / grid) * grid * (std::signbit(b) ? -1.0 : 1.0) + grid;

    constexpr auto dy0 = QPointF{ 0,-8};
    constexpr auto dy1 = QPointF{ 0, 8};
    constexpr auto dx0 = QPointF{-8, 0};
    constexpr auto dx1 = QPointF{ 8, 0};

    p->setPen(QPen(QColor(48, 48, 48, 255), 1));
    for (qreal x = x0; x < x1; x += grid) {
        for (qreal y = y0; y < y1; y += grid) {
            const auto pos = QPointF{x, y};
            p->drawLine({pos+dy0, pos+dy1});
            p->drawLine({pos+dx0, pos+dx1});
        }
    }

    if (rec.contains(QPoint{0, 0})) {
        p->setPen(QPen(QColor(48, 48, 48, 255), 2));
        const auto pos = QPointF{0, 0};
        p->drawLine({pos+dy0, pos+dy1});
        p->drawLine({pos+dx0, pos+dx1});
    }
}
#include "Scene.hpp"
#include "SessionManager.hpp"
#include "bookmark.hpp"
#include "gui/theme.hpp"
#include "nodes.hpp"

#include <QPainter>


using namespace core;

namespace
{
    QColor invert(const QColor& color)
    {
        auto H = 1.0 - color.hueF();
        if (std::abs(H) > 1.0) { H = 0.65; }

        const auto S = 1.0 - color.saturationF();
        const auto V = 1.0 - color.valueF();

        return QColor::fromHsvF(H, S, V);
    }
}

Scene::Scene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRect(-7680 * 4, -4320 * 4, 7680 * 8, 4320 * 8));

    connect(session()->tm(), &gui::ThemeManager::bgColorChanged, this,
        [this] { update(sceneRect()); });
    connect(session()->tm(), &gui::ThemeManager::resetTriggered, this,
        [this] { update(sceneRect()); });

    for (const auto& [pos, name] : session()->bm()->sceneBookmarksAsList()) {
        addItem(new nodes::SceneBookmarkItem{pos, name});
    }
}

void Scene::addSceneBookmark(const QPoint& pos, const QString& name)
{
    auto* bm = session()->bm();

    if (const auto data = SceneBookmarkData{pos, name}; !bm->sceneBookmarks().contains(data)) {
        session()->bm()->insertBookmark(data);
        addItem(new nodes::SceneBookmarkItem(pos, name, true));
    }
}

void Scene::drawBackground(QPainter *p, const QRectF& rec)
{
    p->save();
    const auto bgColor = session()->tm()->bgColor();
    p->fillRect(rec, bgColor);
    drawCrosses(p, rec);
    p->restore();
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

    const auto bgColor = session()->tm()->bgColor();
    const auto fgColor = invert(bgColor);

    p->setPen(QPen(fgColor, 1));
    for (qreal x = x0; x < x1; x += grid) {
        for (qreal y = y0; y < y1; y += grid) {
            const auto pos = QPointF{x, y};
            p->drawLine({pos+dy0, pos+dy1});
            p->drawLine({pos+dx0, pos+dx1});
        }
    }

    if (rec.contains(QPoint{0, 0})) {
        p->setPen(QPen(fgColor, 2));
        const auto pos = QPointF{0, 0};
        p->drawLine({pos+dy0, pos+dy1});
        p->drawLine({pos+dx0, pos+dx1});
    }
}
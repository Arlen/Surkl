#include "Scene.hpp"
#include "SessionManager.hpp"
#include "bookmark.hpp"
#include "db.hpp"
#include "gui/theme.hpp"
#include "gui/view/Inode.hpp"
#include "nodes.hpp"

#include <QFileSystemModel>
#include <QPainter>

#include <ranges>


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

    void drawCrosses(QPainter* p, const QRectF& rec)
    {
        p->save();
        constexpr qreal grid = 512;

        /// without making the rec larger by 8 on each side, sometimes tearing
        /// is produced when items move in the scene.  Since the cross has a
        /// radius of 8, we need to make sure that during each pass a full
        /// cross is drawn.
        const auto l = rec.left()   - 8;
        const auto r = rec.right()  + 8;
        const auto t = rec.top()    - 8;
        const auto b = rec.bottom() + 8;

        const auto x0 = std::floor(std::abs(l) / grid) * grid * (std::signbit(l) ? -1.0 : 1.0) - grid;
        const auto x1 = std::floor(std::abs(r) / grid) * grid * (std::signbit(r) ? -1.0 : 1.0) + grid;
        const auto y0 = std::floor(std::abs(t) / grid) * grid * (std::signbit(t) ? -1.0 : 1.0) - grid;
        const auto y1 = std::floor(std::abs(b) / grid) * grid * (std::signbit(b) ? -1.0 : 1.0) + grid;

        constexpr auto dy0 = QPointF{ 0,-8};
        constexpr auto dy1 = QPointF{ 0, 8};
        constexpr auto dx0 = QPointF{-8, 0};
        constexpr auto dx1 = QPointF{ 8, 0};

        const auto bgColor = SessionManager::tm()->bgColor();
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
        p->restore();
    }


    void drawBorder(QPainter* p, const QRectF& viewRec, const QRectF& sceneRec)
    {
        p->save();
        p->setPen(Qt::NoPen);
        constexpr auto borderThickness = 16.0;
        constexpr auto borderSize      = 128.0;
        constexpr auto increment       = borderSize*2.0;

        /// viewRec is the rect() of the QGraphicsView widget mapped to the scene; it
        /// can cover the entire scene rect, in theory.  But it's usually smaller than
        /// the scene rect.
        /// sceneRec is the entire scene rect.
        const auto center   = viewRec.center();

        /// The view rect can actually be larger than the scene rect, and we only
        /// want to draw a border around where the scene rect ends.
        /// With current scene size and max zoom range, the view rect will never
        /// get larger than the scene rect, but in any case, only draw borders
        /// around the scene rect.
        const auto l = std::max(viewRec.left(), sceneRec.left());
        const auto r = std::min(viewRec.right(), sceneRec.right());
        const auto t = std::max(viewRec.top(), sceneRec.top());
        const auto b = std::min(viewRec.bottom(), sceneRec.bottom());

        const auto x0 = std::ceil(std::abs(l) / borderSize) * borderSize * (std::signbit(l) ? -1.0 : 1.0);
        const auto x1 = std::ceil(std::abs(r) / borderSize) * borderSize * (std::signbit(r) ? -1.0 : 1.0);
        const auto y0 = std::ceil(std::abs(t) / borderSize) * borderSize * (std::signbit(t) ? -1.0 : 1.0);
        const auto y1 = std::ceil(std::abs(b) / borderSize) * borderSize * (std::signbit(b) ? -1.0 : 1.0);

        const bool x0IsEven = int(std::abs(x0)/borderSize) % 2;
        const bool y0IsEven = int(std::abs(y0)/borderSize) % 2;

        auto doDrawH = [p](qreal start, qreal end, qreal y, qreal w, qreal h, const QColor& color)
        {
            p->setBrush(color);
            for (qreal x = start; x < end; x += increment) {
                p->drawRect(QRectF(x, y, w, h));
            }
        };

        if (viewRec.contains(QPointF(center.x(), sceneRec.top() + borderThickness))) {
            doDrawH
            ( x0
            , x1
            , sceneRec.top()
            , borderSize
            , borderThickness
            , x0IsEven ? Qt::black : Qt::white
            );

            doDrawH
            ( x0+borderSize
            , x1
            , sceneRec.top()
            , borderSize
            , borderThickness
            , x0IsEven ? Qt::white : Qt::black
            );
        }

        if (viewRec.contains(QPointF(center.x(), sceneRec.bottom() - borderThickness))) {
            doDrawH
            ( x0
            , x1
            , sceneRec.bottom()
            , borderSize
            , -borderThickness
            , x0IsEven ? Qt::white : Qt::black
            );

            doDrawH
            ( x0+borderSize
            , x1
            , sceneRec.bottom()
            , borderSize
            , -borderThickness
            , x0IsEven ? Qt::black : Qt::white
            );
        }

        auto doDrawV = [p](qreal start, qreal end, qreal x, qreal w, qreal h, const QColor& color)
        {
            p->setBrush(color);
            for (qreal y = start; y < end; y += increment) {
                p->drawRect(QRectF(x, y, w, h));
            }
        };

        if (viewRec.contains(QPointF(sceneRec.left() + borderThickness, center.y()))) {
            doDrawV
            ( y0
            , y1
            , sceneRec.left()
            , borderThickness
            , borderSize
            , y0IsEven ? Qt::black : Qt::white
            );

            doDrawV
            ( y0+borderSize
            , y1
            , sceneRec.left()
            , borderThickness
            , borderSize
            , y0IsEven ? Qt::white : Qt::black
            );
        }

        if (viewRec.contains(QPointF(sceneRec.right() - borderThickness, center.y()))) {
            doDrawV
            ( y0
            , y1
            , sceneRec.right()
            , -borderThickness
            , borderSize
            , y0IsEven ? Qt::white : Qt::black
            );

            doDrawV
            ( y0+borderSize
            , y1
            , sceneRec.right()
            , -borderThickness
            , borderSize
            , y0IsEven ? Qt::black : Qt::white
            );
        }

        p->restore();
    }

    auto notNull = [](auto* item) -> bool { return item != nullptr; };
    auto toNode = [](QGraphicsItem* item) -> gui::view::Inode*
        { return qgraphicsitem_cast<gui::view::Inode*>(item); };
    auto toEdge = [](QGraphicsItem* item) -> gui::view::InodeEdge*
    { return qgraphicsitem_cast<gui::view::InodeEdge*>(item); };

    auto filterNodes = std::views::transform(toNode) | std::views::filter(notNull);
    auto filterEdges = std::views::transform(toEdge) | std::views::filter(notNull);
}


void FileSystemScene::configure(FileSystemScene* scene)
{
    if (auto db = db::get(); false && db.isOpen()) {
        /// how do we save and restore the scene???
    } else {
        auto* n1 = gui::view::Inode::createRoot(scene);
        n1->open();
    }
}

FileSystemScene::FileSystemScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRect(-1024 * 32, -1024 * 32, 1024 * 64, 1024 * 64));

    _model = new QFileSystemModel(this);
    _model->setRootPath("/");
    _model->fetchMore(QModelIndex());

    connect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);
    connect(_model, &QAbstractItemModel::rowsInserted, this, &FileSystemScene::onRowsInserted);
    connect(_model, &QAbstractItemModel::rowsRemoved, this, &FileSystemScene::onRowsRemoved);

    connect(session()->tm(), &gui::ThemeManager::bgColorChanged, this,
        [this] { update(sceneRect()); });
    connect(session()->tm(), &gui::ThemeManager::resetTriggered, this,
        [this] { update(sceneRect()); });

    for (const auto& [pos, name] : session()->bm()->sceneBookmarksAsList()) {
        addItem(new nodes::SceneBookmarkItem{pos, name});
    }
}

FileSystemScene::~FileSystemScene()
{
    /// Need to disconnect this because, otherwise, if there is a selected node
    /// before exit, then we get a runtime error:
    /// "downcast of address 0x5cedde11db10 which does not point to an object
    /// of type 'typename FuncType::Object' (aka 'core::Scene')"
    disconnect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);
}

void FileSystemScene::addSceneBookmark(const QPoint& pos, const QString& name)
{
    auto* bm = SessionManager::bm();
    assert(bm != nullptr);

    if (const auto data = SceneBookmarkData{pos, name}; !bm->sceneBookmarks().contains(data)) {
        bm->insertBookmark(data);
        addItem(new nodes::SceneBookmarkItem(pos, name, true));
    }
}

QPersistentModelIndex FileSystemScene::rootIndex() const
{
    return _model->index(QDir::rootPath());
}

void FileSystemScene::openSelectedNodes() const
{
    for (const auto selection = selectedItems(); auto* node : selection | filterNodes) {
        node->open();
    }
}

void FileSystemScene::closeSelectedNodes() const
{
    for (const auto selection = selectedItems(); auto* node : selection | filterNodes) {
        node->closeOrHalfClose();
    }
}

void FileSystemScene::halfCloseSelectedNodes() const
{
    for (const auto selection = selectedItems(); auto* node : selection | filterNodes) {
        node->closeOrHalfClose(true);
    }
}

void FileSystemScene::onRowsInserted(const QModelIndex& parent, int start, int end) const
{
    for (const auto allItems = items(); auto* node : allItems | filterNodes) {
        if (node->index() == parent) {
            node->reload(start, end);
        }
    }
}

void FileSystemScene::onRowsRemoved(const QModelIndex& parent, int start, int end) const
{
    for (const auto allItems = items(); auto* node : allItems | filterNodes) {
        if (node->index() == parent) {
            node->unload(start, end);
        }
    }
}

void FileSystemScene::drawBackground(QPainter *p, const QRectF& rec)
{
    p->save();
    const auto bgColor = SessionManager::tm()->bgColor();
    p->fillRect(rec, bgColor);
    drawCrosses(p, rec);
    drawBorder(p, rec, sceneRect());
    p->restore();
}

void FileSystemScene::onSelectionChange()
{
    disconnect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);

    const auto selected = selectedItems();
    const auto nodes    = std::ranges::to<std::vector>(selected | filterNodes);
    const auto edges    = std::ranges::to<std::vector>(selected | filterEdges);

    /// for now, we'll give nodes priority over edges.
    if ((!nodes.empty() && !edges.empty()) || edges.empty()) {
        for (auto* edge : edges) {
            edge->setSelected(false);
        }
        for (auto* node : nodes) {
            _model->fetchMore(node->index());
        }
    } else {
        for (auto* nodes : nodes) {
            nodes->setSelected(false);
        }
    }

    connect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);
}
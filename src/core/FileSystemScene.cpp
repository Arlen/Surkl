/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "EdgeItem.hpp"
#include "FileSystemScene.hpp"
#include "GraphicsView.hpp"
#include "NodeItem.hpp"
#include "SessionManager.hpp"
#include "bookmark.hpp"
#include "db.hpp"
#include "gui/theme.hpp"
#include "nodes.hpp"

#include <QDesktopServices>
#include <QFileSystemModel>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QUrl>

#include <ranges>


using namespace core;

namespace
{
    void drawCrosshairs(QPainter* p, const QRectF& rec)
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

        const auto fgColor = SessionManager::tm()->sceneFgColor();

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

    auto notNull = [](auto* item) -> bool
    {
        return item != nullptr;
    };
    auto toNode = [](QGraphicsItem* item) -> NodeItem*
    {
        return qgraphicsitem_cast<NodeItem*>(item);
    };
    auto toEdge = [](QGraphicsItem* item) -> EdgeItem*
    {
        return qgraphicsitem_cast<EdgeItem*>(item);
    };

    auto filterNodes = std::views::transform(toNode) | std::views::filter(notNull);
    auto filterEdges = std::views::transform(toEdge) | std::views::filter(notNull);
}

void FileSystemScene::configure(FileSystemScene* scene)
{
    if (auto db = db::get(); false && db.isOpen()) {
        /// how do we save and restore the scene???
    } else {
        auto* n1 = Node::createRootNode(scene);
    }
}

FileSystemScene::FileSystemScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRect(-1024 * 32, -1024 * 32, 1024 * 64, 1024 * 64));

    _model = new QFileSystemModel(this);
    _model->setRootPath(QDir::rootPath());

    _proxyModel = new QSortFilterProxyModel(this);
    _proxyModel->setSourceModel(_model);
    _proxyModel->setDynamicSortFilter(true);
    _proxyModel->sort(0);

    connect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);
    connect(_proxyModel, &QAbstractItemModel::rowsInserted, this, &FileSystemScene::onRowsInserted);
    connect(_proxyModel, &QAbstractItemModel::rowsRemoved, this, &FileSystemScene::onRowsRemoved);

    for (const auto& [pos, name] : SessionManager::bm()->sceneBookmarksAsList()) {
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
    auto index = _model->index(_model->rootPath());

    return _proxyModel->mapFromSource(index);
}

bool FileSystemScene::isDir(const QModelIndex& index) const
{
    return _model->isDir(_proxyModel->mapToSource(index));
}

void FileSystemScene::setRootPath(const QString& newPath) const
{
    _model->setRootPath(newPath);
}

bool FileSystemScene::openFile(const NodeItem* node) const
{
    bool success = false;
    if (const auto& index = node->index(); index.isValid()) {
        Q_ASSERT(!isDir(index));
        const auto info = _model->filePath(_proxyModel->mapToSource(index));
        success = QDesktopServices::openUrl(QUrl::fromLocalFile(info));
    }
    return success;
}

void FileSystemScene::openSelectedNodes() const
{
    for (const auto selection = selectedItems(); auto* node : selection | filterNodes) {
        if (isDir(node->index())) {
            node->open();
        } else {
            openFile(node);
        }
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

/// updates the items in the scene after a change in ThemeManager for changes to
/// take effect.
void FileSystemScene::refreshItems()
{
    const auto* tm = SessionManager::tm();

    for (const auto xs = items(); auto* x : xs) {
        if (auto* label = qgraphicsitem_cast<EdgeLabelItem*>(x); label) {
            /// Need to set the brush for EdgeLabels directly. Other item types
            /// don't have this problem because theme values are used directly
            /// in paint(), and update() triggers a repaint.
            label->setBrush(tm->edgeTextColor());
        }
    }

    /// other item types.
    update();
}

void FileSystemScene::onRowsInserted(const QModelIndex& parent, int start, int end) const
{
    for (const auto _items = items(); auto* node : _items | filterNodes) {
        if (node->index() == parent) {
            node->reload(start, end);
        }
    }
}

void FileSystemScene::onRowsRemoved(const QModelIndex& parent, int start, int end) const
{
    std::vector<NodeItem*> toBeUnloaded;

    /// can't call Node::unload() while traversing items() because Node::unload()
    /// deletes child nodes and subtrees of items still in items().
    for (const auto _items = items(); auto* node : _items | filterNodes) {
        if (node->index() == parent) {
            toBeUnloaded.push_back(node);
        }
    }

    for (auto* node : toBeUnloaded) {
        node->unload(start, end);
    }
}

void FileSystemScene::drawBackground(QPainter *p, const QRectF& rec)
{
    p->fillRect(rec, SessionManager::tm()->sceneBgColor());
    drawCrosshairs(p, rec);
    drawBorder(p, rec, sceneRect());
}

void FileSystemScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    return QGraphicsScene::mouseDoubleClickEvent(e);
}

void FileSystemScene::onSelectionChange()
{
    disconnect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);

    const auto selected = selectedItems();
    const auto nodes    = std::ranges::to<std::vector>(selected | filterNodes);
    const auto edges    = std::ranges::to<std::vector>(selected | filterEdges);

    /// for now, give nodes priority over edges.
    if ((!nodes.empty() && !edges.empty()) || edges.empty()) {
        for (auto* edge : edges) {
            edge->setSelected(false);
        }
    } else {
        for (auto* node : nodes) {
            node->setSelected(false);
        }
    }

    connect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);
}

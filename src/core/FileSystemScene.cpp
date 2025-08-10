/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileSystemScene.hpp"
#include "BookmarkItem.hpp"
#include "DeleteDialog.hpp"
#include "EdgeItem.hpp"
#include "GraphicsView.hpp"
#include "NodeItem.hpp"
#include "SessionManager.hpp"
#include "bookmark.hpp"
#include "gui/InfoBar.hpp"
#include "gui/theme/theme.hpp"

#include <QDesktopServices>
#include <QFileSystemModel>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMetaEnum>
#include <QMimeData>
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

        const auto fgColor = SessionManager::tm()->sceneColor();

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
            p->drawLine({dy0, dy1});
            p->drawLine({dx0, dx1});
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

        const bool x0IsEven = static_cast<int>(std::abs(x0)/borderSize) % 2;
        const bool y0IsEven = static_cast<int>(std::abs(y0)/borderSize) % 2;

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

    DeletionDialog* createDeleteDialog(const QGraphicsScene* scene)
    {
        for (auto* view: scene->views()) {
            if (view->hasFocus()) {
                auto* dialog = new DeletionDialog(view);
                return dialog;
            }
        }

        return nullptr;
    }
}

FileSystemScene::FileSystemScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRect(-1024 * 32, -1024 * 32, 1024 * 64, 1024 * 64));

    _model = new QFileSystemModel(this);
    _model->setRootPath(QDir::rootPath());
    _model->setReadOnly(true);

    _proxyModel = new QSortFilterProxyModel(this);
    _proxyModel->setSourceModel(_model);
    _proxyModel->setDynamicSortFilter(true);
    _proxyModel->sort(0);

    connect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);

    connect(_proxyModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &FileSystemScene::onRowsAboutToBeRemoved);
    connect(_proxyModel, &QAbstractItemModel::rowsInserted, this, &FileSystemScene::onRowsInserted);
    connect(_proxyModel, &QAbstractItemModel::rowsRemoved, this, &FileSystemScene::onRowsRemoved);

    for (const auto& [pos, name] : SessionManager::bm()->sceneBookmarksAsList()) {
        auto* bookmarkItem = new SceneBookmarkItem(QPoint(0, 0), name);
        addItem(bookmarkItem);
        bookmarkItem->setPos(pos);
    }
}

QPersistentModelIndex FileSystemScene::rootIndex() const
{
    auto index = _model->index(_model->rootPath());

    return _proxyModel->mapFromSource(index);
}

bool FileSystemScene::isDir(const QModelIndex& index) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.model() == _proxyModel);

    return _model->isDir(_proxyModel->mapToSource(index));
}

bool FileSystemScene::isLink(const QModelIndex& index) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.model() == _proxyModel);

    return _model->fileInfo(_proxyModel->mapToSource(index)).isSymbolicLink();
}

QString FileSystemScene::filePath(const QPersistentModelIndex& index) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.model() == _proxyModel);

    return _model->filePath(_proxyModel->mapToSource(index));
}

QPersistentModelIndex FileSystemScene::index(const QString& path) const
{
    return _proxyModel->mapFromSource(_model->index(path));
}

bool FileSystemScene::isReadOnly() const
{
    return _model->isReadOnly();
}

void FileSystemScene::setRootPath(const QString& newPath) const
{
    _model->setRootPath(newPath);
}

void FileSystemScene::openTo(const QString &targetPath) const
{
    auto idx = index(QDir::rootPath());

    if (auto* root = nodeFromIndex(idx); root) {
        if (root->isClosed()) {
            root->open();
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        auto dirs = targetPath.split(QDir::separator(), Qt::SkipEmptyParts);
        auto* parent = root;
        QString subPath;

        while (!dirs.empty()) {
            subPath += QDir::separator() + dirs.first();
            dirs.pop_front();
            idx = index(subPath);
            parent->skipTo(idx.row());

            /// TODO: instnead of nodeFromIndex(), it would be cheaper to have
            /// the parent get the child node from an index.  Or just refactore
            /// NodeItem::skipTo() to return the node?
            if (auto* nextNode = nodeFromIndex(idx); nextNode) {
                parent = nextNode;
                if (parent->isClosed()) {
                    parent->open();
                    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                }
                continue;
            }

            return;
        }
    }
}

void FileSystemScene::fetchMore(const QPersistentModelIndex& index) const
{
    if (_proxyModel->canFetchMore(index)) {
        _proxyModel->fetchMore(index);
    }
}

qint64 FileSystemScene::fileSize(const QPersistentModelIndex& index) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.model() == _proxyModel);

    return _model->size(_proxyModel->mapToSource(index));
}

void FileSystemScene::openSelectedNodes() const
{
    for (const auto selection = selectedItems(); auto* node : selection | filterNodes) {
        if (isDir(node->index())) {
            node->open();
        } else {
            const auto ok = openFile(node);
            Q_UNUSED(ok);
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

void FileSystemScene::addSceneBookmark(const QPoint& clickPos, const QString& name)
{
    auto* bm = SessionManager::bm();

    auto* bookmarkItem      = new SceneBookmarkItem(clickPos, name);
    const auto itemPos      = bookmarkItem->scenePos().toPoint();
    const auto bookmarkData = SceneBookmarkData{itemPos, name};

    if (const auto bookmarks = bm->sceneBookmarks(); !bookmarks.contains(bookmarkData)) {
        bm->insertBookmark(bookmarkData);
        addItem(bookmarkItem);
    } else {
        delete bookmarkItem;
    }
}

void FileSystemScene::toggleReadOnly()
{
    _model->setReadOnly(!_model->isReadOnly());

    const auto enabled = _model->isReadOnly();
    const auto msg = QString("Real-Only Mode: %1").arg(enabled ? "On":"Off");
    SessionManager::ib()->setTimedMsgL(msg, 2000);

    emit readOnlyToggled(_model->isReadOnly());
}

void FileSystemScene::drawBackground(QPainter *p, const QRectF& rec)
{
    p->fillRect(rec, SessionManager::tm()->sceneMidarkColor());
    drawCrosshairs(p, rec);
    drawBorder(p, rec, sceneRect());
}

void FileSystemScene::keyPressEvent(QKeyEvent *event)
{
    const auto key = event->key();
    const auto mod = event->modifiers();

    if (key == Qt::Key_Delete) {
        if (!event->isAutoRepeat()) {
            auto selection = selectedItems() | filterNodes;
            if (std::ranges::distance(selection) > 0) {
                auto* dialog = createDeleteDialog(this);
                connect(dialog, &QDialog::accepted, this, &FileSystemScene::deleteSelection);
                dialog->exec();
            } else {
                deleteSelection();
            }
        }
    } else if (key == Qt::Key_A) {
        rotateSelection(Rotation::CCW, event->modifiers() == Qt::ShiftModifier);
    } else if (key == Qt::Key_D) {
        rotateSelection(Rotation::CW, event->modifiers() == Qt::ShiftModifier);
    } else if (key == Qt::Key_Plus || key == Qt::Key_Minus) {
        auto amount = mod & Qt::ShiftModifier ? 10 : 2;
        amount *= key == Qt::Key_Minus ? -1 : 1;
        const auto selection     = selectedItems();
        const auto selectedNodes = std::ranges::to<QList>(selection | filterNodes);
        for (auto* node : selectedNodes) {
            if (!node->childEdges().empty()) {
                node->growChildren(amount);
            } else {
                node->grow(amount);
            }
        }
    }

    QGraphicsScene::keyPressEvent(event);
}

void FileSystemScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (auto* item = itemAt(event->scenePos(), QTransform()); item && event->button() == Qt::LeftButton) {
        if (auto* node = asNodeItem(item)) {
            if (node->isFile() && openFile(node)) {
                node->setSelected(false);
                return;
            }
            if (node->isOpen()) {
                node->closeOrHalfClose(event->modifiers() & Qt::ShiftModifier);
            } else if (node->isClosed() || node->isHalfClosed()) {
                node->open();
            }
        }
    }

    QGraphicsScene::mouseDoubleClickEvent(event);
}

void FileSystemScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        for (const auto pos = event->scenePos(); auto* edge : _selectedEdges) {
            edge->adjustSourceTo(pos);
            edge->target()->update();
        }
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void FileSystemScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    auto performDropAction = [&] {
        auto destination = items(event->scenePos()) | filterNodes | std::views::take(1);

        if (destination.empty()) { return; }

        const auto* destinationNode = destination.front();

        const auto sourceIndices = _selectedEdges
            | asTargetNodeIndex
            | std::views::transform(
                [this](const QModelIndex& i) {
                    return _proxyModel->mapToSource(i);
                })
            | std::ranges::to<QModelIndexList>();

        if (sourceIndices.empty()) { return; }

        const auto* sourceData = _model->mimeData(sourceIndices);

        if (!sourceData) { return; }

        const auto destinationIndex = _proxyModel->mapToSource(destinationNode->index());

        auto action = Qt::MoveAction;
        if (event->modifiers() == Qt::ControlModifier) {
            action = Qt::CopyAction;
        } else if (event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier) {
            action = Qt::LinkAction;
        }

        /// TODO: dropMimeData does not support directory copy.
        if (_model->dropMimeData(sourceData, action, -1, -1, destinationIndex)) {
            adjustAllEdges(destinationNode);
        } else {
            const auto actionName =
                QString(QMetaEnum::fromType<decltype(action)>().valueToKey(action))
                    .remove("Action");
            SessionManager::ib()->setTimedMsgL(QString("%1 failed!").arg(actionName), 3000);
        }
    };

    if (event->button() & Qt::LeftButton && !_selectedEdges.isEmpty()) {

        performDropAction();

        for (auto* edge : _selectedEdges) {
            edge->adjust();
            edge->target()->update();
        }
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void FileSystemScene::onRowsInserted(const QModelIndex& parent, int start, int end) const
{
    for (const auto _items = items(); auto* node : _items | filterNodes) {
        if (node->index() == parent) {
            node->onRowsInserted(start, end);
            break;
        }
    }

    reportStats();
}

void FileSystemScene::onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) const
{
    for (const auto nodes = items(); auto* node : nodes | filterNodes) {
        if (node->index() == parent) {
            node->onRowsAboutToBeRemoved(start, end);
        }
    }
}

void FileSystemScene::onRowsRemoved(const QModelIndex& parent, int start, int end) const
{
    std::vector<NodeItem*> parentNodes;

    /// can't call Node::unload() while traversing items() because Node::unload()
    /// deletes child nodes and subtrees of items still in items().
    for (const auto nodes = items(); auto* node : nodes | filterNodes) {
        if (node->index() == parent) {
            parentNodes.push_back(node);
        }
    }

    for (auto* node : parentNodes) {
        node->onRowsRemoved(start, end);
    }

    reportStats();
}

void FileSystemScene::onSelectionChange()
{
    disconnect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);

    const auto selection     = selectedItems();
    const auto selectedNodes = std::ranges::to<QList>(selection | filterNodes);
    const auto selectedEdges = std::ranges::to<QList>(selection | filterEdges);

    _selectedEdges.clear();

    /// for now, give nodes priority over edges.
    if ((!selectedNodes.empty() && !selectedEdges.empty()) || selectedEdges.empty()) {
        for (auto* edge : selectedEdges) {
            edge->setSelected(false);
        }
    } else {
        for (auto* node : selectedNodes) {
            node->setSelected(false);
        }
        _selectedEdges = selectedEdges;
    }

    if (selectedNodes.size() > 0) {
        reportStats();
    } else {
        SessionManager::ib()->clear();
    }

    connect(this, &QGraphicsScene::selectionChanged, this, &FileSystemScene::onSelectionChange);
}

bool FileSystemScene::openFile(const NodeItem* node) const
{
    if (const auto& index = node->index(); index.isValid()) {
        Q_ASSERT(!isDir(index));
        Q_ASSERT(index.model() == _proxyModel);

        const auto info = _model->filePath(_proxyModel->mapToSource(index));

        return QDesktopServices::openUrl(QUrl::fromLocalFile(info));
    }

    return false;
}

void FileSystemScene::deleteSelection()
{
    using namespace std::placeholders;

    const auto selection = selectedItems();

    /// 1. remove files and folders
    auto indices = selection
        | std::views::filter(&asNodeItem)
        | std::views::transform(&asNodeItem)
        | asIndex
        | std::views::transform(std::bind(&QSortFilterProxyModel::mapToSource, _proxyModel, _1))
        | std::views::filter(&QModelIndex::isValid)
        ;

    for (auto i : indices) {
        _model->remove(i);
    }


    /// 2. remove bookmarks
    auto* bm = SessionManager::bm();

    auto bookmarkItems = selection
        | std::views::filter([](QGraphicsItem* item)
            { return qgraphicsitem_cast<SceneBookmarkItem*>(item); })
        ;

    QList<SceneBookmarkData> data;
    for (auto* item : bookmarkItems) {
        data.push_back(SceneBookmarkData(item->scenePos().toPoint(), {}));
        removeItem(item);
        delete item;
    }
    bm->removeBookmarks(data);
}

void FileSystemScene::rotateSelection(Rotation rot, bool page) const
{
    auto nodes = selectedItems() | filterNodes;

    for (auto* n : nodes) {
        fetchMore(n->index());
    }

    if (page) {
        for (auto* n : nodes) {
            n->rotatePage(rot);
        }
    } else {
        for (auto* n : nodes) {
            n->rotate(rot);
        }
    }
}

QString FileSystemScene::gatherStats(const QModelIndexList& indices) const
{
    auto locale = QLocale::system();

    if (indices.size() == 1) {
        const auto& fi = _model->fileInfo(indices.first());
        if (fi.isDir()) {
            auto dir = fi.dir();
            dir.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
            dir.cd(fi.baseName());

            return QString("\"%1\" selected (containing %2 items)")
                        .arg(fi.isRoot() ? QDir::separator() : fi.fileName())
                        .arg(dir.count());
        }
        return QString("\"%1\" selected (%2)")
                    .arg(fi.fileName())
                    .arg(locale.formattedDataSize(fi.size()));
    }

    qint64 selectedFolders = 0;
    qint64 folderCount = 0;
    qint64 selectedItems = 0;
    qint64 fileBytes = 0;

    auto dir = QDir();
    dir.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

    for (const auto& index : indices) {
        if (const auto& fi = _model->fileInfo(index); fi.isDir()) {
            selectedFolders++;
            dir.setPath(fi.absoluteFilePath());
            folderCount += dir.count();
        } else {
            selectedItems++;
            fileBytes += fi.size();
        }
    }

    const auto comma         = selectedFolders && selectedItems ? ", " : "";
    const auto other         = selectedItems ? "other" : "";
    const auto aTotalOf      = selectedFolders > 1 ? "a total of" : "";
    const auto folder        = selectedFolders > 1 ? "folders" : "folder";
    const auto folderItems   = folderCount == 1 ? "item" : "items";
    const auto fileItems     = selectedItems == 1 ? "item" : "items";
    const auto formattedSize = locale.formattedDataSize(fileBytes);

    auto msg = QString();
    if (selectedFolders > 0) {
        msg += QString("%1 %2 selected (containing %3 %4 %5)")
                    .arg(selectedFolders)
                    .arg(folder)
                    .arg(aTotalOf)
                    .arg(folderCount)
                    .arg(folderItems);
    }

    msg += comma;

    if (selectedItems > 0) {
        msg += QString("%1 %2 %3 selected (%4)")
                .arg(selectedItems)
                .arg(other)
                .arg(fileItems)
                .arg(formattedSize);
    }

    return msg;
}

void FileSystemScene::reportStats() const
{
    const auto indices = selectedItems()
        | filterNodes
        | asIndex
        | std::views::transform(
            [this](const QPersistentModelIndex& i) {
                return _proxyModel->mapToSource(i);
            })
        | std::ranges::to<QList>()
        ;

    SessionManager::ib()->setMsgR(gatherStats(indices));
}

NodeItem* FileSystemScene::nodeFromIndex(const QModelIndex& index) const
{
    for (const auto nodes = items(); auto* node : nodes | filterNodes) {
        if (node->index() == index) {
            return node;
        }
    }

    return nullptr;
}

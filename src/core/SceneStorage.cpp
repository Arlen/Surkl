/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SceneStorage.hpp"
#include "FileSystemScene.hpp"
#include "NodeItem.hpp"
#include "SessionManager.hpp"
#include "db.hpp"
#include "db/stmt.hpp"

#include <QDir>
#include <QSqlRecord>
#include <QTimer>


using namespace core;

namespace
{
    void skipToFirstRow(QList<NodeData>& data, int firstRow)
    {
        using namespace std;

        const auto isFirstRow = [firstRow](const NodeData& nd) -> bool
        {
            return nd.index.row() == firstRow;
        };

        if (firstRow != -1) {
            if (auto found = ranges::find_if(data, isFirstRow); found != data.end()) {
                QList<NodeData> r, e;
                while (data.first().index.row() != firstRow) {
                    const auto first = data.takeFirst();
                    if  (NodeFlags(first.type).testAnyFlag(NodeType::OpenNode)
                            || NodeFlags(first.type).testAnyFlag(NodeType::HalfClosedNode)) {
                        r.push_back(first);
                    } else {
                        e.push_back(first);
                    }
                }
                data = r + data + e;
            }
        }
    }
}

SceneStorage::SceneStorage(QObject* parent)
    : QObject(parent)
{
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, this, &SceneStorage::nextN);
}

void SceneStorage::configure()
{
    createTable();

    Q_ASSERT(core::db::doesTableExists(stmt::scene::NODES_TABLE));
}

void SceneStorage::deleteNode(const NodeItem *node)
{
    Q_ASSERT(node != nullptr);

    const auto id = _scene->filePath(node->index());
    const auto isDir = _scene->isDir(node->index());

    _queue.push_back(QVariant::fromValue<DeleteData>({id, isDir}));
    _timer->start(125);
}

void SceneStorage::saveNode(const NodeItem *node)
{
    if (!_enabled || !node->index().isValid()) {
        return;
    }

    Q_ASSERT(node != nullptr);

    const auto id       = _scene->filePath(node->index());
    const auto nodeType = static_cast<int>(node->nodeFlags());
    const auto firstRow = node->firstRow();
    const auto pos      = node->scenePos();
    const auto length   = node->parentEdge()->line().length();
    const auto saveData = SaveData{id, nodeType, firstRow, pos, length};

    Q_ASSERT(!id.isEmpty());

    auto match = [id](const QVariant& v) -> bool {
        if (v.canConvert<SaveData>()) {
            return id == v.value<SaveData>().id;
        }
        return false;
    };

    if (auto found = std::ranges::find_if(_queue, match); found != _queue.end()) {
        *found = QVariant::fromValue<SaveData>(saveData);
    } else {
        _queue.push_back(QVariant::fromValue<SaveData>(saveData));
    }

    _timer->start(125);
}

void SceneStorage::saveScene() const
{
    const auto toBeSaved = _scene->items()
        | std::views::filter(&asNodeItem)
        | std::views::transform(&asNodeItem)
        | std::ranges::to<QList<const NodeItem*>>()
        ;

    saveNodes(toBeSaved);
}

void SceneStorage::loadScene(FileSystemScene* scene)
{
    Q_ASSERT(_scene == nullptr);
    _scene = scene;

    auto G = readTable(scene);

    if (G.empty()) {
        auto* edge = NodeItem::createRootNode(scene->rootIndex());
        scene->addItem(edge->source());
        scene->addItem(edge->target());
        scene->addItem(edge);
        edge->adjust();

        scene->openTo(QDir::homePath());
        enableStorage();
        return;
    }

    auto sortByRows = [](QList<NodeData>& data) {
        std::ranges::sort(data, [](const NodeData& a, const NodeData& b) {
            return a.index.row() < b.index.row();
        });
    };

    QList<NodeData> S;
    if (G.contains(QPersistentModelIndex())) {
        /// This assumes we have only a single root node ("/").

        auto& Ms = G[QPersistentModelIndex()];
        sortByRows(Ms);
        Q_ASSERT(Ms.size() == 1);

        for (auto& m : Ms) {
            Q_ASSERT(m.index.isValid());
            m.edge = NodeItem::createRootNode(m.index);
            scene->addItem(m.edge->source());
            scene->addItem(m.edge->target());
            scene->addItem(m.edge);
            m.edge->target()->setPos(m.pos);
            m.edge->adjust();
            scene->fetchMore(m.index);
            S.push_back(m);
        }
    }
    /// Minor Bug: if the DB contains only the root ("/") directory, after
    /// loadScene() finishes, FileSystemScene::onRowsInserted is called, which
    /// then calls NodeItem::reload() and that causes the root node to open.
    /// This only happens with the root ("/") node; it's a minor issue, and not
    /// sure if it's worth fixing.
    /// One solution is to make the FileSystemScene::onRowsInserted connection
    /// after SceneStorage::loadScene() and with a small delay.  The delay is
    /// to give the event loop a chance to catch up and the connection is made
    /// after  NodeItem::reload().

    QList<NodeItem*> halfOpenNodes;
    while (!S.empty()) {
        const auto parent = S.back();
        S.pop_back();
        Q_ASSERT(parent.edge != nullptr);

        auto* parentNode = asNodeItem(parent.edge->target());

        /// if graph does not contain parent.index, then parent is a closed (leaf) node.
        if (G.contains(parent.index)) {
            auto& childNodeData = G[parent.index];
            if (!childNodeData.empty()) {
                sortByRows(childNodeData);
                skipToFirstRow(childNodeData, parent.firstRow);
                parentNode->createChildNodes(childNodeData);
                parent.edge->adjust();
                scene->fetchMore(parent.index);
            }
            for (const auto& nd : childNodeData) {
                if (nd.edge) {
                    S.push_back(nd);
                } else {
                    /// TODO: nd can be removed from DB.
                }
            }
        }
        parentNode->setPos(parent.pos);

        if (NodeFlags(parent.type).testAnyFlag(NodeType::HalfClosedNode)) {
            halfOpenNodes.push_back(parentNode);
        }
    }

    for (auto* node : halfOpenNodes) {
        node->halfClose();
    }
    enableStorage();
}

void SceneStorage::nextN()
{
    const auto batchSize = 128;

    if (const auto N = qMin(batchSize, _queue.size()); N > 0) {
        const auto batch = _queue.first(N);
        _queue.remove(0, N);
        consume(batch);
    }

    if (_queue.empty()) {
        _timer->stop();
    }
}

/// storage functionality should be enabled after the scene has been loaded.
void SceneStorage::enableStorage()
{
    _enabled = true;
}

void SceneStorage::saveNodes(const QList<const NodeItem*>& nodes) const
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery q(db);

        q.prepare(stmt::scene::INSERT_NODE);

        for (auto* node : nodes) {
            Q_ASSERT(node != nullptr);
            if (node->index().isValid()) {
                q.addBindValue(_scene->filePath(node->index()));
                q.addBindValue(static_cast<int>(node->nodeFlags()));
                q.addBindValue(node->firstRow());
                const auto pos = node->scenePos();
                q.addBindValue(pos.x());
                q.addBindValue(pos.y());
                if (const auto* pe  = node->parentEdge(); pe) {
                    q.addBindValue(pe->line().length());
                }
                if (!q.exec()) {
                    qWarning() << q.lastError();
                }
            }
        }

        if (!db.commit()) {
            qWarning() << q.lastError();
        }
    }
}

void SceneStorage::consume(const QList<QVariant>& data)
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery qDelFile(db);
        QSqlQuery qDelDir(db);
        QSqlQuery qIns(db);

        qDelFile.prepare(stmt::scene::DELETE_FILE_NODE);
        qDelDir.prepare(stmt::scene::DELETE_DIR_NODE);
        qIns.prepare(stmt::scene::INSERT_NODE);

        for (const auto& d : data) {
            if (d.canConvert<DeleteData>()) {
                const auto [id, isDir] = d.value<DeleteData>();

                if (isDir) {
                    qDelDir.addBindValue(id + "%");
                    if (!qDelDir.exec()) {
                        qWarning() << qDelDir.lastError();
                    }
                } else {
                    qDelFile.addBindValue(id);
                    if (!qDelFile.exec()) {
                        qWarning() << qDelFile.lastError();
                    }
                }
            } else if (d.canConvert<SaveData>()) {
                const auto [id, nodeType, firstRow, pos, length] = d.value<SaveData>();

                qIns.addBindValue(id);
                qIns.addBindValue(nodeType);
                qIns.addBindValue(firstRow);
                qIns.addBindValue(pos.x());
                qIns.addBindValue(pos.y());
                qIns.addBindValue(length);

                if (!qIns.exec()) {
                    qWarning() << db.lastError();
                }
            }
        }

        if (!db.commit()) {
            qWarning() << db.lastError();
        }
    }
}

void SceneStorage::createTable()
{
    if (const auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);

        if (!q.exec(stmt::scene::CREATE_NODES_TABLE)) {
            qWarning() << "failed to create nodes table" << q.lastError();
        }
    }
}

QHash<QPersistentModelIndex, QList<NodeData>> SceneStorage::readTable(const FileSystemScene* scene)
{
    const auto db = db::get();

    if (!db.isOpen()) {
        return {};
    }

    QHash<QPersistentModelIndex, QList<NodeData>> graph;

    QSqlQuery q(db);
    q.prepare(stmt::scene::SELECT_ALL_NODES);

    if (q.exec()) {
        const auto rec       = q.record();
        const auto idIndex   = rec.indexOf(stmt::scene::NODE_ID);
        const auto typeIndex = rec.indexOf(stmt::scene::NODE_TYPE);
        const auto rowIndex  = rec.indexOf(stmt::scene::FIRST_ROW);
        const auto xposIndex = rec.indexOf(stmt::scene::NODE_XPOS);
        const auto yposIndex = rec.indexOf(stmt::scene::NODE_YPOS);
        const auto lenIndex  = rec.indexOf(stmt::scene::EDGE_LEN);
        auto ok = false;

        while (q.next()) {
            const auto path  = q.value(idIndex).toString();
            const auto index = scene->index(path);
            const auto type  = static_cast<NodeType>(q.value(typeIndex).toInt(&ok)); Q_ASSERT(ok);
            const auto row   = q.value(rowIndex).toInt(&ok);   Q_ASSERT(ok);
            const auto x     = q.value(xposIndex).toReal(&ok); Q_ASSERT(ok);
            const auto y     = q.value(yposIndex).toReal(&ok); Q_ASSERT(ok);
            const auto len   = q.value(lenIndex).toReal(&ok);  Q_ASSERT(ok);

            if (index.isValid()) {
                graph[index.parent()].push_back(NodeData{ index, type, row, QPointF(x, y), len });
            }
        }
    }

    return graph;
}
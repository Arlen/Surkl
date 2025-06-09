/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SceneStorage.hpp"
#include "FileSystemScene.hpp"
#include "NodeItem.hpp"
#include "SessionManager.hpp"
#include "db.hpp"

#include <QDir>
#include <QSqlRecord>
#include <QTimer>


using namespace core;

SceneStorage::SceneStorage(QObject* parent)
    : QObject(parent)
{
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, this, &SceneStorage::nextN);
}

void SceneStorage::configure()
{
    createTable();

    Q_ASSERT(core::db::doesTableExists(NODES_TABLE));
}

void SceneStorage::deleteNode(const NodeItem *node)
{
    Q_ASSERT(node != nullptr);

    const auto id = _scene->filePath(node->index());

    _queue.push_back(QVariant::fromValue<DeleteData>({id}));
    _timer->start(125);
}

void SceneStorage::saveNode(const NodeItem *node)
{
    if (!_enabled) {
        return;
    }

    Q_ASSERT(node != nullptr);

    const auto id       = _scene->filePath(node->index());
    const auto nodeType = static_cast<int>(node->nodeType());
    const auto pos      = node->scenePos();
    const auto length   = node->parentEdge()->line().length();

    Q_ASSERT(!id.isEmpty());

    auto match = [id](const QVariant& v) -> bool {
        if (v.canConvert<SaveData>()) {
            return id == v.value<SaveData>().id;
        }
        return false;
    };

    if (auto found = std::ranges::find_if(_queue, match); found != _queue.end()) {
        *found = QVariant::fromValue<SaveData>({id, nodeType, pos, length});
    } else {
        _queue.push_back(QVariant::fromValue<SaveData>({id, nodeType, pos, length}));
    }

    _timer->start(125);
}

void SceneStorage::saveScene() const
{
    const auto toBeSaved = _scene->items()
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
    if (G.contains(QModelIndex())) {
        /// This assumes we have only a single root node ("/").

        auto& Ms = G[QModelIndex()];
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
        if (G.count(parent.index)) {
            auto& childNodeData = G[parent.index];
            if (!childNodeData.empty()) {
                sortByRows(childNodeData);

                parentNode->createChildNodes(childNodeData);
                parent.edge->adjust();
            }
            for (const auto& nd : childNodeData) {
                S.push_back(nd);
            }
        }
        parentNode->setPos(parent.pos);

        if (parent.type == NodeType::HalfClosedNode) {
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

        q.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2,%3,%4,%5,%6) VALUES(?, ?, ?, ?, ?)")
            .arg(NODES_TABLE)

            .arg(NODE_ID)
            .arg(NODE_TYPE)
            .arg(NODE_XPOS)
            .arg(NODE_YPOS)
            .arg(EDGE_LEN));

        for (auto* node : nodes) {
            if (node && node->index().isValid()) {
                q.addBindValue(_scene->filePath(node->index()));
                q.addBindValue(static_cast<int>(node->nodeType()));
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
        QSqlQuery q_del(db);
        QSqlQuery q_save(db);

        q_del.prepare(QLatin1StringView("DELETE FROM %1 WHERE %2=:id")
            .arg(NODES_TABLE)
            .arg(NODE_ID));

        q_save.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2,%3,%4,%5,%6) VALUES(?, ?, ?, ?, ?)")
            .arg(NODES_TABLE)

            .arg(NODE_ID)
            .arg(NODE_TYPE)
            .arg(NODE_XPOS)
            .arg(NODE_YPOS)
            .arg(EDGE_LEN));

        for (const auto& d : data) {
            if (d.canConvert<DeleteData>()) {
                const auto [id] = d.value<DeleteData>();

                q_del.bindValue(":id", id);

                if (!q_del.exec()) {
                    qWarning() << q_del.lastError();
                }
            } else if (d.canConvert<SaveData>()) {
                const auto [id, nodeType, pos, length] = d.value<SaveData>();

                q_save.addBindValue(id);
                q_save.addBindValue(nodeType);
                q_save.addBindValue(pos.x());
                q_save.addBindValue(pos.y());
                q_save.addBindValue(length);

                if (!q_save.exec()) {
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

        q.exec(
            QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 TEXT PRIMARY KEY
                            , %4 INTEGER
                            , %5 REAL
                            , %6 REAL
                            , %7 REAL))")
                .arg(NODES_TABLE)
                .arg(NODE_ID)
                .arg(NODE_TYPE)
                .arg(NODE_XPOS)
                .arg(NODE_YPOS)
                .arg(EDGE_LEN));
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
    q.prepare(QLatin1StringView("SELECT * FROM %1").arg(NODES_TABLE));

    if (q.exec()) {
        const auto rec       = q.record();
        const auto idIndex   = rec.indexOf(NODE_ID);
        const auto typeIndex = rec.indexOf(NODE_TYPE);
        const auto xposIndex = rec.indexOf(NODE_XPOS);
        const auto yposIndex = rec.indexOf(NODE_YPOS);
        const auto lenIndex  = rec.indexOf(EDGE_LEN);
        auto ok = false;

        while (q.next()) {
            const auto path  = q.value(idIndex).toString();
            const auto index = scene->index(path);
            const auto type  = static_cast<NodeType>(q.value(typeIndex).toInt(&ok)); Q_ASSERT(ok);
            const auto x     = q.value(xposIndex).toReal(&ok); Q_ASSERT(ok);
            const auto y     = q.value(yposIndex).toReal(&ok); Q_ASSERT(ok);
            const auto len   = q.value(lenIndex).toReal(&ok);  Q_ASSERT(ok);

            if (index.isValid()) {
                graph[index.parent()].push_back(NodeData{ index, type, QPointF(x, y), len });
            }
        }
    }

    return graph;
}
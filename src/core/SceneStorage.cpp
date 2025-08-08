/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SceneStorage.hpp"
#include "FileSystemScene.hpp"
#include "NodeItem.hpp"
#include "SessionManager.hpp"
#include "db/db.hpp"
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
    Q_ASSERT(core::db::doesTableExists(stmt::scene::NODES_DIR_ATTR_TABLE));
}

void SceneStorage::deleteNode(const NodeItem *node)
{
    Q_ASSERT(node != nullptr);

    const auto data = getStorageData(node, StorageData::DeleteOp);

    _queue.push_back(QVariant::fromValue<StorageData>(data));
    _timer->start(125);
}

void SceneStorage::saveNode(const NodeItem *node)
{
    if (!_enabled || !node->index().isValid()) {
        return;
    }

    Q_ASSERT(node != nullptr);

    const auto data = getStorageData(node, StorageData::SaveOp);

    Q_ASSERT(!data.id.isEmpty());

    auto match = [id=data.id](const QVariant& var) -> bool
    {
        const auto sd = var.value<StorageData>();

        return sd.op == StorageData::SaveOp && sd.id == id;
    };

    if (auto found = std::ranges::find_if(_queue, match); found != _queue.end()) {
        *found = QVariant::fromValue<StorageData>(data);
    } else {
        _queue.push_back(QVariant::fromValue<StorageData>(data));
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
        enableStorage();
        auto* edge = NodeItem::createRootNode(scene->rootIndex());
        scene->addItem(edge->source());
        scene->addItem(edge->target());
        scene->addItem(edge);
        edge->adjust();

        scene->openTo(QDir::homePath());
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
            if (!NodeFlags(m.type).testAnyFlag(NodeType::ClosedNode)) {
                S.push_back(m);
            }
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
            Q_ASSERT(node->parentEdge());
            if (node->index().isValid()) {
                q.addBindValue(_scene->filePath(node->index()));
                q.addBindValue(static_cast<int>(node->nodeFlags()));
                const auto pos = node->scenePos();
                q.addBindValue(pos.x());
                q.addBindValue(pos.y());
                q.addBindValue(node->length());

                if (!q.exec()) {
                    qWarning() << q.lastError();
                }
            }
        }

        q.prepare(stmt::scene::INSERT_NODE_DIR_ATTR);

        for (auto* node : nodes) {
            if (node->index().isValid()) {
                q.addBindValue(_scene->filePath(node->index()));
                q.addBindValue(node->firstRow());
                /// TODO: q.addBindValue(node->rotation());
                q.addBindValue(0.0); // set angle of rotation to zero for now.

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

StorageData SceneStorage::getStorageData(const NodeItem* node, StorageData::OperationType op) const
{
    return
        {
            .op       = op,
            .id       = _scene->filePath(node->index()),
            .nodeType = static_cast<int>(node->nodeFlags()),
            .firstRow = node->firstRow(),
            .pos      = node->scenePos(),
            .length   = node->length(),
            .rotation = 0.0, // TODO: node->rotation()
            .isDir    = node->isDir()
        };
}

void SceneStorage::consume(const QList<QVariant>& data)
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery qDelFile(db);
        QSqlQuery qDelDir(db);
        QSqlQuery qDelNodeDirAttr(db);
        QSqlQuery qInsNode(db);
        QSqlQuery qInsNodeDirAttr(db);

        qDelFile.prepare(stmt::scene::DELETE_FILE_NODE);
        qDelDir.prepare(stmt::scene::DELETE_DIR_NODE);
        qDelNodeDirAttr.prepare(stmt::scene::DELETE_NODE_DIR_ATTR);
        qInsNode.prepare(stmt::scene::INSERT_NODE);
        qInsNodeDirAttr.prepare(stmt::scene::INSERT_NODE_DIR_ATTR);

        for (const auto& d : data) {
            const auto [op, id, nodeType, firstRow, pos, length, rotation, isDir] = d.value<StorageData>();

            if (op == StorageData::DeleteOp) {
                if (isDir) {
                    qDelDir.addBindValue(id);
                    if (!qDelDir.exec()) {
                        qWarning() << db.lastError();
                    }
                    qDelNodeDirAttr.addBindValue(id);
                    if (!qDelNodeDirAttr.exec()) {
                        qWarning() << db.lastError();
                    }
                } else {
                    qDelFile.addBindValue(id);
                    if (!qDelFile.exec()) {
                        qWarning() << db.lastError();
                    }
                }
            } else if (op == StorageData::SaveOp) {
                qInsNode.addBindValue(id);
                qInsNode.addBindValue(nodeType);
                qInsNode.addBindValue(pos.x());
                qInsNode.addBindValue(pos.y());
                qInsNode.addBindValue(length);
                if (!qInsNode.exec()) {
                    qWarning() << db.lastError();
                }

                if (isDir) {
                    qInsNodeDirAttr.addBindValue(id);
                    qInsNodeDirAttr.addBindValue(firstRow);
                    qInsNodeDirAttr.addBindValue(rotation);
                    if (!qInsNodeDirAttr.exec()) {
                        qWarning() << db.lastError();
                    }
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

        if (!q.exec(stmt::scene::CREATE_NODES_DIR_ATTR_TABLE)) {
            qWarning() << "failed to create node dir-attributes table" << q.lastError();
        }
    }
}

QHash<QPersistentModelIndex, QList<NodeData>> SceneStorage::readTable(const FileSystemScene* scene)
{
    const auto db = db::get();

    if (!db.isOpen()) {
        return {};
    }

    struct Attribute
    {
        int firstRow{0};
        qreal rotation{0};
    };

    QSqlQuery q(db);

    q.prepare(stmt::scene::SELECT_ALL_NODES_DIR_ATTRS);
    QHash<QPersistentModelIndex, Attribute> attributes;

    if (q.exec()) {
        const auto rec    = q.record();
        const auto idIdx  = rec.indexOf(stmt::scene::NODE_ID);
        const auto rowIdx = rec.indexOf(stmt::scene::FIRST_ROW);
        const auto rotIdx = rec.indexOf(stmt::scene::NODE_ROT);
        auto ok = false;

        while (q.next()) {
            const auto path  = q.value(idIdx).toString();
            const auto index = scene->index(path);
            const auto row   = q.value(rowIdx).toInt(&ok);   Q_ASSERT(ok);
            const auto rot   = q.value(rotIdx).toReal(&ok);  Q_ASSERT(ok);

            if (index.isValid()) {
                /// QModelIndex and QPersistentModelIndex behave differently
                /// when it comes to uniqueness.
                /// QModelIndex: two link directories pointing to the same directory are considered the same.
                /// QPersistentModelIndex: "" not the same.
                Q_ASSERT(!attributes.contains(index));

                attributes[index] = {row, rot};
            }
        }
    }

    q.prepare(stmt::scene::SELECT_ALL_NODES);
    QHash<QPersistentModelIndex, QList<NodeData>> graph;

    if (q.exec()) {
        const auto rec     = q.record();
        const auto idIdx   = rec.indexOf(stmt::scene::NODE_ID);
        const auto typeIdx = rec.indexOf(stmt::scene::NODE_TYPE);
        const auto posxIdx = rec.indexOf(stmt::scene::NODE_POS_X);
        const auto posyIdx = rec.indexOf(stmt::scene::NODE_POS_Y);
        const auto lenIdx  = rec.indexOf(stmt::scene::EDGE_LEN);
        auto ok = false;

        while (q.next()) {
            const auto path  = q.value(idIdx).toString();
            const auto index = scene->index(path);
            const auto type  = static_cast<NodeType>(q.value(typeIdx).toInt(&ok)); Q_ASSERT(ok);
            const auto x     = q.value(posxIdx).toReal(&ok); Q_ASSERT(ok);
            const auto y     = q.value(posyIdx).toReal(&ok); Q_ASSERT(ok);
            const auto len   = q.value(lenIdx).toReal(&ok);  Q_ASSERT(ok);

            if (index.isValid()) {
                auto nd = NodeData
                    {
                        .index    = index,
                        .type     = type,
                        .firstRow = 0,
                        .pos      = QPointF(x, y),
                        .length   = len,
                        .rotation = 0
                    };

                if (attributes.contains(index)) {
                    const auto [row, rot] = attributes[index];
                    nd.firstRow = row;
                    nd.rotation = rot;
                }

                if (index.isValid()) {
                    graph[index.parent()].push_back(nd);
                } else {
                    qWarning() << "read invalid index!";
                }
            }
        }
    }

    return graph;
}
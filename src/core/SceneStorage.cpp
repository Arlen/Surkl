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

    connect(_timer, &QTimer::timeout, this, &SceneStorage::push);
}

void SceneStorage::configure()
{
    createTable();

    Q_ASSERT(core::db::doesTableExists(NODES_TABLE));
}

void SceneStorage::saveNode(const NodeItem *node)
{
    Q_ASSERT(node != nullptr);

    _toBeSaved.removeOne(node);
    _toBeSaved.push_back(node);

    _timer->start(125);
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

void SceneStorage::saveScene(const QGraphicsScene* scene)
        if (!db.commit()) {
            qWarning() << q.lastError();
        }
    }
}

void SceneStorage::saveScene() const
{
    const auto toBeSaved = _scene->items()
        | std::views::transform(&asNodeItem)
        | std::ranges::to<QList<const NodeItem*>>()
        ;

    saveNodes(toBeSaved);
}

{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
void SceneStorage::push()
{
    if (const auto nextN = qMin(32, _toBeSaved.size()); nextN > 0) {
        const auto nodes = _toBeSaved.first(nextN);
        _toBeSaved.remove(0, nextN);
        saveNodes(nodes);
    }

    if (_toBeSaved.empty()) {
        _timer->stop();
    }
}

void SceneStorage::enableSave()
{
    connect(_scene, &QGraphicsScene::changed,
        [this](const QList<QRectF>& region) {
            for (const auto& reg : region) {
                for (const auto items = _scene->items(reg); auto* item : items) {
                    if (auto* node = qgraphicsitem_cast<const NodeItem*>(item); node) {
                        saveNode(node);
                    }
                }
            }
        });
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
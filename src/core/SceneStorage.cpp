/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SceneStorage.hpp"
#include "FileSystemScene.hpp"
#include "NodeItem.hpp"
#include "SessionManager.hpp"
#include "db.hpp"


using namespace core;

SceneStorage::SceneStorage(QObject* parent)
    : QObject(parent)
{

}

void SceneStorage::configure(SceneStorage* ps)
{
    Q_UNUSED(ps);

    using namespace std::ranges;

    createTables();
    Q_ASSERT(core::db::doesTableExists(NODES_TABLE));
}

void SceneStorage::createTables()
{
    const auto db = db::get();

    if (!db.isOpen()) { return; }

    QSqlQuery q(db);

    q.exec(
        QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 TEXT NOT NULL PRIMARY KEY
                            , %3 TEXT
                            , %4 INTEGER
                            , %5 REAL
                            , %6 REAL
                            , %7 REAL))")
            .arg(NODES_TABLE)
            .arg(NODE_ID)
            .arg(PARENT_ID)
            .arg(NODE_STATE)
            .arg(NODE_XPOS)
            .arg(NODE_YPOS)
            .arg(EDGE_LEN));
}

void SceneStorage::saveNode(const NodeItem *node)
{
    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);

        q.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2,%3,%4,%5,%6,%7) VALUES(?, ?, ?, ?, ?, ?)")
            .arg(NODES_TABLE)

            .arg(NODE_ID)
            .arg(PARENT_ID)
            .arg(NODE_STATE)
            .arg(NODE_XPOS)
            .arg(NODE_YPOS)
            .arg(EDGE_LEN));

        q.addBindValue(SessionManager::scene()->filePath(node->index()));
        q.addBindValue(SessionManager::scene()->filePath(node->index().parent()));
        q.addBindValue(static_cast<int>(node->nodeType()));
        const auto pos = node->scenePos();
        q.addBindValue(pos.x());
        q.addBindValue(pos.y());
        if (auto* pe  = node->parentEdge(); pe) {
            q.addBindValue(pe->line().length());
        }

        if (!q.exec()) {
            qWarning() << q.lastError();
        }
    }
}

void SceneStorage::saveScene(const QGraphicsScene* scene)
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery q(db);

        q.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2,%3,%4,%5,%6,%7) VALUES(?, ?, ?, ?, ?, ?)")
            .arg(NODES_TABLE)

            .arg(NODE_ID)
            .arg(PARENT_ID)
            .arg(NODE_STATE)
            .arg(NODE_XPOS)
            .arg(NODE_YPOS)
            .arg(EDGE_LEN));

        const auto items = scene->items();
        for (auto* item : items) {
            if (const auto* node = qgraphicsitem_cast<NodeItem*>(item); node) {
                q.addBindValue(SessionManager::scene()->filePath(node->index()));
                q.addBindValue(SessionManager::scene()->filePath(node->index().parent()));
                q.addBindValue(static_cast<int>(node->nodeType()));
                const auto pos = node->scenePos();
                q.addBindValue(pos.x());
                q.addBindValue(pos.y());
                if (auto* pe  = node->parentEdge(); pe) {
                    q.addBindValue(pe->line().length());
                }
            }
        }

        if (!db.commit()) {
            qWarning() << q.lastError();
        }
    }
}

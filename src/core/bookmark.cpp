/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "bookmark.hpp"
#include "db/db.hpp"
#include "db/stmt.hpp"

#include <QSqlRecord>


using namespace core;

void BookmarkManager::configure(BookmarkManager* bm)
{
    if (auto db = db::get(); db.isOpen()) {
        if (db::doesTableExists(stmt::bm::TABLE_NAME)) {
            QSqlQuery q(db);
            q.prepare(stmt::bm::SELECT_ALL_PALETTES);

            if (!q.exec()) { qWarning() << q.lastError(); }

            const auto rec     = q.record();
            const auto posXIdx = rec.indexOf(stmt::bm::POSITION_X_COL);
            const auto posYIdx = rec.indexOf(stmt::bm::POSITION_Y_COL);
            const auto nameIdx = rec.indexOf(stmt::bm::NAME_COL);

            while (q.next())
            {
                const auto pos  = QPoint(q.value(posXIdx).toInt(), q.value(posYIdx).toInt());
                const auto name = q.value(nameIdx).toString();
                const auto sbm  = SceneBookmarkData{pos, name};

                bm->_scene_bms.insert(sbm);
            }
        } else {
            db.transaction();
            QSqlQuery q(db);
            q.prepare(stmt::bm::CREATE_SCENE_BOOKMARKS_TABLE);

            if (!q.exec()) { qWarning() << q.lastError(); }

            for (const auto& x : bm->sceneBookmarksAsList()) {
                addToDatabase(x);
            }

            if (!db.commit()) {
                qWarning()
                    << "BookmarkManager::configure: Failed to commit changes ("
                    << db.lastError()
                    << ")";
            }
        }
    }
}

void BookmarkManager::saveToDatabase(BookmarkManager* bm)
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        for (const auto& x : bm->sceneBookmarksAsList()) {
            addToDatabase(x);
        }

        if (!db.commit()) {
            qWarning()
                << "BookmarkManager::configure: Failed to commit changes ("
                << db.lastError()
                << ")";
        }
    }
}

BookmarkManager::BookmarkManager(QObject* parent)
    : QObject(parent)
{

}

void BookmarkManager::insertBookmark(const SceneBookmarkData& bm)
{
    Q_ASSERT(!_scene_bms.contains(bm));

    if (!_scene_bms.contains(bm)) {
        addToDatabase(bm);
        _scene_bms.insert(bm);
    }
}

void BookmarkManager::updateBookmark(const SceneBookmarkData& bm)
{
    Q_ASSERT(_scene_bms.contains(bm));

    if (_scene_bms.contains(bm)) {
        addToDatabase(bm);
        _scene_bms.remove(bm);
        _scene_bms.insert(bm);
    }
}

void BookmarkManager::removeBookmarks(const QList<SceneBookmarkData>& bookmarks)
{
    QList<SceneBookmarkData> data;

    for (const auto& bm : bookmarks) {
        Q_ASSERT(_scene_bms.contains(bm));

        if (_scene_bms.remove(bm)) {
            data.push_back(bm);
        }
    }

    removeFromDatabase(data);
}

QList<SceneBookmarkData> BookmarkManager::sceneBookmarksAsList() const
{
    return _scene_bms.values();
}

QSet<SceneBookmarkData> BookmarkManager::sceneBookmarks() const
{
    return _scene_bms;
}

void BookmarkManager::addToDatabase(const SceneBookmarkData& sbm)
{
    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(stmt::bm::INSERT_BM);
        q.addBindValue(sbm.pos.x());
        q.addBindValue(sbm.pos.y());
        q.addBindValue(sbm.name);

        if (!q.exec()) {
            qWarning() << q.lastError();
        }
    }
}

void BookmarkManager::removeFromDatabase(const QList<SceneBookmarkData>& bookmarks)
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();

        QSqlQuery q(db);
        q.prepare(stmt::bm::DELETE_BM);

        for (const auto& sbm : bookmarks) {
            q.addBindValue(sbm.pos.x());
            q.addBindValue(sbm.pos.y());

            if (!q.exec()) {
                qWarning() << q.lastError();
            }
        }

        if (!db.commit()) {
            qWarning()
                << "BookmarkManager::removeFromDatabase: Failed to commit changes ("
                << db.lastError()
                << ")";
        }
    }
}
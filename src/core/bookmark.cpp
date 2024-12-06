#include "bookmark.hpp"
#include "db.hpp"


using namespace core;
using namespace Qt::Literals::StringLiterals;

void BookmarkManager::configure(BookmarkManager* bm)
{
    if (auto db = db::get(); db.isOpen()) {
        if (db::doesTableExists(TABLE_NAME)) {
            QSqlQuery q(db);
            q.prepare(QLatin1StringView("SELECT * FROM %1").arg(TABLE_NAME));
            q.exec();
            const auto rec         = q.record();
            const auto hash_index  = rec.indexOf(BOOKMARK_HASH);
            const auto pos_x_index = rec.indexOf(POSITION_X_COL);
            const auto pos_y_index = rec.indexOf(POSITION_Y_COL);
            const auto name_index  = rec.indexOf(NAME_COL);

            while (q.next())
            {
                bool ok = false;
                const auto hash = q.value(hash_index).toULongLong(&ok);
                const auto pos = QPoint(q.value(pos_x_index).toInt(), q.value(pos_y_index).toInt());
                const auto name = q.value(name_index).toString();
                const auto sbm = SceneBookmarkData{pos, name};
                const auto computed_hash = qHash(pos);

                assert(ok);

                if (computed_hash == hash) {
                    bm->_scene_bms.insert(sbm);
                } else {
                    qWarning() << "BookmarkManager::configure bookmarks don't match";
                }
            }
        } else {
            db.transaction();
            QSqlQuery q(db);
            q.prepare(
                QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                                    ( %2 INTEGER NOT NULL PRIMARY KEY
                                    , %3 INTEGER NOT NULL
                                    , %4 INTEGER NOT NULL
                                    , %5 TEXT NOT NULL))")
                .arg(TABLE_NAME)
                .arg(BOOKMARK_HASH)
                .arg(POSITION_X_COL)
                .arg(POSITION_Y_COL)
                .arg(NAME_COL));
            q.exec();
            for (const auto& x : bm->sceneBookmarks()) {
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
        for (const auto& x : bm->sceneBookmarks()) {
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
    assert(!_scene_bms.contains(bm));

    if (!_scene_bms.contains(bm)) {
        addToDatabase(bm);
        _scene_bms.insert(bm);
    }
}

void BookmarkManager::updateBookmark(const SceneBookmarkData& bm)
{
    assert(_scene_bms.contains(bm));

    if (_scene_bms.contains(bm)) {
        addToDatabase(bm);
        _scene_bms.remove(bm);
        _scene_bms.insert(bm);
    }
}

void BookmarkManager::removeBookmark(const SceneBookmarkData& bm)
{
    assert(_scene_bms.contains(bm));

    if (!_scene_bms.contains(bm)) {
        removeFromDatabase(bm);
        _scene_bms.remove(bm);
    }
}

QList<SceneBookmarkData> BookmarkManager::sceneBookmarks() const
{
    return _scene_bms.values();
}

void BookmarkManager::addToDatabase(const SceneBookmarkData& sbm)
{
    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(R"(INSERT OR REPLACE INTO %1 (%2, %3, %4, %5)
                        VALUES(?, ?, ?, ?))"_L1
        .arg(TABLE_NAME)
        .arg(BOOKMARK_HASH)
        .arg(POSITION_X_COL)
        .arg(POSITION_Y_COL)
        .arg(NAME_COL));
        q.addBindValue(QVariant::fromValue(qHash(sbm.pos)));
        q.addBindValue(sbm.pos.x());
        q.addBindValue(sbm.pos.y());
        q.addBindValue(sbm.name);
        q.exec();
    }
}

void BookmarkManager::removeFromDatabase(const SceneBookmarkData& sbm)
{
    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(R"(DELETE FROM %1 WHERE hash=%2)"_L1
        .arg(TABLE_NAME)
        .arg(qHash(sbm.pos)));
        q.exec();
    }
}
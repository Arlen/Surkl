#include "bookmark.hpp"
#include "db.hpp"

#include <QSqlRecord>


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
            const auto pos_x_index = rec.indexOf(POSITION_X_COL);
            const auto pos_y_index = rec.indexOf(POSITION_Y_COL);
            const auto name_index  = rec.indexOf(NAME_COL);

            while (q.next())
            {
                const auto pos  = QPoint(q.value(pos_x_index).toInt(), q.value(pos_y_index).toInt());
                const auto name = q.value(name_index).toString();
                const auto sbm  = SceneBookmarkData{pos, name};

                bm->_scene_bms.insert(sbm);
            }
        } else {
            db.transaction();
            QSqlQuery q(db);
            q.prepare(
                QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                                    ( %2 INTEGER NOT NULL
                                    , %3 INTEGER NOT NULL
                                    , %4 TEXT NOT NULL
                                    , UNIQUE(%2, %3)))")
                .arg(TABLE_NAME)
                .arg(POSITION_X_COL)
                .arg(POSITION_Y_COL)
                .arg(NAME_COL));
            q.exec();
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
        q.prepare(R"(INSERT OR REPLACE INTO %1 (%2, %3, %4) VALUES(?, ?, ?))"_L1
            .arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL)
            .arg(NAME_COL));

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
        q.prepare(R"(DELETE FROM %1 WHERE %2=? AND %3=?)"_L1
            .arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL));

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
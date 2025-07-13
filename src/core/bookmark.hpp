/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPoint>
#include <QSet>


namespace core
{
    struct SceneBookmarkData
    {
        QPoint pos;
        QString name;
    };

    inline bool operator==(const SceneBookmarkData& lhs, const SceneBookmarkData& rhs)
    {
        return lhs.pos == rhs.pos;
    }

    inline size_t qHash(const SceneBookmarkData& sbm, size_t seed)
    {
        return qHash(sbm.pos, seed);
    }

    /// TODO: filesystem booksmark


    class BookmarkManager : public QObject
    {
        Q_OBJECT

    public:
        static void configure(BookmarkManager* tm);
        static void saveToDatabase(BookmarkManager* tm);

        explicit BookmarkManager(QObject* parent = nullptr);

        void insertBookmark(const SceneBookmarkData& bm);
        void updateBookmark(const SceneBookmarkData& bm);
        void removeBookmarks(const QList<SceneBookmarkData>& bookmarks);
        QList<SceneBookmarkData> sceneBookmarksAsList() const;
        QSet<SceneBookmarkData> sceneBookmarks() const;

    private:
        static void addToDatabase(const SceneBookmarkData& sbm);
        static void removeFromDatabase(const QList<SceneBookmarkData>& bookmarks);

        QSet<SceneBookmarkData> _scene_bms;
    };
}

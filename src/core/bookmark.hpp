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
        static constexpr auto TABLE_NAME     = QLatin1StringView("SceneBookmarks");
        static constexpr auto BOOKMARK_HASH  = QLatin1StringView("hash");
        static constexpr auto POSITION_X_COL = QLatin1StringView("position_x");
        static constexpr auto POSITION_Y_COL = QLatin1StringView("position_y");
        static constexpr auto NAME_COL       = QLatin1StringView("name");

        static void configure(BookmarkManager* tm);
        static void saveToDatabase(BookmarkManager* tm);

        explicit BookmarkManager(QObject* parent = nullptr);

        void insertBookmark(const SceneBookmarkData& bm);
        void updateBookmark(const SceneBookmarkData& bm);
        void removeBookmark(const SceneBookmarkData& bm);
        QList<SceneBookmarkData> sceneBookmarks() const;

    private:
        static void addToDatabase(const SceneBookmarkData& sbm);
        static void removeFromDatabase(const SceneBookmarkData& sbm);

        QSet<SceneBookmarkData> _scene_bms;
    };
}

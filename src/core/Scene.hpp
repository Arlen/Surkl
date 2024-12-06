#pragma once

#include <QGraphicsScene>


namespace core
{
    namespace nodes
    {
        class SceneBookmarkItem;
    }

    class Scene final : public QGraphicsScene
    {
        Q_OBJECT

    public:
        explicit Scene(QObject* parent = nullptr);
        void addSceneBookmark(const QPoint& pos, const QString& name);

    protected:
        void drawBackground(QPainter* p, const QRectF& rec) override;

    private:
        void drawCrosses(QPainter* p, const QRectF& rec) const;
    };
}

#pragma once

#include <QGraphicsScene>


namespace core
{
    class Scene final : public QGraphicsScene
    {
    public:
        explicit Scene(QObject* parent = nullptr);

    protected:
        void drawBackground(QPainter* p, const QRectF& rec) override;

    private:
        void drawCrosses(QPainter* p, const QRectF& rec) const;
    };
}

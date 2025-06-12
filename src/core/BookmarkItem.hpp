/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QGraphicsRectItem>


namespace core
{
    namespace internal
    {
        void drawItemShape(QPainter* p, QGraphicsItem* item);
        void drawBoundingRect(QPainter* p, QGraphicsItem* item);
    }

    class SceneBookmarkItem final : public QGraphicsRectItem
    {
        static constexpr auto RECT_SIZE = 32;

    public:
        enum { Type = UserType + 6 };

        SceneBookmarkItem(const QPoint& pos, const QString& name);

        void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

        [[nodiscard]] int type() const override { return Type; }

    private:
        QGraphicsSimpleTextItem* _name;
    };
}

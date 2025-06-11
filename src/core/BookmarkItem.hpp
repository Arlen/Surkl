/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "geometry.hpp"

#include <QGraphicsRectItem>
#include <QRadialGradient>


namespace core
{
    namespace internal
    {
        void drawItemShape(QPainter* p, QGraphicsItem* item);
        void drawBoundingRect(QPainter* p, QGraphicsItem* item);
    }

    class SceneBookmarkItem final : public QGraphicsRectItem
    {
        static constexpr auto RECT_SIZE   = 64*4;
        inline static const auto POLYGONS = geometry::SceneBookmarkIcon().generate(RECT_SIZE);

    public:
        SceneBookmarkItem(const QPoint& pos, const QString& name, bool born = false);

        void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    protected:
        void setFrame(int frame);

    private:
        int _frame = -1;
        std::array<QColor, 4> _bigLeafColors;
        std::array<QColor, 4> _smallLeafColors;

        QRadialGradient _bigGradient1;
        QRadialGradient _bigGradient2;
        QRadialGradient _bigGradient3;

        QString _name;
    };
}

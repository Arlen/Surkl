/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <QLineF>


class QGraphicsItem;

namespace core
{
    class NodeItem;

    struct Side
    {
        QLineF edge;
        QLineF norm;
    };

    using Ngon = std::vector<Side>;
    using NgonVector = std::vector<Ngon>;

    QLineF lineOf(const QGraphicsItem* a, const QGraphicsItem* b);

    Ngon getNgon(int n);

    Ngon makeNgon(int n, qreal startAngle = 0);

    NgonVector makeNgons(int N);

    QLineF getNgonSideNorm(int i, int n);

    Ngon getGuides(const NodeItem* node, const QGraphicsItem* ignore = nullptr);

    Ngon getGuides(const NodeItem* node, int sides, const std::vector<const QGraphicsItem*>& fixed);
}
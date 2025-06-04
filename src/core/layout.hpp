/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <deque>
#include <vector>

#include <QLineF>


class QGraphicsItem;

namespace core
{
    class NodeItem;

    using Ngon = std::deque<QLineF>;

    constexpr Ngon makeNgon(int sides, qreal startAngle = 0.0);

    constexpr QLineF lineOf(const QGraphicsItem* a, const QGraphicsItem* b);

    constexpr std::vector<QLineF> linesOf(const QGraphicsItem* a, const std::vector<const QGraphicsItem*>& items);

    Ngon guideLines(const NodeItem* node);

    Ngon guideLines(const NodeItem* node, int sides, bool ignoreGrabber = false);

    Ngon guideLines(const NodeItem* node, int sides, const std::vector<const QGraphicsItem*>& excluded);
}
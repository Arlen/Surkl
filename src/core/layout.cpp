/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "layout.hpp"
#include "NodeItem.hpp"

#include <QGraphicsScene>
#include <QtMath>

#include <ranges>


using namespace core;

constexpr Ngon core::makeNgon(int n, qreal startAngle)
{
    n = std::max(1, n);

    const auto angle  = 360.0 / n;
    const auto offset = startAngle - angle * 0.5;

    Ngon result;
    for (auto i : std::views::iota(0, n)) {
        const auto a  = angle * i     + offset;
        const auto b  = angle * (i+1) + offset;
        const auto x1 = std::cos(qDegreesToRadians(-a));
        const auto y1 = std::sin(qDegreesToRadians(-a));
        const auto x2 = std::cos(qDegreesToRadians(-b));
        const auto y2 = std::sin(qDegreesToRadians(-b));

        result.emplace_back(QPointF(x2, y2), QPointF(x1, y1));
    }

    return result;
}

constexpr QLineF core::lineOf(const QGraphicsItem* a, const QGraphicsItem* b)
{
    return QLineF(QPointF(0, 0), a->mapFromItem(b, QPointF(0, 0)));
}

constexpr std::vector<QLineF> core::linesOf(const QGraphicsItem* a, const std::vector<const QGraphicsItem*>& items)
{
    std::vector<QLineF> result;

    for (const auto* i : items) {
        result.push_back(lineOf(a, i));
    }

    return result;
}

Ngon core::guideLines(const NodeItem* node)
{
    using namespace std;

    auto intersectsWith = [](const QLineF& line)
    {
        return [line](const QLineF& other) -> bool
        {
            return other.intersects(line) == QLineF::BoundedIntersection;
        };
    };

    auto removeIfIntersects = [intersectsWith](Ngon& lines, const QLineF& line) {
        /// using find_if b/c only one guide line per intersecting line should
        /// be removed. e.g., when 'lines' contains only two lines, parentEdge
        /// will most likely intersect with two.
        const auto found = ranges::find_if(lines, intersectsWith(line));
        if (found != lines.end()) {
            lines.erase(found);
        }
    };

    const auto sides = node->childEdges().size()
        + (node->parentEdge() ? 1 : 0)
        + (node->knot() ? 1 : 0);

    auto excludedItems = node->childEdges()
        | asNotClosedTargetNodes
        | ranges::to<std::vector<const QGraphicsItem*>>()
        ;

    if (const auto* mg = node->scene()->mouseGrabberItem(); mg)
        { excludedItems.push_back(mg); }
    excludedItems.push_back(node->parentEdge()->source());
    excludedItems.push_back(node->knot());

    auto guideLines = makeNgon(sides, lineOf(node, node->knot()).angle());

    for (const auto& line : linesOf(node, excludedItems)) {
        removeIfIntersects(guideLines, line);
    }

    return guideLines;
}

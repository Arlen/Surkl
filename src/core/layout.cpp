/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "layout.hpp"
#include "NodeItem.hpp"

#include <QtMath>

#include <ranges>


using namespace core;

QLineF core::lineOf(const QGraphicsItem* a, const QGraphicsItem* b)
{
    return QLineF(QPointF(0, 0), a->mapFromItem(b, QPointF(0, 0)));
}

/// make a Ngon with 'n' sides, and with the first side perpendicular to a line
/// with an angle 'startAngle' that goes through the center.
/// zero angle is at 3 o'clock.
Ngon core::makeNgon(int n, qreal startAngle)
{
    n = std::max(1, n);

    const auto angle  = 360.0 / n;
    const auto offset = startAngle - angle * 0.5;

    Ngon result;
    for (const auto i : std::views::iota(0, n)) {
        const auto a    = angle * i     + offset;
        const auto b    = angle * (i+1) + offset;
        const auto x1   = std::cos(qDegreesToRadians(-a));
        const auto y1   = std::sin(qDegreesToRadians(-a));
        const auto x2   = std::cos(qDegreesToRadians(-b));
        const auto y2   = std::sin(qDegreesToRadians(-b));
        const auto side = QLineF(QPointF(x2, y2), QPointF(x1, y1));

        result.emplace_back(side, side.normalVector());
    }

    return result;
}

NgonVector core::makeNgons(int N)
{
    auto result = NgonVector{};
    result.push_back({}); // 0
    result.push_back({}); // 1

    for (int i = 2; i <= N; ++i) {
        result.push_back(makeNgon(i, 0));
    }

    return result;
}

Ngon core::getNgon(int n)
{
    static const auto ngons = makeNgons(NodeItem::NODE_CHILD_COUNT+2);

    Q_ASSERT(n < std::ssize(ngons));

    return ngons[n];
}

QLineF core::getNgonSideNorm(int i, int n)
{
    Q_ASSERT(i < std::ssize(getNgon(n)));
    Q_ASSERT(i < n);

    return getNgon(n)[i].norm;
}

Ngon core::getGuides(const NodeItem* node, const QGraphicsItem* ignore)
{
    using namespace std;

    const auto sides = node->childEdges().size()
        + 1  // for node->parentEdge()
        + 1; // for node->knot()

    auto fixedItems = node->childEdges()
        | asNotClosedTargetNodes
        | ranges::to<std::vector<const QGraphicsItem*>>()
        ;

    if (ignore && ignore != node) {
        if (ranges::find(fixedItems, ignore) == fixedItems.end()) {
            fixedItems.push_back(ignore);
        }
    }
    fixedItems.push_back(node->parentEdge()->source());
    fixedItems.push_back(node->knot());

    return getGuides(node, sides, fixedItems);
}

Ngon core::getGuides(const NodeItem* node, int sides, const std::vector<const QGraphicsItem*>& fixed)
{
    auto ngon = getNgon(sides);

    for (const auto* item : fixed) {
        const auto line = lineOf(node, item);
        Q_ASSERT(!line.isNull());
        for (auto& side : ngon) {
            if (!side.norm.isNull() && side.edge.intersects(line) == QLineF::BoundedIntersection) {
                side.norm = QLineF();
                break;
            }
        }
    }

    return ngon;
}
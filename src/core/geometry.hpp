#pragma once

#include <QPolygonF>
#include <QTransform>


namespace core::geometry
{
    /// Returns a square that fits inside the unit square, centered at (0,0).
    /// The square is rotated 45 deg so that a corner points up.
    constexpr QPolygonF square()
    {
        constexpr auto zero = qreal(0);
        constexpr auto half = qreal(0.5);

        return QPolygonF()
            << QPointF( zero,  half)
            << QPointF( half,  zero)
            << QPointF( zero, -half)
            << QPointF(-half,  zero)
            << QPointF( zero,  half);
    }

    class SceneBookmarkIcon final
    {
        QPolygonF scaled(const QPolygonF& pg, qreal size)
        {
            /// flip the y-coordinate b/c of Qt's coordinate system.
            return pg * QTransform().scale(size, -size);
        }

    public:
        enum TimeLine : int
        {
            Frame0 = 0,
            Frame1,
            Frame2,
            Frame3,
            Frame4,
            Frame5,
            Frame6,
            Frame7,
            Frame8,
            Frame9,
            Frame10,
            Frame11,
            Frame12,
            Frame13,
            Frame14,
            Frame15,
            Frame16,
            Frame17,
            Frame18,
            Frame19,
            Frame20,
            Frame21,
            Final
        };

        SceneBookmarkIcon()
        {
            _square1 = square();
            _square2 = _square1 * QTransform().rotate(90 * 0.25);
            _square3 = _square1 * QTransform().rotate(90 * 0.50);
            _square4 = _square1 * QTransform().rotate(90 * 0.75);
        }

        QPolygonF square1(qreal size = 1) { return scaled(_square1, size); }
        QPolygonF square2(qreal size = 1) { return scaled(_square2, size); }
        QPolygonF square3(qreal size = 1) { return scaled(_square3, size); }
        QPolygonF square4(qreal size = 1) { return scaled(_square4, size); }

        QList<QPolygonF> generate(qreal size)
        {
            QList<QPolygonF> intersections;

            intersections.append(square1().intersected(square2()));
            intersections.append(square1().intersected(square3()));
            intersections.append(square1().intersected(square4()));

            intersections.append(square2().intersected(square3()));
            intersections.append(square2().intersected(square4()));

            intersections.append(square3().intersected(square4()));

            intersections.append(square1());
            intersections.append(square2());
            intersections.append(square3());
            intersections.append(square4());

            auto get = [&](int i, int j) -> auto
            {
                assert(i < intersections.size());
                const auto poly = intersections[i];
                assert(j < poly.size());
                return poly[j];
            };

            constexpr auto zero = QPointF(0, 0);

            north_left << zero << get(7, 0) << get(0, 7) << get(6, 0) << zero;
            east_top    = north_left * QTransform().rotate(90);
            south_right = north_left * QTransform().rotate(90*2);
            west_bottom = north_left * QTransform().rotate(90*3);

            north_right << zero << get(6, 0) << get(2, 0) << get(9, 1) << zero;
            east_bottom = north_right * QTransform().rotate(90);
            south_left  = north_right * QTransform().rotate(90*2);
            west_top    = north_right * QTransform().rotate(90*3);

            ne0 << zero << get(1, 0) << get(0, 0) << get(4, 1) << zero;
            se0 = ne0 * QTransform().rotate(90);
            sw0 = ne0 * QTransform().rotate(90*2);
            nw0 = ne0 * QTransform().rotate(90*3);

            ne1 << zero << get(4, 1) << get(2, 1) << get(1, 1) << zero;
            se1 = ne1 * QTransform().rotate(90);
            sw1 = ne1 * QTransform().rotate(90*2);
            nw1 = ne1 * QTransform().rotate(90*3);

            const auto xform = QTransform().scale(size, size);

            QList<QPolygonF> result;

            result.append(square1(size));
            result.append(square4(size));
            result.append(square3(size));
            result.append(square2(size));

            result.append(north_left  * xform);
            result.append(north_right * xform);

            result.append(east_top    * xform);
            result.append(east_bottom * xform);

            result.append(south_right * xform);
            result.append(south_left  * xform);

            result.append(west_bottom * xform);
            result.append(west_top    * xform);

            /// one more square3, used at background.
            result.append(square3(size));

            result.append(nw0 * xform);
            result.append(nw1 * xform);

            result.append(ne0 * xform);
            result.append(ne1 * xform);

            result.append(se0 * xform);
            result.append(se1 * xform);

            result.append(sw0 * xform);
            result.append(sw1 * xform);

            return result;
        }

    private:
        QPolygonF _square1;
        QPolygonF _square2;
        QPolygonF _square3;
        QPolygonF _square4;

        /// These are the four main pieces of the cross, each having two halves.
        /// from the north, going CW.
        QPolygonF north_left;
        QPolygonF north_right;
        QPolygonF east_top;
        QPolygonF east_bottom;
        QPolygonF south_right;
        QPolygonF south_left;
        QPolygonF west_bottom;
        QPolygonF west_top;

        /// These are the secondary four pieces, each having two halves.
        /// Starting from north-eastern one, going CW.
        QPolygonF ne0;
        QPolygonF ne1;
        QPolygonF se0;
        QPolygonF se1;
        QPolygonF sw0;
        QPolygonF sw1;
        QPolygonF nw0;
        QPolygonF nw1;
    };
}
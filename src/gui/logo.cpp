/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "logo.hpp"
#include "SessionManager.hpp"
#include "theme/theme.hpp"

#include <QPainter>


using namespace gui;

QPixmap gui::createLogo(int size)
{
    const auto* tm = core::SessionManager::tm();

    auto result = QPixmap(size, size);
    const auto rec = result.rect();
    constexpr auto space = 3.0;
    constexpr auto scale = 2.75;

    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(result.rect(), tm->sceneColor());
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(tm->sceneLightColor(), 6));
    p.drawEllipse(result.rect().adjusted(3, 3, -3, -3));

    auto pen = QPen();
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setColor(tm->sceneDarkColor());
    pen.setWidth(2.0);
    pen.setDashPattern(QList<qreal>()
        << 3.0*scale << space
        << 1.0*scale << space
        << 4.0*scale << space
        << 1.0*scale << space
        << 5.0*scale << space
        << 9.0*scale << space
        << 2.0*scale << space
        << 6.0*scale << space
        << 0.0 << 100.0);

    p.setPen(pen);
    p.drawArc(rec.adjusted(3, 3, -3, -3), 0, 360 * 16);

    pen.setDashPattern(QList<qreal>() << 1.0 << 1.0);
    p.setPen(pen);
    p.drawArc(rec.adjusted(3, 3, -3, -3), 270 * 16, 90 * 16);

    return result;
}

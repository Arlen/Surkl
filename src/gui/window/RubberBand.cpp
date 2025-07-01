/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "RubberBand.hpp"
#include "core/SessionManager.hpp"
#include "theme/theme.hpp"

#include <QPainter>


using namespace gui::window;

RubberBand::RubberBand(QWidget *parent)
    : QRubberBand(QRubberBand::Line, parent)
{
    setGeometry(-1, -1, 1, 1);
}

void RubberBand::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    const auto* tm = core::SessionManager::tm();

    const QRect rec = rect();

    QPainter p(this);
    p.setBrush(Qt::NoBrush);
    p.setCompositionMode(QPainter::CompositionMode_Exclusion);
    p.setPen(tm->sceneLightColor());
    p.drawRect(rec.adjusted(0, 0, -1, -1));
}
/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SurklStyle.hpp"
#include "core/SessionManager.hpp"
#include "theme/theme.hpp"

#include <QPainter>
#include <QStyleOptionRubberBand>


using namespace gui;

void SurklStyle::drawControl(QStyle::ControlElement element, const QStyleOption *opt,
                             QPainter *p, const QWidget *widget) const
{
    if (element == CE_RubberBand) {
        const auto *tm = core::SessionManager::tm();

        p->save();
        p->setBrush(Qt::NoBrush);
        p->setCompositionMode(QPainter::CompositionMode_Exclusion);
        p->setPen(tm->sceneLightColor());
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        p->restore();
    } else {
        QProxyStyle::drawControl(element, opt, p, widget);
    }
}

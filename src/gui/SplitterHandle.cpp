/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SplitterHandle.hpp"
#include "core/SessionManager.hpp"
#include "theme/theme.hpp"

#include <QPainter>


using namespace gui;
using namespace gui::theme;

SplitterHandle::SplitterHandle(Qt::Orientation ori, QSplitter *parent)
    : QSplitterHandle(ori, parent)
{
}

void SplitterHandle::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto* tm = core::SessionManager::tm();

    QPainter p(this);
    p.fillRect(rect(), tm->sceneShadowColor());
    p.fillRect(rect().adjusted(1, 1, -1, -1),
               tm->sceneDarkColor());
}

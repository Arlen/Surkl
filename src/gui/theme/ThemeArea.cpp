/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "ThemeArea.hpp"
#include "ThemeSettings.hpp"


using namespace gui::theme;

ThemeArea::ThemeArea(window::Window *parent)
    : AbstractWindowArea(parent)
{
    setWidget(AreaType::ThemeArea, new ThemeSettings(this));

    setFocusPolicy(Qt::StrongFocus);
}

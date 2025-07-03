/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "ThemeArea.hpp"
#include "ThemeSettings.hpp"
#include "window/TitleBar.hpp"

#include <QVBoxLayout>


using namespace gui::theme;

ThemeArea::ThemeArea(window::Window *parent)
    : AbstractWindowArea(parent)
{
    _areaType = AreaType::ThemeArea;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(new ThemeSettings(this));

    setFocusPolicy(Qt::StrongFocus);
}

void ThemeArea::setTitleBar(window::TitleBar *tb)
{
    AbstractWindowArea::setTitleBar(tb);

    titleBar()->titleButton()->setText("Theme Settings");
}

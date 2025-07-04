/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "AbstractWindowArea.hpp"
#include "Window.hpp"

#include <QVBoxLayout>


using namespace gui::window;

AbstractWindowArea::AbstractWindowArea(Window *parent)
    : QWidget(parent)
{

}

void AbstractWindowArea::setWidget(AreaType type, QWidget* widget)
{
    Q_ASSERT(widget);
    Q_ASSERT(_widget == nullptr);
    Q_ASSERT(type != AreaType::InvalidArea);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _widget = widget;
    layout->addWidget(_widget);

    _type = type;
}

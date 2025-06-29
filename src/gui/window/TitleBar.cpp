/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "TitleBar.hpp"

#include <QHBoxLayout>


using namespace gui::window;

TitleBar::TitleBar(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layoutA = new QHBoxLayout(this);
    layoutA->setContentsMargins(0, 0, 0, 0);
    layoutA->setSpacing(0);

    _menuButton = new MenuButton(this);
    layoutA->addWidget(_menuButton);

    _splitButton = new SplitButton(this);
    layoutA->addWidget(_splitButton);

    _titleButton = new TitleButton(this);
    _titleButton->setContextMenuPolicy(Qt::NoContextMenu);
    layoutA->addWidget(_titleButton);

    _closeButton = new CloseButton(this);
    layoutA->addWidget(_closeButton);

    constexpr auto size = 22;

    setMaximumHeight(size);
    _splitButton->setMaximumSize(size, size);
    _menuButton->setMaximumSize(size, size);
    _titleButton->setMaximumHeight(size);
    _closeButton->setMaximumSize(size, size);
}

void TitleBar::setTitle(const QString& text) const
{
    _titleButton->setText(text);
}

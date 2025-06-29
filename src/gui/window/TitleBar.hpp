/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "buttons.hpp"

#include <QAbstractButton>


namespace gui::window
{
    class CloseButton;
    class MenuButton;
    class SplitButton;
    class TitleButton;

    class TitleBar final : public QWidget
    {
    public:
        explicit TitleBar(QWidget *parent = nullptr);

        void setTitle(const QString &text) const;

        QAbstractButton *splitButton() const { return _splitButton; }

        QAbstractButton *menuButton() const { return _menuButton; }

        QAbstractButton *titleButton() const { return _titleButton; }

        QAbstractButton *closeButton() const { return _closeButton; }

    private:
        SplitButton *_splitButton{nullptr};
        MenuButton *_menuButton{nullptr};
        TitleButton *_titleButton{nullptr};
        CloseButton *_closeButton{nullptr};
    };
}

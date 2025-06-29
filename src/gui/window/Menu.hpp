/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMenu>


namespace gui::window
{
    class Menu final : public QMenu
    {
        Q_OBJECT

      public:
        explicit Menu(QWidget* parent = nullptr);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
    };
}

/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPushButton>


namespace gui::view
{
    class QuadrantButton final : public QWidget
    {
        Q_OBJECT

    signals:
        void quad1Pressed();
        void quad2Pressed();
        void quad3Pressed();
        void quad4Pressed();
        void centerPressed();

    public:
        explicit QuadrantButton(QWidget *parent = nullptr);
    };
}
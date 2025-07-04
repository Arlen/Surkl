/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "window/AbstractWindowArea.hpp"


namespace gui::theme
{
    class ThemeArea final : public window::AbstractWindowArea
    {
        Q_OBJECT

    public:
        explicit ThemeArea(window::Window* parent = nullptr);
    };
}
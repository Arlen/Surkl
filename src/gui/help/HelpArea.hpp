/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "window/AbstractWindowArea.hpp"


class QTextEdit;

namespace gui::help
{
    class HelpArea final : public window::AbstractWindowArea
    {
        Q_OBJECT

    public:
        explicit HelpArea(window::Window* parent = nullptr);

        AreaType type() const override;

    private:
        QTextEdit* textEdit();
    };
}
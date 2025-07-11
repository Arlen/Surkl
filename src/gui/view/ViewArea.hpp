/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "window/AbstractWindowArea.hpp"


namespace core
{
    class FileSystemScene;
}

namespace gui::view
{
    class ViewArea final : public window::AbstractWindowArea
    {
        Q_OBJECT

    public:
        explicit ViewArea(core::FileSystemScene* scene, window::Window* parent = nullptr);

        AreaType type() const override;
    };
}
/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>


class QHBoxLayout;

namespace core
{
    class FileSystemScene;
}

namespace gui
{
    namespace view
    {
        class GraphicsView;
    }

    class MainWindow final : public QWidget
    {
    public:
        explicit MainWindow(core::FileSystemScene* scene, QWidget* parent = nullptr);

    private:
        QHBoxLayout* _layout{nullptr};
        view::GraphicsView* _view{nullptr};
    };
}
/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>


namespace gui::window
{
    class Window;

    class AbstractWindowArea : public QWidget
    {
    public:
        enum AreaType
        {
            ViewArea = 0,
            ThemeArea,
            /// others...

            InvalidArea,
        };

        explicit AbstractWindowArea(Window *parent);

        void setWidget(QWidget* widget);

        [[nodiscard]] QWidget* widget() const { return _widget; }

        [[nodiscard]] virtual AreaType type() const = 0;

    protected:
        QWidget* _widget{nullptr};
    };
}

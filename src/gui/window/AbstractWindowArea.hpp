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
            InvalidArea = -1,
            ViewArea = 0,
            ThemeArea = 1,
            /// others...
        };

        explicit AbstractWindowArea(Window *parent);

        void setWidget(AreaType typep, QWidget* widget);

        [[nodiscard]] QWidget* widget() const { return _widget; }

        [[nodiscard]] AreaType type() const { return _type; }

    protected:
        QWidget* _widget{nullptr};
        AreaType _type{InvalidArea};
    };
}

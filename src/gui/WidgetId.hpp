/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtTypes>


namespace gui
{
    template <class Widget>
    struct WidgetId
    {
        WidgetId() : _id{_class_id}
        {
            _class_id++;
        }

        [[nodiscard]] qint32 widgetId() const { return _id; }

    private:
        const qint32 _id{-1};

        inline static qint32 _class_id{0};
    };
}

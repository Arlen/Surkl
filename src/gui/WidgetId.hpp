/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtTypes>


namespace gui
{
    struct WidgetId
    {
        using ValueType = qint32;

        WidgetId()
        {
            static ValueType counter{0};

            _widgetId = counter++;
        }

        [[nodiscard]] ValueType widgetId() const noexcept
        {
            return _widgetId;
        }

    private:
        ValueType _widgetId{-1};
    };
}

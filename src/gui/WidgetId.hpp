/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtTypes>


namespace gui
{
    struct WidgetId
    {
        WidgetId()
        {
            static qint32 counter{0};

            _id = counter++;
        }

        [[nodiscard]] qint32 widgetId() const noexcept
        {
            return _id;
        }

    private:
        qint32 _id{-1};
    };
}

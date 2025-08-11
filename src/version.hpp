/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define SURKL_VERSION_MAJOR 0
#define SURKL_VERSION_MINOR 3
#define SURKL_VERSION_RELEASE 0

#include <QString>


inline QString version()
{
    return QString("%1.%2.%3")
        .arg(SURKL_VERSION_MAJOR)
        .arg(SURKL_VERSION_MINOR)
        .arg(SURKL_VERSION_RELEASE);
}

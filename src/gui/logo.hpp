/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPixmap>


namespace gui
{
    QPixmap createLogo(int size);

    void exportLogoPNG(int size, const QString& path);

    void exportLogoSVG(const QString& path);

    void exportLogo(const QString& path);

    void drawLogo(QPainter* p, const QRect& rec);
}
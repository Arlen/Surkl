/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "gui/theme/theme.hpp"

#include <QTest>



class TestTheme final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initTestCase_data();
    void cleanupTestCase();
    void init();
    void cleanup();

    void factoryPalette();

    void randomPalettes();
    void randomPalettes_data();

    void activePalette();
    void activePalette_data();

    void keepAndDiscardPalette();
    void keepAndDiscardPalette_data();

    void generateRangedPalette();
    void generateRangedPalette_data();

private:
    void generateRandomPalettes(int N);

    gui::theme::Colors _keptColors;
};

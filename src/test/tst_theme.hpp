/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QTest>

#include "theme.hpp"


class TestTheme final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initTestCase_data();
    void cleanupTestCase();
    void init();
    void cleanup();

    void factoryPalette() const;
    void randomPalettes() const;
    void activePalette() const;
    void keepAndDiscardPalette();

private:
    void generateRandomPalettes(int N);

    gui::Colors _keptColors;
};

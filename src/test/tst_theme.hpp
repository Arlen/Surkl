/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QTest>


class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initTestCase_data();
    void cleanupTestCase();
    void init();
    void cleanup();

    void paletteIdConversion();
};
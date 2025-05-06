/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "tst_theme.hpp"
#include "core/db.hpp"
#include "gui/theme.hpp"


using namespace core;
using namespace gui;

void TestTheme::initTestCase()
{
    db::init(db::DB_CONFIG_TEST);
    QVERIFY(QFile::exists(db::DB_CONFIG_TEST.databaseName));
}

void TestTheme::initTestCase_data()
{

}

void TestTheme::cleanupTestCase()
{
    QSqlDatabase::removeDatabase(db::DB_CONFIG_TEST.databaseName);
    QVERIFY(QFile::exists(db::DB_CONFIG_TEST.databaseName));
    QFile::remove(db::DB_CONFIG_TEST.databaseName);
    QVERIFY(!QFile::exists(db::DB_CONFIG_TEST.databaseName));
}

void TestTheme::init()
{

}

void TestTheme::cleanup()
{

}

void TestTheme::paletteIdConversion()
{
    const auto factoryPalette = ThemeManager::factory();
    const auto factoryId      = ThemeManager::idFromPalette(factoryPalette);
    const auto palette        = ThemeManager::paletteFromId(factoryId);
    const auto id             = ThemeManager::idFromPalette(palette);

    for (int i = 0; i < PLACEHOLDER_COLOR_1; ++i) {
        auto c1 = factoryPalette[i];
        auto c2 = palette[i];
        QCOMPARE_EQ(c2, c1);
    }

    QCOMPARE(id, factoryId);
}


QTEST_MAIN(TestTheme)

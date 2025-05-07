/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "tst_theme.hpp"
#include "core/db.hpp"
#include "gui/theme.hpp"

#include <QRandomGenerator>

// ReSharper disable CppMemberFunctionMayBeStatic


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

void TestTheme::factoryPalette() const
{
    constexpr auto factoryPal = ThemeManager::factory();
    const auto factoryId      = ThemeManager::idFromPalette(factoryPal);
    const auto palette        = ThemeManager::paletteFromId(factoryId);
    const auto id             = ThemeManager::idFromPalette(palette);

    for (int i = 0; i < PLACEHOLDER_COLOR_1; ++i) {
        const auto c1 = factoryPal[i];
        const auto c2 = palette[i];
        QCOMPARE_EQ(c2, c1);
    }

    QCOMPARE(id, factoryId);
}

void TestTheme::randomPalettes() const
{
    QFETCH(std::string, id);
    QFETCH(Palette, palette);

    const auto pfi = ThemeManager::paletteFromId(id);
    const auto ifp = ThemeManager::idFromPalette(pfi);

    for (int i = 0; i < PLACEHOLDER_COLOR_1; ++i) {
        const auto c1 = palette[i];
        const auto c2 = pfi[i];
        QCOMPARE_EQ(c2, c1);
    }

    QCOMPARE(ifp, id);
}

void TestTheme::randomPalettes_data()
{
    QTest::addColumn<std::string>("id");
    QTest::addColumn<Palette>("palette");

    for (int i = 0; i < 1024; ++i) {
        auto palette = Palette{};
        palette.fill(QColor(0, 0, 0, 0));
        for (int j = 0; j < PLACEHOLDER_COLOR_1; ++j) {
            const auto h = QRandomGenerator::global()->bounded(1.0);
            const auto s = QRandomGenerator::global()->bounded(1.0);
            const auto v = QRandomGenerator::global()->bounded(1.0);
            const auto a = QRandomGenerator::global()->bounded(1.0);
            const auto c = QColor::fromHsvF(h, s, v, a).rgba();
            palette[j] = c;
        }
        QTest::newRow(QString::number(i).toLatin1())
            << ThemeManager::idFromPalette(palette)
            << palette;
    }
}


QTEST_MAIN(TestTheme)

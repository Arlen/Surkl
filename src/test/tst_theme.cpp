/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "tst_theme.hpp"
#include "core/db.hpp"
#include "core/SessionManager.hpp"
#include "gui/ThemeSettings.hpp"

#include <QRandomGenerator>
#include <QStandardItemModel>

#include <ranges>


using namespace core;
using namespace gui;

void TestTheme::initTestCase()
{
    qApp->setProperty(db::DB_NAME, db::DB_CONFIG_TEST.databaseName);
    qApp->setProperty(db::DB_CONNECTION_NAME, db::DB_CONFIG_TEST.connectionName);

    auto* tm = SessionManager::tm();
    constexpr auto factoryPal = ThemeManager::factory();
    const auto factoryId  = ThemeManager::idFromPalette(factoryPal);
    QVERIFY(tm->isActive(factoryId));
}

void TestTheme::initTestCase_data()
{

}

void TestTheme::cleanupTestCase()
{
    const auto databaseName   = qApp->property(db::DB_NAME).toString();
    const auto connectionName = qApp->property(db::DB_CONNECTION_NAME).toString();

    QVERIFY(QFile::exists(databaseName));
    QFile::remove(databaseName);
    QVERIFY(!QFile::exists(databaseName));
}

void TestTheme::init()
{

}

void TestTheme::cleanup()
{

}

void TestTheme::factoryPalette()
{
    constexpr auto factoryPal = ThemeManager::factory();
    const auto factoryId      = ThemeManager::idFromPalette(factoryPal);
    const auto palette        = ThemeManager::paletteFromId(factoryId);
    const auto id             = ThemeManager::idFromPalette(palette);

    for (int i = 0; i < PaletteIndexSize; ++i) {
        const auto c1 = factoryPal[i];
        const auto c2 = palette[i];
        QCOMPARE_EQ(c2, c1);
    }

    QCOMPARE(id, factoryId);
}

void TestTheme::randomPalettes()
{
    QFETCH(std::string, id);
    QFETCH(Palette, palette);

    const auto pfi = ThemeManager::paletteFromId(id);
    const auto ifp = ThemeManager::idFromPalette(pfi);

    for (int i = 0; i < PaletteIndexSize; ++i) {
        const auto c1 = palette[i];
        const auto c2 = pfi[i];
        QCOMPARE_EQ(c2, c1);
    }

    QCOMPARE(ifp, id);
}

void TestTheme::randomPalettes_data()
{
    generateRandomPalettes(1024);
}

void TestTheme::activePalette()
{
    constexpr auto factoryPal = ThemeManager::factory();
    const auto factoryId      = ThemeManager::idFromPalette(factoryPal);

    QFETCH(std::string, id);
    QFETCH(Palette, palette);

    auto* tm = SessionManager::tm();
    QVERIFY(tm->isActive(factoryId));
    QVERIFY(!tm->isActive(id));
    tm->setActivePalette(palette);
    QVERIFY(tm->isActive(id));
    tm->switchTo(factoryId);
    QVERIFY(!tm->isActive(id));
    QVERIFY(tm->isActive(factoryId));
}

void TestTheme::activePalette_data()
{
    generateRandomPalettes(1024);
}

void TestTheme::keepAndDiscardPalette()
{
    QFETCH(std::string, id);
    QFETCH(Palette, palette);

    constexpr auto factoryPal = ThemeManager::factory();
    const auto factoryId      = ThemeManager::idFromPalette(factoryPal);
    auto* tm = SessionManager::tm();
    auto* model = tm->model();

    const auto keep    = QRandomGenerator::global()->bounded(0, 2) == false; // %50
    const auto discard = QRandomGenerator::global()->bounded(0, 5) == false; // %25

    auto itemCount = [model](const PaletteId& id) -> int {
        const auto text = QString::fromStdString(id);
        return model->findItems(text, Qt::MatchExactly, ThemeManager::PaletteIdColumn)
            .count();
    };

    if (keep) {
        if (_keptColors.contains(id)) {
            QCOMPARE(itemCount(id), 1);
        } else {
            QCOMPARE(itemCount(id), 0);
            tm->keep(palette);
            QCOMPARE(tm->isActive(id), true);
            QCOMPARE(itemCount(id), 1);
            _keptColors[id] = palette;
        }
    }
    if (discard) {
        if (_keptColors.contains(id)) {
            QCOMPARE(itemCount(id), 1);
            const bool wasActive = tm->isActive(id);
            tm->discard(id);
            QCOMPARE(tm->isActive(id), false);
            QCOMPARE(itemCount(id), 0);
            if (wasActive) {
                QCOMPARE(tm->isActive(factoryId), true);
            }
            _keptColors.erase(id);
        }
    }
}

void TestTheme::keepAndDiscardPalette_data()
{
    generateRandomPalettes(1024);
}

void TestTheme::generateRangedPalette()
{
    QFETCH(qreal, p1);
    QFETCH(qreal, p2);
    constexpr auto count = 1024;

    for (int i = 0; i < count; ++i) {
        HsvRange range {{p1*360, p2*360}, {p1, p2}, {p1, p2}};
        const auto palette = generatePalette(range);
        const auto ifp = ThemeManager::idFromPalette(palette);
        const auto pfi = ThemeManager::paletteFromId(ifp);

        QCOMPARE(pfi, palette);
    }
}

void TestTheme::generateRangedPalette_data()
{
    QTest::addColumn<double>("p1");
    QTest::addColumn<double>("p2");

    QTest::newRow("normal: (0,1)")     << 0.0 << 1.0;
    QTest::newRow("normal: (0.2,0.8)") << 0.2 << 0.8;
    QTest::newRow("normal: (0.4,0.6)") << 0.4 << 0.6;

    QTest::newRow("wrap: (1,0)")     << 1.0 << 0.0;
    QTest::newRow("wrap: (0.8,0.2)") << 0.8 << 0.2;
    QTest::newRow("wrap: (0.6,0.4)") << 0.6 << 0.4;
}

void TestTheme::generateRandomPalettes(int N)
{
    QTest::addColumn<PaletteId>("id");
    QTest::addColumn<Palette>("palette");

    auto* rng = QRandomGenerator::global();
    std::vector<Palette> palettes;

    for (int i = 0; i < N; ++i) {
        auto palette = Palette{};
        palette.fill(QColor(0, 0, 0, 0));
        for (int j = 0; j < PaletteIndexSize; ++j) {
            const auto h = rng->bounded(1.0);
            const auto s = rng->bounded(1.0);
            const auto v = rng->bounded(1.0);
            const auto a = rng->bounded(1.0);
            const auto c = QColor::fromHsvF(h, s, v, a).rgba();
            palette[j] = c;
        }
        palettes.push_back(palette);

        /// ~ %33 duplicates
        if (rng->bounded(0, 3) == false) {
            palettes.push_back(palette);
        }
    }

    std::ranges::shuffle(palettes, *QRandomGenerator::global());

    for (int i = 0; const auto& palette : palettes) {
        const auto id = ThemeManager::idFromPalette(palette);
        QTest::newRow(QString::number(i++).toLatin1())
            << id << palette;
    }
}


QTEST_MAIN(TestTheme)

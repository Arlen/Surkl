/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "test_bookmark.hpp"

#include "core/SessionManager.hpp"
#include "core/bookmark.hpp"
#include "core/db.hpp"

#include <QTest>


void TestBookmarks::initTestCase()
{
    qApp->setProperty(core::db::DB_NAME
        , core::db::DB_CONFIG_TEST.databaseName);
    qApp->setProperty(core::db::DB_CONNECTION_NAME
        , core::db::DB_CONFIG_TEST.connectionName);

    if (auto dbFile = QFile(qApp->property(core::db::DB_NAME).toString()); dbFile.exists()) {
        dbFile.remove();
    }

    auto* bm = core::SessionManager::bm();
    QVERIFY(bm->sceneBookmarks().size() == 0);
}

void TestBookmarks::initTestCase_data()
{
    QTest::addColumn<QPoint>("pos");
    QTest::addColumn<QString>("name");

    QTest::newRow("1st-bookmark") << QPoint(2, 3) << "Alpha";
    QTest::newRow("2nd-bookmark") << QPoint(5, 7) << "Beta";
    QTest::newRow("3rd-bookmark") << QPoint(2, 4) << "Gamma";
    QTest::newRow("4th-bookmark") << QPoint(6, 8) << "Delta";
    QTest::newRow("5th-bookmark") << QPoint(5, 7) << "duplicate";
}

void TestBookmarks::cleanupTestCase()
{
    auto dbFile = QFile(qApp->property(core::db::DB_NAME).toString());

    QVERIFY(dbFile.exists());
    QVERIFY(dbFile.remove());
}

void TestBookmarks::init()
{

}

void TestBookmarks::sceneBookmarkInsert()
{
    QFETCH_GLOBAL(QPoint, pos);
    QFETCH_GLOBAL(QString, name);

    if (auto* bm = core::SessionManager::bm(); bm->sceneBookmarks().contains({pos})) {
        QCOMPARE(name, "duplicate");
    } else {
        bm->insertBookmark({pos, name});
        QCOMPARE(bm->sceneBookmarks().contains({pos}), true);
    }
}

void TestBookmarks::sceneBookmarkUpdate()
{
    QFETCH_GLOBAL(QPoint, pos);
    QFETCH_GLOBAL(QString, name);

    if (auto* bm = core::SessionManager::bm(); bm->sceneBookmarks().contains({pos})) {
        bm->updateBookmark({pos, name + "Updated"});
        const auto bookmarks = bm->sceneBookmarks();
        QCOMPARE(bookmarks.contains({pos}), true);
        const auto updateName = bookmarks.find({pos});
        QVERIFY(updateName != bookmarks.end());
        QCOMPARE(updateName->name, name + "Updated");
    }
}

void TestBookmarks::sceneBookmarkRemove()
{
    QFETCH_GLOBAL(QPoint, pos);
    QFETCH_GLOBAL(QString, name);

    if (auto* bm = core::SessionManager::bm(); name == "duplicate") {
        QCOMPARE(bm->sceneBookmarks().contains({pos}), false);
    } else {
        QCOMPARE(bm->sceneBookmarks().contains({pos}), true);
        bm->removeBookmarks({{pos, name}});
        QCOMPARE(bm->sceneBookmarks().contains({pos}), false);
    }
}

QTEST_MAIN(TestBookmarks)

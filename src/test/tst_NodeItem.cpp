/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "tst_NodeItem.hpp"
#include "core/FileSystemScene.hpp"
#include "core/NodeItem.hpp"
#include "core/SceneStorage.hpp"
#include "core/SessionManager.hpp"
#include "db/db.hpp"

#include <QFileSystemModel>
#include <QRandomGenerator>
#include <QSignalSpy>

#include <algorithm>
#include <random>
#include <unordered_set>
#include <vector>


using namespace std;

namespace
{
    auto fileOrClosedDirAreSorted = [](const core::NodeItem* node)
    {
        auto rows = node->childEdges()
            | core::asFilesOrClosedTargetNodes
            | core::asIndexRow
            ;

        return ranges::is_sorted(ranges::begin(rows), ranges::end(rows));
    };

    auto allSorted = [](const core::NodeItem* parent)
    {
        auto rows = parent->childEdges()
            | core::asTargetNode
            | core::asIndexRow
            ;

        return ranges::is_sorted(ranges::begin(rows), ranges::end(rows));
    };

    auto uniqueRowCount = [](const core::NodeItem* parent)
    {
        const auto rows = parent->childEdges()
            | core::asTargetNode
            | core::asIndexRow
            | ranges::to<std::unordered_set>()
            ;

        return rows.size();
    };

    auto nodeFromPath = [](const core::FileSystemScene* scene, const QString& path) -> core::NodeItem*
    {
        if (const auto index = scene->index(path); index.isValid()) {
            const auto items = scene->items();

            auto nodes = items
                | views::filter(&core::asNodeItem)
                | views::transform(&core::asNodeItem)
                ;

            for (auto* n : nodes) {
                if (n->index() == index) {
                    return n;
                }
            }
        }

        return nullptr;
    };
}

#define TEST_TRACK_STEPS

#ifdef TEST_TRACK_STEPS
#define SHOW_STEPS()  \
    do { qDebug() << _steps; } while (false) \

#else
#define SHOW_STEPS() \

#endif

void TestNodeItem::initTestCase()
{
    qApp->setProperty(core::db::DB_NAME
        , core::db::DB_CONFIG_TEST.databaseName);
    qApp->setProperty(core::db::DB_CONNECTION_NAME
        , core::db::DB_CONFIG_TEST.connectionName);

    if (auto dbFile = QFile(qApp->property(core::db::DB_NAME).toString()); dbFile.exists()) {
        dbFile.remove();
    }

    auto* scene = core::SessionManager::scene();
    auto dir = QDir(ROOT_PATH);
    scene->setRootPath(dir.absolutePath());
    core::SessionManager::ss()->loadScene(scene);

    /// wait for filesystem data to be fetched
    QTest::qWait(50);

    _scene = scene;
}

void TestNodeItem::initTestCase_data()
{
    QDir dir;
    QVERIFY(dir.mkpath(ROOT_PATH));
    dir.cd(ROOT_PATH);

    for (int i = 0; i < 50; ++i) {
        auto name = QString("A%1")
            .arg(i, 2, 10, QChar('0'));
        QVERIFY(dir.mkpath(name));
    }
    QTest::addColumn<QDir>("testDir");
    QTest::newRow("one-level") << dir;
}

void TestNodeItem::cleanupTestCase()
{
    auto dir = QDir(ROOT_PATH);
    QVERIFY(dir.removeRecursively());

    auto dbFile = QFile(qApp->property(core::db::DB_NAME).toString());

    QVERIFY(dbFile.exists());
    QVERIFY(dbFile.remove());
}

void TestNodeItem::init()
{
}

void TestNodeItem::cleanup()
{
    _steps.clear();
}

void TestNodeItem::rotation_data()
{
    QTest::addColumn<QPair<qreal,qreal>>("rotWeights");
    QTest::newRow("50/50") << QPair{50.0, 50.0};
    QTest::newRow("60/40") << QPair{60.0, 40.0};
    QTest::newRow("40/60") << QPair{40.0, 60.0};
    QTest::newRow("90/10") << QPair{90.0, 10.0};
    QTest::newRow("10/90") << QPair{10.0, 90.0};
    QTest::newRow("100/0") << QPair{100.0, 0.0};
    QTest::newRow("0/100") << QPair{0.0, 100.0};
}

void TestNodeItem::rotation()
{
    QFETCH_GLOBAL(QDir, testDir);
    QFETCH(WeightPair, rotWeights);

    auto* node = nodeFromPath(_scene, testDir.path());
    QVERIFY(node != nullptr);
    QVERIFY(_scene->isDir(node->index()));
    QVERIFY(_scene == node->scene());

    if (node->isClosed()) {
        node->open();

        /// wait for filesystem data to be fetched
        QTest::qWait(25);
    }

    QCOMPARE(node->childEdges().size(), qMin(core::NodeItem::NODE_CHILD_COUNT, testDir.count()));
    QCOMPARE(uniqueRowCount(node), node->childEdges().size());
    QVERIFY(fileOrClosedDirAreSorted(node));
    verifyNames(node, testDir);

    constexpr auto MAX_ITER = core::NodeItem::NODE_CHILD_COUNT;
    QSignalSpy spy1(_scene, &core::FileSystemScene::sequenceFinished);

    for (int k = 1; k <= MAX_ITER; ++k) {
        for (int i = 1; i <= MAX_ITER; ++i) {
            doRandomRotations(node, rotWeights, i*k);
            QVERIFY(spy1.wait(250));
            QCOMPARE(uniqueRowCount(node), node->childEdges().size());
            QVERIFY(fileOrClosedDirAreSorted(node));
            verifyNames(node, testDir);
        }
        node->close();
        node->open();

        QVERIFY(fileOrClosedDirAreSorted(node));
        QCOMPARE(uniqueRowCount(node), node->childEdges().size());
        verifyNames(node, testDir);
    }
}

void TestNodeItem::rotationOpenCloseSubdir_data()
{
    QTest::addColumn<QPair<qreal,qreal>>("rotWeights");
    QTest::newRow("50/50") << QPair{50.0, 50.0};
    QTest::newRow("60/40") << QPair{60.0, 40.0};
    QTest::newRow("40/60") << QPair{40.0, 60.0};
    QTest::newRow("90/10") << QPair{90.0, 10.0};
    QTest::newRow("10/90") << QPair{10.0, 90.0};
    QTest::newRow("100/0") << QPair{100.0, 0.0};
    QTest::newRow("0/100") << QPair{0.0, 100.0};
}

void TestNodeItem::rotationOpenCloseSubdir()
{
    QFETCH_GLOBAL(QDir, testDir);
    QFETCH(WeightPair, rotWeights);

    auto* node = nodeFromPath(_scene, testDir.path());

    if (node->isClosed()) {
        node->open();

        /// wait for filesystem data to be fetched
        QTest::qWait(25);
    }

    QCOMPARE(node->childEdges().size(), qMin(core::NodeItem::NODE_CHILD_COUNT, testDir.count()));
    QCOMPARE(uniqueRowCount(node), node->childEdges().size());
    QVERIFY(fileOrClosedDirAreSorted(node));
    verifyNames(node, testDir);

    constexpr auto MAX_ITER = core::NodeItem::NODE_CHILD_COUNT;
    QSignalSpy animSpy(_scene, &core::FileSystemScene::sequenceFinished);

    for (int k = 1; k <= MAX_ITER; ++k) {
        for (int i = 1; i <= MAX_ITER; ++i) {
            doRandomRotations(node, rotWeights, i*k);
            animSpy.wait(250);
            QCOMPARE(node->childEdges().size(), qMin(core::NodeItem::NODE_CHILD_COUNT, testDir.count()));
            QCOMPARE(uniqueRowCount(node), node->childEdges().size());
            QVERIFY(fileOrClosedDirAreSorted(node));
            verifyNames(node, testDir);

            randomOpen(node, i);
            doRandomRotations(node, rotWeights, i);
            animSpy.wait(250);
            QCOMPARE(node->childEdges().size(), qMin(core::NodeItem::NODE_CHILD_COUNT, testDir.count()));
            QCOMPARE(uniqueRowCount(node), node->childEdges().size());
            QVERIFY(fileOrClosedDirAreSorted(node));
            verifyNames(node, testDir);

            randomClose(node, i);
            animSpy.wait(250);
            QCOMPARE(node->childEdges().size(), qMin(core::NodeItem::NODE_CHILD_COUNT, testDir.count()));
            QCOMPARE(uniqueRowCount(node), node->childEdges().size());
            QVERIFY(fileOrClosedDirAreSorted(node)); /// TODO: this fails
            SHOW_STEPS();
            verifyNames(node, testDir);

            doRandomRotations(node, rotWeights, i);
            animSpy.wait(250);
            QCOMPARE(node->childEdges().size(), qMin(core::NodeItem::NODE_CHILD_COUNT, testDir.count()));
            QCOMPARE(uniqueRowCount(node), node->childEdges().size());
            QVERIFY(fileOrClosedDirAreSorted(node));
            verifyNames(node, testDir);
        }

#ifdef TEST_TRACK_STEPS
        _steps.clear();
#endif
        node->close();
        animSpy.wait(250);
        QCOMPARE(uniqueRowCount(node), 0);
        node->open();
        QCOMPARE(uniqueRowCount(node), node->childEdges().size());
        QVERIFY(allSorted(node));
        QVERIFY(fileOrClosedDirAreSorted(node));
        verifyNames(node, testDir);
    }
    node->close();
    animSpy.wait(250);
    QCOMPARE(node->childEdges().size(), 0);
}

void TestNodeItem::verifyNames(core::NodeItem* node, const QDir& dir)
{
    QCOMPARE(node->childEdges().empty(), dir.isEmpty());

    for (const auto* child : node->childEdges() | core::asTargetNode) {
        QVERIFY(dir.exists(child->name()));
        QCOMPARE(child->index().data().toString(), child->name());
        QCOMPARE(child->parentEdge()->label()->text(), child->name());
    }
}

void TestNodeItem::doRandomRotations(core::NodeItem* node, const QPair<qreal, qreal>& weights, int count)
{
    std::discrete_distribution<> dd({weights.first, weights.second});
    for (int i = 0; i < count; ++i) {
        const auto randomPick = dd(*QRandomGenerator::global());
        QVERIFY(randomPick == 0 || randomPick == 1);
        const auto rot = randomPick == 0 ? core::Rotation::CCW : core::Rotation::CW;
        node->rotate(rot);

#ifdef TEST_TRACK_STEPS
        _steps += QString("%1(%2),")
            .arg(rot == core::Rotation::CCW ? "CCW" : "CW")
            .arg(QString::number(count));
#endif
    }
}

void TestNodeItem::randomOpen(const core::NodeItem* node, int count)
{
    auto candidates = node->childEdges()
        | core::asTargetNode
        | views::filter(&core::NodeItem::isDir)
        | views::filter(std::not_fn(&core::NodeItem::isOpen))
        | ranges::to<std::vector>()
        ;
    ranges::shuffle(candidates, *QRandomGenerator::global());

    for (auto* child : candidates | views::take(count)) {
        QVERIFY(!child->isOpen());
        child->open();

#ifdef TEST_TRACK_STEPS
        _steps += QString("O(%1),").arg(child->name());
#endif
    }
}

void TestNodeItem::randomClose(const core::NodeItem* node, int count)
{
    auto candidates = node->childEdges()
        | core::asTargetNode
        | views::filter(&core::NodeItem::isDir)
        | views::filter(std::not_fn(&core::NodeItem::isClosed))
        | ranges::to<std::vector>()
        ;
    ranges::shuffle(candidates, *QRandomGenerator::global());

    for (auto* child : candidates | views::take(count)) {
        QVERIFY(!child->isClosed());
        child->close();

#ifdef TEST_TRACK_STEPS
        _steps += QString("C(%1),").arg(child->name());
#endif
    }
}

void TestNodeItem::closeAll(core::NodeItem* node)
{
    auto notClosed = node->childEdges()
        | core::asTargetNode
        | views::filter(&core::NodeItem::isDir)
        | views::filter(std::not_fn(&core::NodeItem::isClosed));

    for (auto* child : notClosed) {
        child->close();
    }
}


QTEST_MAIN(TestNodeItem)
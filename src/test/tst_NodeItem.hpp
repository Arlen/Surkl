/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QTest>
#include <QGraphicsView>


namespace core
{
    class FileSystemScene;
    class NodeItem;
}

using WeightPair = QPair<qreal, qreal>;

class TestNodeItem final : public QObject
{
    Q_OBJECT

    static constexpr auto ROOT_PATH = "TestDataSurkl";

private slots:
    void initTestCase();
    void initTestCase_data();
    void cleanupTestCase();
    void init();
    void cleanup();

    void rotation_data();
    void rotation();
    void rotationOpenCloseSubdir_data();
    void rotationOpenCloseSubdir();

private:
    void verifyNames(core::NodeItem* node, const QDir& dir);
    void verifyIndices(const core::NodeItem* node);
    void doRandomRotations(core::NodeItem* node, const WeightPair& weights, int count = 1);

    void randomOpen(const core::NodeItem* node, int count);
    void randomClose(const core::NodeItem* node, int count);

    void closeAll(core::NodeItem* node);

    QString _steps;
};


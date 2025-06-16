/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>


class TestBookmarks final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initTestCase_data();
    void cleanupTestCase();

    void init();

    void sceneBookmarkInsert();
    void sceneBookmarkUpdate();
    void sceneBookmarkRemove();
};

#pragma once

#include <QTest>


namespace core
{
    class Scene;
    class BookmarkManager;
}

class TestBookmarks : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void bookmarkSingle();

private:
    core::Scene *_scene;
    core::BookmarkManager *_bm;
};

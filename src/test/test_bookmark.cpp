#include "test_bookmark.hpp"

#include "core/db.hpp"
#include "core/Scene.hpp"
#include "core/SessionManager.hpp"
#include "core/bookmark.hpp"
#include "view/GraphicsView.hpp"


void TestBookmarks::initTestCase()
{
    QFile db(core::db::DB_DATABASE_NAME);
    if (db.exists()) {
        db.remove();
    }

    core::session()->start();
    core::session()->mw()->resize(640*2, 480*2);
    core::session()->mw()->show();

    _scene = core::session()->scene();
    QVERIFY(_scene != nullptr);
    _bm = core::session()->bm();
    QVERIFY(_bm != nullptr);
    QVERIFY(_bm->sceneBookmarks().size() == 0);
}

void TestBookmarks::cleanupTestCase()
{
    QFile db(core::db::DB_DATABASE_NAME);
    QVERIFY(db.exists());
    QVERIFY(db.remove());
}

void TestBookmarks::init()
{

}

void TestBookmarks::bookmarkSingle()
{
    QVERIFY(_bm->sceneBookmarks().size() == 0);
    QVERIFY(_scene->views().size() > 0);

    if (auto* view = dynamic_cast<gui::view::GraphicsView*>(_scene->views().first())) {
        QVERIFY(view != nullptr);
        view->setFocus();
        auto* vp = view->viewport();
        QVERIFY(vp != nullptr);
        QVERIFY(_bm->sceneBookmarks().size() == 0);

        QTest::mouseMove(vp);
        QTest::keyPress(vp, Qt::Key_B, Qt::NoModifier);

        const auto bmPos  = QPoint(300, 300);
        QTest::mousePress(vp, Qt::LeftButton, Qt::NoModifier, bmPos);
        QVERIFY(_bm->sceneBookmarks().contains({bmPos, ""}));
        _bm->removeBookmark({bmPos, ""});
        QVERIFY(!_bm->sceneBookmarks().contains({bmPos, ""}));
        QVERIFY(_bm->sceneBookmarks().size() == 0);
    }
}

QTEST_MAIN(TestBookmarks)

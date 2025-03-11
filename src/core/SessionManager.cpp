#include "core/SessionManager.hpp"
#include "core/Scene.hpp"
#include "core/bookmark.hpp"
#include "gui/MainWindow.hpp"
#include "gui/theme.hpp"

#include <QApplication>


using namespace core;

namespace
{
    constexpr auto OBJ_NAME = QLatin1StringView("ksed-session-manager");
}


SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{

}

Scene* SessionManager::scene()
{
    return session()->_sc;
}

BookmarkManager* SessionManager::bm()
{
    return session()->_bm;
}

gui::ThemeManager* SessionManager::tm()
{
    return session()->_tm;
}

gui::MainWindow* SessionManager::mw()
{
    return session()->_mw;
}

void SessionManager::cleanup() const
{
    gui::ThemeManager::saveToDatabase(_tm);
    BookmarkManager::saveToDatabase(_bm);
}

void SessionManager::init()
{
    _tm = new gui::ThemeManager(this);
    gui::ThemeManager::configure(_tm);

    _bm = new BookmarkManager(this);
    BookmarkManager::configure(_bm);

    _sc = new Scene(this);
    _mw = new gui::MainWindow(_sc);

    connect(qApp, &QApplication::aboutToQuit, _mw, &QWidget::deleteLater);
    connect(qApp, &QApplication::aboutToQuit, this, &SessionManager::cleanup);
}

SessionManager* SessionManager::session()
{
    assert(qApp != nullptr);

    SessionManager* sm{nullptr};

    if (sm = qApp->findChild<SessionManager*>(OBJ_NAME); sm == nullptr) {
        sm = new SessionManager(qApp);
        sm->setObjectName(OBJ_NAME);
        sm->init();
    }

    return sm;
}
#include "core/SessionManager.hpp"
#include "core/Scene.hpp"
#include "core/bookmark.hpp"
#include "gui/MainWindow.hpp"
#include "gui/theme.hpp"

#include <QApplication>


using namespace core;

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
    , _initialized(false)
{

}

SessionManager::~SessionManager()
{
    gui::ThemeManager::saveToDatabase(_tm);
    BookmarkManager::saveToDatabase(_bm);
}

void SessionManager::start()
{
    if (_initialized) {
        return;
    }

    initialize();

    _initialized = true;
}

Scene* SessionManager::scene() const
{
    return _scene;
}

BookmarkManager* SessionManager::bm() const
{
    return _bm;
}

gui::ThemeManager* SessionManager::tm() const
{
    return _tm;
}

gui::MainWindow* SessionManager::mw() const
{
    return _mw;
}

void SessionManager::initialize()
{
    _tm = new gui::ThemeManager(this);
    gui::ThemeManager::configure(_tm);

    _bm = new BookmarkManager(this);
    BookmarkManager::configure(_bm);

    _scene = new Scene(this);

    _mw = new gui::MainWindow(_scene);
}

SessionManager* core::session()
{
    static auto* sm = new SessionManager(qApp);

    return sm;
}

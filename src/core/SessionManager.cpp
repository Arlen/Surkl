/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SessionManager.hpp"
#include "FileSystemScene.hpp"
#include "bookmark.hpp"
#include "db.hpp"
#include "gui/MainWindow.hpp"
#include "gui/theme.hpp"

#include <QApplication>


using namespace core;

namespace
{
    constexpr auto OBJ_NAME = QLatin1StringView("surkl-session-manager");
}


SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{

}

SessionManager::~SessionManager()
{
    delete _mw;
}

FileSystemScene* SessionManager::scene()
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
    BookmarkManager::saveToDatabase(_bm);
}

void SessionManager::init()
{
    db::init();

    _tm = new gui::ThemeManager(this);
    gui::ThemeManager::configure(_tm);

    _bm = new BookmarkManager(this);
    BookmarkManager::configure(_bm);

    _sc = new FileSystemScene(this);
    FileSystemScene::configure(_sc);

    _mw = new gui::MainWindow(_sc);

    connect(qApp, &QApplication::aboutToQuit, this, &SessionManager::cleanup);

    connect(_tm, &gui::ThemeManager::themeChanged, _sc, &FileSystemScene::refreshItems);
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
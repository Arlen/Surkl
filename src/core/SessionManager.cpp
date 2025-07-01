/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SessionManager.hpp"
#include "FileSystemScene.hpp"
#include "SceneStorage.hpp"
#include "bookmark.hpp"
#include "db.hpp"
#include "gui/MainWindow.hpp"
#include "gui/theme/theme.hpp"

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

BookmarkManager* SessionManager::bm()
{
    return session()->_bm;
}

FileSystemScene* SessionManager::scene()
{
    return session()->_sc;
}

SceneStorage* SessionManager::ss()
{
    return session()->_ss;
}

gui::MainWindow* SessionManager::mw()
{
    return session()->_mw;
}

gui::theme::ThemeManager* SessionManager::tm()
{
    return session()->_tm;
}

void SessionManager::cleanup() const
{
    BookmarkManager::saveToDatabase(_bm);

    /// TODO: interactive save does not cover everything.
    _ss->saveScene();
}

void SessionManager::init()
{
    db::init();

    _tm = new gui::theme::ThemeManager(this);
    gui::theme::ThemeManager::configure(_tm);

    _bm = new BookmarkManager(this);
    BookmarkManager::configure(_bm);

    _sc = new FileSystemScene(this);

    _ss = new SceneStorage(this);
    SceneStorage::configure();

    _mw = new gui::MainWindow(_sc);

    connect(qApp, &QApplication::aboutToQuit, this, &SessionManager::cleanup);

    connect(_tm, &gui::theme::ThemeManager::themeChanged, _sc, &FileSystemScene::refreshItems);
}

SessionManager* SessionManager::session()
{
    Q_ASSERT(qApp != nullptr);

    SessionManager* sm{nullptr};

    if (sm = qApp->findChild<SessionManager*>(OBJ_NAME); sm == nullptr) {
        sm = new SessionManager(qApp);
        sm->setObjectName(OBJ_NAME);
        sm->init();
    }

    return sm;
}
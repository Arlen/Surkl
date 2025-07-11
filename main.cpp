/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SceneStorage.hpp"
#include "SurklStyle.hpp"
#include "core/SessionManager.hpp"
#include "db.hpp"
#include "version.hpp"

#include <QApplication>
#include <QSqlDatabase>

#include <print>


int main(int argc, char* argv[])
{
    QScopedPointer app(new QApplication(argc, argv));

    QApplication::setApplicationName("surkl");

    std::println("Surkl {},", version().toStdString());
    std::println("Copyright (C) 2025 Arlen Avakian");

    if (!QSqlDatabase::drivers().contains("QSQLITE"))
    {
        qWarning() << "Error: QSQLITE database driver not available.";
        return -1;
    }

    app->setProperty(core::db::DB_NAME
        , core::db::DB_CONFIG.databaseName);
    app->setProperty(core::db::DB_CONNECTION_NAME
        , core::db::DB_CONFIG.connectionName);

    app->setStyle(new gui::SurklStyle());

    core::SessionManager::mw()->show();
    core::SessionManager::ss()->loadScene(core::SessionManager::scene());

    return app->exec();
}

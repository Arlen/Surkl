/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/db.hpp"

#include <QApplication>


using namespace core;

void db::init()
{
    const auto databaseName   = qApp->property(DB_NAME).toString();
    const auto connectionName = qApp->property(DB_CONNECTION_NAME).toString();

    if (databaseName.isEmpty()) {
        qWarning() << "DB name not set!";
    }

    if (connectionName.isEmpty()) {
        qWarning() << "DB connection name not set!";
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(databaseName);

    if (db.open()) {
        QSqlQuery q(db);
        q.exec(QLatin1String("PRAGMA synchronous = OFF;"));
        q.exec(QLatin1String("PRAGMA application_id = 314159265;"));
    } else {
        qWarning() << "database" << databaseName << "failed to open!";
    }
}

bool db::doesTableExists(QLatin1StringView name)
{
    QSqlQuery q(get());

    q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'")
        .arg(name));

    return q.next();
}

QSqlDatabase db::get()
{
    const auto connectionName = qApp->property(DB_CONNECTION_NAME).toString();

    return QSqlDatabase::database(connectionName);
}
#pragma once

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

namespace core::db
{
    constexpr auto DB_CONNECTION_NAME      = QLatin1StringView("Surkl_db_connection");
    constexpr auto DB_DATABASE_NAME        = QLatin1StringView("Surkl.db");
    constexpr auto DB_TEST_CONNECTION_NAME = QLatin1StringView("Surkl_test_db_connection");
    constexpr auto DB_TEST_DATABASE_NAME   = QLatin1StringView("Surkl_test.db");

    void init(QLatin1StringView dbName = DB_DATABASE_NAME, QLatin1StringView connectionName = DB_CONNECTION_NAME);
    bool doesTableExists(QLatin1StringView name);

    inline QSqlDatabase get(QLatin1StringView connectionName = DB_CONNECTION_NAME)
    {
        return QSqlDatabase::database(connectionName);
    }
}


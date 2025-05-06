#pragma once

#include <QSqlError>
#include <QSqlQuery>


namespace core::db
{
    struct DatabaseConfig
    {
        QLatin1StringView databaseName;
        QLatin1StringView connectionName;
    };

    constexpr auto DB_CONNECTION_NAME      = QLatin1StringView("Surkl_db_connection");
    constexpr auto DB_DATABASE_NAME        = QLatin1StringView("Surkl.db");
    constexpr auto DB_TEST_CONNECTION_NAME = QLatin1StringView("Surkl_test_db_connection");
    constexpr auto DB_TEST_DATABASE_NAME   = QLatin1StringView("Surkl_test.db");

    void init(const DatabaseConfig& config =
        { .databaseName   = DB_DATABASE_NAME
        , .connectionName = DB_CONNECTION_NAME
        });

    bool doesTableExists(QLatin1StringView name);

    inline QSqlDatabase get(QLatin1StringView connectionName = DB_CONNECTION_NAME)
    {
        return QSqlDatabase::database(connectionName);
    }
}


#pragma once

#include <QSqlError>
#include <QSqlQuery>


namespace core::db
{
    struct DatabaseConfig
    {
        const char* const databaseName;
        const char* const connectionName;
    };

    constexpr auto DB_NAME            = "SURKL_DB_NAME";
    constexpr auto DB_CONNECTION_NAME = "SURKL_DB_CONNECTION_NAME";

    constexpr auto DB_CONFIG = DatabaseConfig
        { .databaseName   = "Surkl.db"
        , .connectionName = "Surkl_db_connection"
        };
    constexpr auto DB_CONFIG_TEST = DatabaseConfig
        { .databaseName   = "Surkl_test.db"
        , .connectionName = "Surkl_test_db_connection"
        };


    void init();

    bool doesTableExists(QLatin1StringView name);

    QSqlDatabase get();
}
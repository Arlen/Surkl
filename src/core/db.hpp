#pragma once

#include <QSqlError>
#include <QSqlQuery>


namespace core::db
{
    using namespace Qt::Literals::StringLiterals;

    struct DatabaseConfig
    {
        QLatin1StringView databaseName;
        QLatin1StringView connectionName;
    };

    constexpr auto DB_CONFIG = DatabaseConfig
        { .databaseName   = "Surkl.db"_L1
        , .connectionName = "Surkl_db_connection"_L1
        };
    constexpr auto DB_CONFIG_TEST = DatabaseConfig
        { .databaseName   = "Surkl_test.db"_L1
        , .connectionName = "Surkl_test_db_connection"_L1
        };


    void init(const DatabaseConfig& config = DB_CONFIG);

    bool doesTableExists(QLatin1StringView name);

    inline QSqlDatabase get(QLatin1StringView connectionName = DB_CONFIG.connectionName)
    {
        return QSqlDatabase::database(connectionName);
    }
}
#pragma once

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>


namespace core::db
{
    constexpr auto DB_CONNECTION_NAME = QLatin1StringView("Ksed_db_connection");
    constexpr auto DB_DATABASE_NAME   = QLatin1StringView("Ksed.db");

    inline auto get()
    {
        class Database
        {
            QSqlDatabase _db;

        public:
            Database()
            {
                _db = QSqlDatabase::addDatabase("QSQLITE", DB_CONNECTION_NAME);
                _db.setDatabaseName(DB_DATABASE_NAME);

                if (_db.open()) {
                    QSqlQuery q(_db);
                    q.exec(QLatin1String("PRAGMA synchronous = OFF;"));
                    /// KSED in ASCII = 75836968
                    q.exec(QLatin1String("PRAGMA application_id = 75836968;"));
                } else {
                    qWarning() << "database" << DB_DATABASE_NAME << "failed to open!";
                }
            }

            ~Database()
            {
                QSqlDatabase::removeDatabase(DB_CONNECTION_NAME);
            }

            [[nodiscard]] QSqlDatabase sqlite() const { return _db; }
        };

        static auto* instance = new Database{};

        return instance->sqlite();
    }

    bool doesTableExists(QLatin1StringView name);
}
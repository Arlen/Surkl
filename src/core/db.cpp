#include "core/db.hpp"


using namespace core;

void db::init(const DatabaseConfig& config)
{
    auto db = QSqlDatabase::addDatabase("QSQLITE", config.connectionName);
    db.setDatabaseName(config.databaseName);

    if (db.open()) {
        QSqlQuery q(db);
        q.exec(QLatin1String("PRAGMA synchronous = OFF;"));
        q.exec(QLatin1String("PRAGMA application_id = 314159265;"));
    } else {
        qWarning() << "database" << config.databaseName << "failed to open!";
    }
}

bool db::doesTableExists(QLatin1StringView name)
{
    QSqlQuery q(get());

    q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'")
        .arg(name));

    return q.next();
}

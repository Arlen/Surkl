#include "core/db.hpp"


using namespace core;

void init(QLatin1StringView dbName, QLatin1StringView connectionName)
{
    auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbName);

    if (db.open()) {
        QSqlQuery q(db);
        q.exec(QLatin1String("PRAGMA synchronous = OFF;"));
        q.exec(QLatin1String("PRAGMA application_id = 314159265;"));
    } else {
        qWarning() << "database" << dbName << "failed to open!";
    }
}

bool db::doesTableExists(QLatin1StringView name)
{
    QSqlQuery q(get());

    q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'")
        .arg(name));

    return q.next();
}

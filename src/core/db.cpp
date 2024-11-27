#include "core/db.hpp"


using namespace core;

bool db::doesTableExists(QLatin1StringView name)
{
    QSqlQuery q(get());

    q.exec(QLatin1String("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'")
        .arg(name));

    return q.next();
}

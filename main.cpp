#include "src/core/SessionManager.hpp"
#include "SceneStorage.hpp"
#include "db.hpp"

#include <QApplication>
#include <QSqlDatabase>


int main(int argc, char* argv[])
{
    QScopedPointer app(new QApplication(argc, argv));

    if (!QSqlDatabase::drivers().contains("QSQLITE"))
    {
        qWarning() << "Error: QSQLITE database driver not available.";
        return -1;
    }

    app->setProperty(core::db::DB_NAME
        , core::db::DB_CONFIG.databaseName);
    app->setProperty(core::db::DB_CONNECTION_NAME
        , core::db::DB_CONFIG.connectionName);

    core::SessionManager::mw()->resize(640*2, 480*2);
    core::SessionManager::mw()->show();

    core::SessionManager::ss()->loadScene(core::SessionManager::scene());

    return app->exec();
}

#include "src/core/SessionManager.hpp"

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

    core::SessionManager::mw()->resize(640*2, 480*2);
    core::SessionManager::mw()->show();

    return app->exec();
}

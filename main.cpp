#include "src/core/SessionManager.hpp"

#include <QApplication>
#include <QSqlDatabase>


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    if (!QSqlDatabase::drivers().contains("QSQLITE"))
    {
        qWarning() << "Error: QSQLITE database driver not available.";
        return -1;
    }

    core::session()->start();
    core::session()->mw()->resize(640*2, 480*2);
    core::session()->mw()->show();

    return  a.exec();
}

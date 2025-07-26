/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "SceneStorage.hpp"
#include "AboutSurkl.hpp"
#include "SurklStyle.hpp"
#include "core/SessionManager.hpp"
#include "db.hpp"
#include "db/stmt.hpp"
#include "version.hpp"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlRecord>

#include <print>

namespace
{
    constexpr auto SHOW_ABOUT = "show_about"_L1;

    void showAboutDialog()
    {
        auto* dialog = new gui::AboutDialog();
        QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
        dialog->show();
    }

    QHash<QString, QVariant> getAttributes()
    {
        QHash<QString, QVariant> attrs;

        if (const auto db = core::db::get(); db.isOpen()) {
            QSqlQuery q(db);
            q.prepare(stmt::surkl::SELECT_ATTRIBUTE);

            if (q.exec()) {
                const auto rec = q.record();
                const auto keyIdx = rec.indexOf(stmt::surkl::ATTRIBUTE_KEY);
                const auto valIdx = rec.indexOf(stmt::surkl::ATTRIBUTE_VALUE);

                while (q.next()) {
                    attrs.insert(q.value(keyIdx).toString(), q.value(valIdx));
                }
            }
        }

        return attrs;
    }

    void saveAttribute(const QString& key, const QVariant &val)
    {
        if (const auto db = core::db::get(); db.isOpen()) {
            QSqlQuery q(db);
            q.prepare(stmt::surkl::INSERT_ATTRIBUTE);
            q.addBindValue(key);
            q.addBindValue(val);

            if (!q.exec()) {
                qWarning() << "Surkl: failed to save attribute" << q.lastError();
            }
        }
    }
}

int main(int argc, char* argv[])
{
    auto* app = new QApplication(argc, argv);

    QApplication::setApplicationName("surkl");
    QApplication::setApplicationDisplayName("Surkl");
    QApplication::setApplicationVersion(version());

    std::println("{} {},",
        QApplication::applicationDisplayName().toStdString(),
        QApplication::applicationVersion().toStdString());
    std::println("Copyright (C) 2025 Arlen Avakian");

    if (!QSqlDatabase::drivers().contains("QSQLITE"))
    {
        qWarning() << "Error: QSQLITE database driver not available.";
        return -1;
    }

    app->setProperty(core::db::DB_NAME
        , core::db::DB_CONFIG.databaseName);
    app->setProperty(core::db::DB_CONNECTION_NAME
        , core::db::DB_CONFIG.connectionName);

    app->setStyle(new gui::SurklStyle());

    core::SessionManager::mw()->show();
    core::SessionManager::ss()->loadScene(core::SessionManager::scene());

    if (const auto db = core::db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.exec(stmt::surkl::CREATE_SURKL_TABLE);
    }

    const auto attrs = getAttributes();
    if (const auto var = attrs.value(SHOW_ABOUT, QVariant(true)); var.canConvert<bool>() && var.value<bool>()) {
        showAboutDialog();
    }
    saveAttribute(SHOW_ABOUT, false);


    return app->exec();
}

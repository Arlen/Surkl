#include "gui/theme.hpp"
#include "core/db.hpp"

#include <QMetaProperty>


using namespace gui;
using namespace Qt::Literals::StringLiterals;

void ThemeManager::configure(ThemeManager* tm)
{
    if (auto db = core::db::get(); db.isOpen()) {
        if (core::db::doesTableExists(TABLE_NAME)) {

            /// setProperty() will trigger the NOTIFY signal of each property,
            /// which are connected to 'save##Color' slots, and those call
            /// save() which starts a new QSqlQuery, and that somehow corrupts
            /// the QSqlQuery object below, which then causes the while loop to
            /// run forever.
            QSignalBlocker blocker(tm);

            QSqlQuery q(db);
            q.prepare(QLatin1StringView("SELECT %1,%2 FROM %3")
                .arg(PROPERTY_NAME_COL)
                .arg(COLOR_VALUE_COL)
                .arg(TABLE_NAME));
            q.exec();
            const auto rec         = q.record();
            const auto name_index  = rec.indexOf(PROPERTY_NAME_COL);
            const auto value_index = rec.indexOf(COLOR_VALUE_COL);
            while (q.next())
            {
                const auto name = q.value(name_index).toString().toLatin1();

                if (!tm->property(name).isValid()) {
                    /// if what is in the DB does not exists in the class
                    /// definition, then skip.
                    continue;
                }

                const auto color = QColor::fromRgba(q.value(value_index).toInt());
                tm->setProperty(name, color);
            }
        } else {
            db.transaction();
            QSqlQuery q(db);
            q.prepare(
                QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                                    ( %2 TEXT NOT NULL PRIMARY KEY
                                    , %3 INTEGER NOT NULL
                                    , UNIQUE(%2)))")
                .arg(TABLE_NAME)
                .arg(PROPERTY_NAME_COL)
                .arg(COLOR_VALUE_COL));
            q.exec();

            const auto* metaObj = tm->metaObject();
            for (auto i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
                const auto prop  = metaObj->property(i);
                const auto color = prop.read(tm).value<QColor>();
                saveToDatabase(QLatin1StringView(prop.name()), color);
            }

            if (!db.commit()) {
                qWarning()
                    << "ThemeManager::configure: Failed to commit changes ("
                    << db.lastError()
                    << ")";
            }
        }
    }
}

void ThemeManager::saveToDatabase(ThemeManager* tm)
{
    if (auto db = core::db::get(); db.isOpen()) {
        db.transaction();
        const auto* metaObj = tm->metaObject();
        for (auto i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
            const auto prop  = metaObj->property(i);
            const auto color = prop.read(tm).value<QColor>();
            saveToDatabase(QLatin1StringView(prop.name()), color);
        }
        if (!db.commit()) {
            qWarning()
                << "ThemeManager::save: Failed to commit changes ("
                << db.lastError()
                << ")";
        }
    }
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    reset();

    connect(this, &ThemeManager::bgColorChanged, this, &ThemeManager::saveBgColor);
}

void ThemeManager::setBgColor(const QColor& color) noexcept
{
    _bg_color = color;

    emit bgColorChanged(_bg_color);
}

void ThemeManager::reset() noexcept
{
    {
        QSignalBlocker blocker(this);
        _bg_color = builtin::bgColor();
    }

    emit resetTriggered();
}

void ThemeManager::saveToDatabase(QLatin1StringView name, const QColor& color)
{
    if (auto db = core::db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2, %3) VALUES(?, ?)")
            .arg(TABLE_NAME)
            .arg(PROPERTY_NAME_COL)
            .arg(COLOR_VALUE_COL));
        q.addBindValue(name);
        q.addBindValue(color.rgba());
        q.exec();
    }
}

void ThemeManager::saveBgColor() const
{
    saveToDatabase("bgColor"_L1, _bg_color);
}

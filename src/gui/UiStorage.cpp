/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "UiStorage.hpp"
#include "Splitter.hpp"
#include "core/db.hpp"
#include "theme/ThemeArea.hpp"
#include "view/GraphicsView.hpp"
#include "view/ViewArea.hpp"
#include "window/AbstractWindowArea.hpp"
#include "window/Window.hpp"
#include "MainWindow.hpp"

#include <QSqlRecord>



using namespace gui;
using namespace core;

namespace
{
    window::AbstractWindowArea::AreaType getAreaType(const window::Window* win)
    {
        Q_ASSERT(win);

        auto result = window::AbstractWindowArea::AreaType::InvalidArea;

        if (qobject_cast<view::ViewArea*>(win->areaWidget())) {
            result = window::AbstractWindowArea::AreaType::ViewArea;
        } else if (qobject_cast<theme::ThemeArea*>(win->areaWidget())) {
            result = window::AbstractWindowArea::AreaType::ThemeArea;
        }

        return result;
    }

    qint32 getWindowSize(const window::Window* win)
    {
        Q_ASSERT(win);

        if (auto* splitter = qobject_cast<Splitter*>(win->parentWidget())) {
            if (splitter->orientation() == Qt::Horizontal) {
                return win->width();
            }
            return win->height();
        }

        return -1;
    }
}

UiStorage::UiStorage(QObject* parent)
    : QObject(parent)
{
    _saveViewConnection = connect
        ( this
        , qOverload<const view::GraphicsView*>(&UiStorage::stateChanged)
        , this
        , &saveView);

    _saveWindowConnection = connect
        ( this
        , qOverload<const window::Window*>(&UiStorage::stateChanged)
        , this
        , &saveWindow);

    _saveSplitterConnection = connect
        ( this
        , qOverload<const Splitter*>(&UiStorage::stateChanged)
        , this
        , &saveSplitter);

    _saveMainWindowConnection = connect
        ( this
        , qOverload<const MainWindow*>(&UiStorage::stateChanged)
        , this
        , &saveMainWindow);
}

void UiStorage::configure()
{
    using namespace core::db;

    createTable();

    Q_ASSERT(doesTableExists(storage::MAIN_WINDOWS_TABLE));
    Q_ASSERT(doesTableExists(storage::SPLITTERS_TABLE));
    Q_ASSERT(doesTableExists(storage::WINDOWS_TABLE));
    Q_ASSERT(doesTableExists(storage::GRAPHICS_VIEWS_TABLE));
}

void UiStorage::createTable()
{
    using namespace core;
    using namespace gui::storage;

    if (const auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);

        bool ok = q.exec(
            QLatin1String(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 INTEGER
                            , %3 INTEGER
                            , %4 INTEGER
                            , %5 INTEGER
                            , UNIQUE(%2)
                            , UNIQUE(%5)) )")
            .arg(MAIN_WINDOWS_TABLE)
            .arg(MAIN_WINDOW_ID)
            .arg(MAIN_WINDOW_WIDTH)
            .arg(MAIN_WINDOW_HEIGHT)
            .arg(MAIN_WINDOW_ROOT_SPLITTER));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1String(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 INTEGER PRIMARY KEY
                            , %3 INTEGER
                            , %4 INTEGER) )")
            .arg(SPLITTERS_TABLE)
            .arg(SPLITTER_ID)
            .arg(SPLITTER_SIZE)
            .arg(SPLITTER_ORIENTATION));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1String(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 INTEGER PRIMARY KEY
                            , %3 INTEGER) )")
            .arg(WIDGET_INDICES_TABLE)
            .arg(WIDGET_ID)
            .arg(WIDGET_INDEX));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1String(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 INTEGER PRIMARY KEY
                            , %3 INTEGER) )")
            .arg(SPLITTER_WIDGETS_TABLE)
            .arg(WIDGET_ID)
            .arg(SPLITTER_ID));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1String(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 INTEGER PRIMARY KEY
                            , %3 INTEGER
                            , %4 INTEGER) )")
            .arg(WINDOWS_TABLE)
            .arg(WINDOW_ID)
            .arg(WINDOW_SIZE)
            .arg(WINDOW_TYPE));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1String(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 INTEGER PRIMARY KEY
                            , %3 INTEGER
                            , %4 INTEGER) )")
            .arg(GRAPHICS_VIEWS_TABLE)
            .arg(GRAPHICS_VIEW_PARENT)
            .arg(GRAPHICS_VIEW_CENTER_X)
            .arg(GRAPHICS_VIEW_CENTER_Y));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }
    }
}

storage::UiState UiStorage::load()
{
    storage::UiState state;

    readTable(state);

    return state;
}


void UiStorage::readTable(storage::UiState& state)
{
    using namespace gui::storage;

    const auto db = core::db::get();

    if (!db.isOpen()) { return; }
    bool ok;

    QSqlQuery q(db);

    q.prepare(QLatin1String("SELECT * FROM %1").arg(GRAPHICS_VIEWS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto parentIdx = rec.indexOf(GRAPHICS_VIEW_PARENT);
        const auto cxIdx     = rec.indexOf(GRAPHICS_VIEW_CENTER_X);
        const auto cyIdx     = rec.indexOf(GRAPHICS_VIEW_CENTER_Y);

        while (q.next()) {
            const auto parent = q.value(parentIdx).toInt(&ok); Q_ASSERT(ok);
            const auto cx = q.value(cxIdx).toInt(&ok); Q_ASSERT(ok);
            const auto cy = q.value(cyIdx).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!state.views.contains(parent));
            state.views[parent] = QPoint(cx, cy);
        }
    }

    q.prepare(QLatin1String("SELECT * FROM %1").arg(WINDOWS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto windowIdIdx = rec.indexOf(WINDOW_ID);
        const auto sizeIdx     = rec.indexOf(WINDOW_SIZE);
        const auto typeIdx     = rec.indexOf(WINDOW_TYPE);

        while (q.next()) {
            const auto id   = q.value(windowIdIdx).toInt(&ok); Q_ASSERT(ok);
            const auto size = q.value(sizeIdx).toInt(&ok); Q_ASSERT(ok);
            const auto type = q.value(typeIdx).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!state.windows.contains(id));
            state.windows[id] = storage::Window{size, static_cast<window::AbstractWindowArea::AreaType>(type)};
        }
    }

    q.prepare(QLatin1String("SELECT * FROM %1").arg(SPLITTERS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto splitterIdIdx   = rec.indexOf(SPLITTER_ID);
        const auto sizeIdx = rec.indexOf(SPLITTER_SIZE);
        const auto oriIdx  = rec.indexOf(SPLITTER_ORIENTATION);

        while (q.next()) {
            const auto id   = q.value(splitterIdIdx).toInt(&ok); Q_ASSERT(ok);
            const auto size = q.value(sizeIdx).toInt(&ok); Q_ASSERT(ok);
            const auto ori  = q.value(oriIdx).toInt(&ok); Q_ASSERT(ok);
            const auto orientation = static_cast<Qt::Orientation>(ori);
            Q_ASSERT(!state.splitters.contains(id));
            Q_ASSERT(orientation == Qt::Horizontal || orientation == Qt::Vertical);
            state.splitters[id] = storage::Splitter{size, orientation};
        }
    }

    q.prepare(QLatin1String("SELECT * FROM %1").arg(MAIN_WINDOWS_TABLE));

    if (q.exec()) {
        const auto rec         = q.record();
        const auto idIdx       = rec.indexOf(MAIN_WINDOW_ID);
        const auto wIdx        = rec.indexOf(MAIN_WINDOW_WIDTH);
        const auto hIdx        = rec.indexOf(MAIN_WINDOW_HEIGHT);
        const auto splitterIdx = rec.indexOf(MAIN_WINDOW_ROOT_SPLITTER);

        while (q.next()) {
            const auto id       = q.value(idIdx).toInt(&ok); Q_ASSERT(ok);
            const auto w        = q.value(wIdx).toInt(&ok); Q_ASSERT(ok);
            const auto h        = q.value(hIdx).toInt(&ok); Q_ASSERT(ok);
            const auto splitter = q.value(splitterIdx).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!state.mws.contains(id));
            state.mws[id] = {QSize(w, h), splitter};
        }
    }

    q.prepare(QLatin1String("SELECT * FROM %1").arg(WIDGET_INDICES_TABLE));
    std::unordered_map<storage::WidgetId, qint32> widgetIndices;

    if (q.exec()) {
        const auto rec = q.record();
        const auto widgetIdIdx    = rec.indexOf(WIDGET_ID);
        const auto widgetIndexIdx = rec.indexOf(WIDGET_INDEX);

        while (q.next()) {
            const auto id    = q.value(widgetIdIdx).toInt(&ok); Q_ASSERT(ok);
            const auto index = q.value(widgetIndexIdx).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!widgetIndices.contains(id));
            widgetIndices[id] = index;
        }
    }

    q.prepare(QLatin1String("SELECT * FROM %1").arg(SPLITTER_WIDGETS_TABLE));
    std::unordered_map<storage::WidgetId, storage::SplitterId> splitterWidgets;

    if (q.exec()) {
        const auto rec = q.record();
        const auto widgetIdIdx   = rec.indexOf(WIDGET_ID);
        const auto splitterIdIdx = rec.indexOf(SPLITTER_ID);

        while (q.next()) {
            const auto widgetId   = q.value(widgetIdIdx).toInt(&ok); Q_ASSERT(ok);
            const auto splitterId = q.value(splitterIdIdx).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(widgetIndices.contains(widgetId));
            Q_ASSERT(state.splitters.contains(splitterId));
            const auto index = widgetIndices[widgetId];
            Q_ASSERT(!state.splitters[splitterId].widgets.contains(index));
            state.splitters[splitterId].widgets.insert({index, widgetId});
        }
    }
}

void UiStorage::saveView(const view::GraphicsView* gv)
{
    Q_ASSERT(gv);

    if (const auto* va = qobject_cast<view::ViewArea*>(gv->parentWidget())) {
        if (const auto* window = qobject_cast<window::Window*>(va->parentWidget())) {
            if (auto db = db::get(); db.isOpen()) {
                QSqlQuery q(db);

                const auto id = window->widgetId();
                const auto center = gv->mapToScene(gv->rect().center());

                if (!q.exec(QLatin1String("INSERT OR REPLACE INTO %1 VALUES (%2, %3, %4)")
                    .arg(storage::GRAPHICS_VIEWS_TABLE)
                    .arg(id)
                    .arg(center.x())
                    .arg(center.y()))) {

                    qWarning() << db.lastError();
                }
            }
        }
    }
}

void UiStorage::saveWindow(const window::Window* win)
{
    Q_ASSERT(win);

    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);

        const auto id   = win->widgetId();
        const auto size = getWindowSize(win);
        const auto type = getAreaType(win);

        if (!q.exec(QLatin1String("INSERT OR REPLACE INTO %1 VALUES (%2, %3, %4)")
            .arg(storage::WINDOWS_TABLE)
            .arg(id)
            .arg(size)
            .arg(type))) {
            qWarning() << db.lastError();
        }
    }
}

void UiStorage::saveSplitter(const Splitter* splitter)
{
    Q_ASSERT(splitter);

    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery q(db);

        const auto splitterId  = splitter->widgetId();
        const auto splitterOri = splitter->orientation();

        if (!q.exec(QLatin1String("INSERT OR REPLACE INTO %1 VALUES (%2, %3, %4)")
            .arg(storage::SPLITTERS_TABLE)
            .arg(splitterId)
            .arg(splitterOri == Qt::Horizontal ? splitter->height() : splitter->width())
            .arg(splitterOri))) {
            qWarning() << db.lastError();
        }

       q.prepare(QLatin1String(R"(INSERT OR REPLACE INTO %1 (%2, %3) VALUES(?, ?))")
           .arg(storage::WIDGET_INDICES_TABLE)
           .arg(storage::WIDGET_ID)
           .arg(storage::WIDGET_INDEX));

        QList<qint32> widgetIds;
        for (int i = 0; i < splitter->count(); ++i) {
            auto* widget = splitter->widget(i);
            qint32 widgetId = -1;
            if (const auto* win = qobject_cast<window::Window*>(widget)) {
                widgetId = win->widgetId();
            } else if (const auto* sp = qobject_cast<Splitter*>(widget)) {
                widgetId = sp->widgetId();
            }
            Q_ASSERT(widgetId != -1);
            widgetIds.push_back(widgetId);
            q.addBindValue(widgetId);
            q.addBindValue(i);
            if (!q.exec()) {
                qWarning() << db.lastError();
            }
        }

        q.prepare(QLatin1String(R"(INSERT OR REPLACE INTO %1 (%2, %3) VALUES(?, ?))")
            .arg(storage::SPLITTER_WIDGETS_TABLE)
            .arg(storage::WIDGET_ID)
            .arg(storage::SPLITTER_ID));

        for (const auto widgetId : widgetIds) {
            q.addBindValue(widgetId);
            q.addBindValue(splitterId);
            if (!q.exec()) {
                qWarning() << db.lastError();
            }
        }

        db.commit();
    }
}

void UiStorage::saveMainWindow(const MainWindow* mw)
{
    Q_ASSERT(mw);

    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);

        const auto id   = mw->widgetId();
        const auto size = mw->size();
        const auto root = mw->splitter()->widgetId();

        if (!q.exec(QString("INSERT OR REPLACE INTO %1 VALUES (%2, %3, %4, %5)")
            .arg(storage::MAIN_WINDOWS_TABLE)
            .arg(id)
            .arg(size.width())
            .arg(size.height())
            .arg(root))) {
            qWarning() << db.lastError();
        }
    }
}

void UiStorage::deleteView(qint32 parentId)
{
    deleteView(QList<qint32>() << parentId);
}

void UiStorage::deleteView(const QList<qint32>& ids)
{
    deleteFrom(storage::GRAPHICS_VIEWS_TABLE, storage::GRAPHICS_VIEW_PARENT, ids);
}

void UiStorage::deleteWindow(qint32 id)
{
    deleteWindow(QList<qint32>() << id);
}

void UiStorage::deleteWindow(const QList<qint32>& ids)
{
    deleteFrom(storage::WINDOWS_TABLE, storage::WINDOW_ID, ids);

    deleteFrom(storage::WIDGET_INDICES_TABLE, storage::WIDGET_ID, ids);
    deleteFrom(storage::SPLITTER_WIDGETS_TABLE, storage::WIDGET_ID, ids);
}

void UiStorage::deleteSplitter(qint32 id)
{
    deleteSplitter(QList<qint32>() << id);
}

void UiStorage::deleteSplitter(const QList<qint32>& ids)
{
    deleteFrom(storage::SPLITTERS_TABLE, storage::SPLITTER_ID, ids);

    QList<qint32> widgetIds;
    widgetIds << ids;

    if (auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);

        q.prepare(QString("SELECT %3 FROM %1 WHERE %2=:id")
            .arg(storage::SPLITTER_WIDGETS_TABLE)
            .arg(storage::SPLITTER_ID)
            .arg(storage::WIDGET_ID));

        bool ok;
        for (auto id : ids) {
            q.bindValue(":id", id);
            if (q.exec()) {
                auto widgetIdIndex = q.record().indexOf(storage::WIDGET_ID);

                while (q.next()) {
                    auto widgetId = q.value(widgetIdIndex).toInt(&ok); Q_ASSERT(ok);
                    widgetIds.push_back(widgetId);
                }
            }
        }
    }

    deleteFrom(storage::WIDGET_INDICES_TABLE, storage::WIDGET_ID, widgetIds);
    deleteFrom(storage::SPLITTER_WIDGETS_TABLE, storage::WIDGET_ID, widgetIds);
}

void UiStorage::deleteMainWindow(qint32 id)
{
    deleteMainWindow(QList<qint32>() << id);
}

void UiStorage::deleteMainWindow(const QList<qint32>& ids)
{
    deleteFrom(storage::MAIN_WINDOWS_TABLE, storage::MAIN_WINDOW_ID, ids);
}

void UiStorage::deleteFrom(const QLatin1String& table, const QLatin1String& key, const QList<qint32>& values)
{
    if (auto db = db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery q(db);
        q.prepare(QLatin1String("DELETE FROM %1 WHERE %2=:value")
            .arg(table)
            .arg(key));

        for (auto value : values) {
            q.bindValue(":value", value);
            if (!q.exec()) {
                qWarning() << db.lastError();
            }
        }
        if (!db.commit()) {
            qWarning() << db.lastError();
        }
    }
}
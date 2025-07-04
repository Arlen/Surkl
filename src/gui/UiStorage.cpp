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

}

void UiStorage::configure()
{
    createTable();

    Q_ASSERT(core::db::doesTableExists(storage::MAIN_WINDOWS_TABLE));
    Q_ASSERT(core::db::doesTableExists(storage::MAIN_WINDOWS_TABLE));
    Q_ASSERT(core::db::doesTableExists(storage::MAIN_WINDOWS_TABLE));
    Q_ASSERT(core::db::doesTableExists(storage::MAIN_WINDOWS_TABLE));

}

void UiStorage::createTable()
{
    using namespace core;
    using namespace gui::storage;

    if (const auto db = db::get(); db.isOpen()) {
        QSqlQuery q(db);
        bool ok;

        ok = q.exec(
            QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                                ( %2 INTEGER
                                , %3 INTEGER
                                , UNIQUE(%2)
                                , UNIQUE(%3)) )")
            .arg(MAIN_WINDOWS_TABLE)
            .arg(MAIN_WINDOW_ID)
            .arg(MAIN_WINDOW_ROOT_SPLITTER));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                                ( %2 INTEGER PRIMARY KEY
                                , %3 INTEGER) )")
            .arg(SPLITTERS_TABLE)
            .arg(SPLITTER_ID)
            .arg(SPLITTER_ORIENTATION));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                                ( %2 INTEGER PRIMARY KEY
                                , %3 INTEGER
                                , %4 INTEGER) )")
            .arg(WINDOWS_TABLE)
            .arg(WINDOW_ID)
            .arg(WINDOW_SIZE)
            .arg(WINDOW_TYPE));

        if (!ok) { qWarning() << q.lastError() << q.executedQuery(); }

        ok = q.exec(
            QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
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

void UiStorage::readTable()
{
    using namespace gui::storage;

    const auto db = core::db::get();

    if (!db.isOpen()) {
        return; // {};
    }
    bool ok;

    QSqlQuery q(db);
    Views views;
    Windows windows;
    Splitters splitters;
    MainWindows mainWindows;

    q.prepare(QLatin1StringView("SELECT * FROM %1").arg(GRAPHICS_VIEWS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto parentIndex = rec.indexOf(GRAPHICS_VIEW_PARENT);
        const auto cxIndex = rec.indexOf(GRAPHICS_VIEW_CENTER_X);
        const auto cyIndex = rec.indexOf(GRAPHICS_VIEW_CENTER_Y);

        while (q.next()) {
            const auto parent = q.value(parentIndex).toInt(&ok); Q_ASSERT(ok);
            const auto cx = q.value(cxIndex).toInt(&ok); Q_ASSERT(ok);
            const auto cy = q.value(cyIndex).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!views.contains(parent));
            views[parent] = QPoint(cx, cy);
        }
    }

    q.prepare(QLatin1StringView("SELECT * FROM %1").arg(WINDOWS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto idIndex   = rec.indexOf(WINDOW_ID);
        const auto sizeIndex = rec.indexOf(WINDOW_SIZE);
        const auto typeIndex = rec.indexOf(WINDOW_TYPE);

        while (q.next()) {
            const auto id   = q.value(idIndex).toInt(&ok); Q_ASSERT(ok);
            const auto size = q.value(sizeIndex).toInt(&ok); Q_ASSERT(ok);
            const auto type = q.value(typeIndex).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!windows.contains(id));
            windows[id] = storage::Window{size, static_cast<window::AbstractWindowArea::AreaType>(type)};
        }
    }

    q.prepare(QLatin1StringView("SELECT * FROM %1").arg(SPLITTERS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto idIndex  = rec.indexOf(SPLITTER_ID);
        const auto oriIndex = rec.indexOf(SPLITTER_ORIENTATION);

        while (q.next()) {
            const auto id  = q.value(idIndex).toInt(&ok); Q_ASSERT(ok);
            const auto ori = q.value(oriIndex).toInt(&ok); Q_ASSERT(ok);
            const auto orientation = static_cast<Qt::Orientation>(ori);
            Q_ASSERT(!splitters.contains(id));
            Q_ASSERT(orientation == Qt::Horizontal || orientation == Qt::Vertical);
            splitters[id] = storage::Splitter{orientation};
        }
    }

    q.prepare(QLatin1StringView("SELECT * FROM %1").arg(MAIN_WINDOWS_TABLE));

    if (q.exec()) {
        const auto rec = q.record();
        const auto idIndex           = rec.indexOf(MAIN_WINDOW_ID);
        const auto rootSplitterIndex = rec.indexOf(MAIN_WINDOW_ROOT_SPLITTER);

        while (q.next()) {
            const auto id           = q.value(idIndex).toInt(&ok); Q_ASSERT(ok);
            const auto rootSplitter = q.value(rootSplitterIndex).toInt(&ok); Q_ASSERT(ok);
            Q_ASSERT(!mainWindows.contains(id));
            mainWindows[id] = rootSplitter;
        }
    }

    for (const auto& sp : splitters) {
        const auto table = QString(SPLITTER_WIDGETS_TABLES).arg(sp.first);
         q.prepare(QLatin1StringView("SELECT * FROM %1").arg(table));

        ///

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

                if (!q.exec(QString("INSERT OR REPLACE INTO %1 VALUES (%2, %3, %4)")
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

        if (!q.exec(QString("INSERT OR REPLACE INTO %1 VALUES (%2, %3, %4)")
            .arg(storage::WINDOWS_TABLE)
            .arg(id)
            .arg(size)
            .arg(type))) {
            qWarning() << db.lastError();
        }
    }
}

void UiStorage::deleteView(const QWidget* widget)
{
    Q_ASSERT(widget);

    const view::GraphicsView* gv = qobject_cast<const view::GraphicsView*>(widget);

    if (gv == nullptr) {
        return;
    }


    if (const auto* va = qobject_cast<view::ViewArea*>(gv->parentWidget())) {
        if (const auto* window = qobject_cast<window::Window*>(va->parentWidget())) {
            if (auto db = db::get(); db.isOpen()) {
                QSqlQuery q(db);

                const auto id = window->widgetId();

                if (!q.exec(QString("DELETE FROM %1 WHERE %2=%3")
                    .arg(storage::GRAPHICS_VIEWS_TABLE)
                    .arg(storage::GRAPHICS_VIEW_PARENT)
                    .arg(id))) {
                        qWarning() << db.lastError();
                    }
            }
        }
    }
}
/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "gui/window/AbstractWindowArea.hpp"

#include <QObject>
#include <QPoint>

#include <unordered_map>
#include <unordered_set>


namespace gui::storage
{
    constexpr auto MAIN_WINDOWS_TABLE        = QLatin1String("MainWindows");
    constexpr auto MAIN_WINDOW_ID            = QLatin1String("mw_id");
    constexpr auto MAIN_WINDOW_ROOT_SPLITTER = QLatin1String("root_splitter");

    constexpr auto SPLITTERS_TABLE      = QLatin1String("Splitters");
    constexpr auto SPLITTER_ID          = QLatin1String("splitter_id");
    constexpr auto SPLITTER_SIZE        = QLatin1String("size");
    constexpr auto SPLITTER_ORIENTATION = QLatin1String("orientation");

    /// a widget is either a Window or a Splitter.
    /// a widget belongs to a Splitter, and has an index.
    constexpr auto WIDGETS_TABLE = QLatin1String("Widgets");
    constexpr auto WIDGET_ID     = QLatin1String("widget_id");
    constexpr auto WIDGET_INDEX  = QLatin1String("widget_index");

    /// a table to keep track of which Splitter a widget belongs to.
    /// (widget_id, splitter_id)
    constexpr auto SPLITTER_WIDGETS_TABLE = QLatin1StringView("SplitterWidgets");

    constexpr auto WINDOWS_TABLE = QLatin1String("Windows");
    constexpr auto WINDOW_ID     = QLatin1String("window_id");
    constexpr auto WINDOW_SIZE   = QLatin1String("size");
    constexpr auto WINDOW_TYPE   = QLatin1String("type");

    constexpr auto GRAPHICS_VIEWS_TABLE   = QLatin1String("GraphicsViews");
    // the parent Window which contains the ViewArea that contains a GraphicsView widget.
    constexpr auto GRAPHICS_VIEW_PARENT   = QLatin1String("parent_id");
    // the scene pos of the center of GraphicsView widget.
    constexpr auto GRAPHICS_VIEW_CENTER_X = QLatin1String("center_x");
    constexpr auto GRAPHICS_VIEW_CENTER_Y = QLatin1String("center_y");

    using WindowId = qint32;

    struct View
    {
        //qint32 parent;
        QPoint center;
    };
    using Views = std::unordered_map<WindowId, QPoint>;

    struct Window
    {
        //qint32 id;
        qint32 size;
        //qint32 type;
        window::AbstractWindowArea::AreaType type;
    };
    using Windows = std::unordered_map<WindowId, Window>;

    struct Splitter
    {
        //qint32 id;
        qint32 size;
        qint32 orientation;
        std::unordered_set<qint32> widgets; // IDs of Windows or Splitters.
    };

    using Splitters = std::unordered_map<qint32, Splitter>;
    using MainWindows = std::unordered_map<qint32, qint32>;
}


namespace gui
{
    class MainWindow;
    class Splitter;

    namespace view
    {
        class GraphicsView;
    }

    class UiStorage final : public QObject
    {
    public:
        explicit UiStorage(QObject* parent = nullptr);

        static void configure();

        static void saveView(const view::GraphicsView* gv);

        static void saveWindow(const window::Window* win);

        static void saveSplitter(const Splitter* splitter);

        static void saveMainWindow(const MainWindow* mw);

        void deleteView(qint32 parentId);

        void deleteView(const QList<qint32>& ids);

        void deleteWindow(qint32 id);

        void deleteWindow(const QList<qint32>& ids);

        void deleteSplitter(qint32 id);

        void deleteSplitter(const QList<qint32>& ids);

        void deleteMainWindow(qint32 id);

        void deleteMainWindow(const QList<qint32>& ids);

    private:
        void deleteFrom(const QLatin1String& table, const QLatin1String& key, const QList<qint32>& values);

        static void createTable();

        void readTable();
    };
}
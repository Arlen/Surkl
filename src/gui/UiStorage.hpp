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
    constexpr auto MAIN_WINDOW_ID            = QLatin1String("id");
    constexpr auto MAIN_WINDOW_ROOT_SPLITTER = QLatin1String("root_splitter");

    constexpr auto SPLITTERS_TABLE      = QLatin1String("Splitters");
    constexpr auto SPLITTER_ID          = QLatin1String("id");
    //constexpr auto SPLITTER_INDEX       = QLatin1String("index");
    //constexpr auto SPLITTER_PARENT      = QLatin1String("parent");
    constexpr auto SPLITTER_ORIENTATION = QLatin1String("orientation");

    constexpr auto SPLITTER_WIDGETS_TABLES = QLatin1String("Splitter%1Widgets");
    constexpr auto SPLITTER_WIDGET_INDEX   = QLatin1String("widget_index");
    constexpr auto SPLITTER_WIDGET_ID      = QLatin1String("widget_id");


    constexpr auto WINDOWS_TABLE = QLatin1String("Windows");
    constexpr auto WINDOW_ID     = QLatin1String("id");
    //constexpr auto WINDOW_INDEX  = QLatin1String("index");
    //constexpr auto WINDOW_PARENT = QLatin1String("parent");
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
        qint32 orientation;
        std::unordered_set<qint32> widgets; // IDs of Windows or Splitters.
    };

    using Splitters = std::unordered_map<qint32, Splitter>;
    using MainWindows = std::unordered_map<qint32, qint32>;
}


namespace gui
{
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

        void deleteView(const QWidget* widget);

    private:
        static void createTable();

        void readTable();
    };
}
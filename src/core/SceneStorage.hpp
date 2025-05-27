/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>


class QGraphicsScene;

namespace core
{
    class NodeItem;

    constexpr auto NODES_TABLE = QLatin1String("Nodes");
    constexpr auto NODE_ID     = QLatin1String("node_id");
    constexpr auto PARENT_ID   = QLatin1String("parent_id");
    constexpr auto NODE_STATE  = QLatin1String("node_state");
    constexpr auto NODE_XPOS   = QLatin1String("node_xpos");
    constexpr auto NODE_YPOS   = QLatin1String("node_ypos");
    constexpr auto EDGE_LEN    = QLatin1String("edge_len");


    class SceneStorage final : public QObject
    {
    public:
        explicit SceneStorage(QObject* parent = nullptr);

        static void configure(SceneStorage* ps);

        void saveNode(const NodeItem* node);

        static void saveScene(const QGraphicsScene* scene);

    private:
        static void createTables();
    };
}
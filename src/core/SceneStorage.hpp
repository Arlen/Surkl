/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>


class QGraphicsScene;
class QTimer;

namespace core
{
    class NodeItem;
    class FileSystemScene;

    constexpr auto NODES_TABLE = QLatin1String("Nodes");
    constexpr auto NODE_ID     = QLatin1String("node_id");
    constexpr auto NODE_TYPE   = QLatin1String("node_type");
    constexpr auto NODE_XPOS   = QLatin1String("node_xpos");
    constexpr auto NODE_YPOS   = QLatin1String("node_ypos");
    constexpr auto EDGE_LEN    = QLatin1String("edge_len");


    class NodeData;

    class SceneStorage final : public QObject
    {
        Q_OBJECT

    public:
        explicit SceneStorage(QObject* parent = nullptr);

        static void configure();

        void saveNode(const NodeItem* node);

        void saveNodes(const QList<const NodeItem*>& nodes) const;

        void saveScene() const;

        void loadScene(FileSystemScene* scene);

    public slots:
        void push();

    private:
        void enableSave();

        static void createTable();

        static QHash<QPersistentModelIndex, QList<NodeData>> readTable(const FileSystemScene* scene);

        FileSystemScene* _scene{nullptr};
        QTimer* _timer{nullptr};
        QList<const NodeItem*> _toBeSaved;
    };
}
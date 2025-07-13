/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPointF>


class QGraphicsScene;
class QTimer;

namespace core
{
    class NodeItem;
    class FileSystemScene;
    class NodeData;

    struct SaveData
    {
        QString id;
        int nodeType;
        int firstRow;
        QPointF pos;
        qreal length;
    };

    struct DeleteData
    {
        QString id;
    };

    class SceneStorage final : public QObject
    {
        Q_OBJECT

    public:
        explicit SceneStorage(QObject* parent = nullptr);

        static void configure();

        void deleteNode(const NodeItem* node);

        void saveNode(const NodeItem* node);

        void saveScene() const;

        void loadScene(FileSystemScene* scene);

    private slots:
        void nextN();

    private:
        void enableStorage();

        void saveNodes(const QList<const NodeItem*>& nodes) const;

        static void consume(const QList<QVariant>& data);

        static void createTable();

        static QHash<QPersistentModelIndex, QList<NodeData>> readTable(const FileSystemScene* scene);

        bool _enabled{false};
        FileSystemScene* _scene{nullptr};
        QTimer* _timer{nullptr};

        QList<QVariant> _queue;
    };
}
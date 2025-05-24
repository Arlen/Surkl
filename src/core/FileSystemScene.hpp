/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QGraphicsScene>


class QFileSystemModel;
class QSortFilterProxyModel;

namespace core
{
    class NodeItem;

    namespace nodes
    {
        class SceneBookmarkItem;
    }

    class FileSystemScene final : public QGraphicsScene
    {
        Q_OBJECT

#ifdef TEST_ANIMATIONS
    signals:
        void sequenceFinished();
#endif

    public:
        static void configure(FileSystemScene* scene);

        explicit FileSystemScene(QObject* parent = nullptr);
        ~FileSystemScene() override;
        void addSceneBookmark(const QPoint& pos, const QString& name);
        [[nodiscard]] QPersistentModelIndex rootIndex() const;
        bool isDir(const QModelIndex& index) const;
        void setRootPath(const QString& newPath) const;

    public slots:
        void openSelectedNodes() const;
        void closeSelectedNodes() const;
        void halfCloseSelectedNodes() const;
        void refreshItems();

    protected:
        void drawBackground(QPainter* p, const QRectF& rec) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override;

    private slots:
        void onSelectionChange();
        void onRowsInserted(const QModelIndex& parent, int start, int end) const;
        void onRowsRemoved(const QModelIndex& parent, int start, int end) const;

    private:
        bool openFile(const NodeItem* node) const;

        QFileSystemModel* _model{nullptr};
        QSortFilterProxyModel* _proxyModel{nullptr};
    };
}

/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "NodeItem.hpp"

#include <QGraphicsScene>


class QFileSystemModel;
class QSortFilterProxyModel;

namespace core
{
    class NodeItem;
    class SceneBookmarkItem;


    class FileSystemScene final : public QGraphicsScene
    {
        Q_OBJECT

#ifdef TEST_ANIMATIONS
    signals:
        void sequenceFinished();
#endif

    public:
        explicit FileSystemScene(QObject* parent = nullptr);
        [[nodiscard]] QPersistentModelIndex rootIndex() const;
        bool isDir(const QModelIndex& index) const;
        bool isLink(const QModelIndex& index) const;
        [[nodiscard]] QString filePath(const QPersistentModelIndex& index) const;
        [[nodiscard]] QPersistentModelIndex index(const QString& paht) const;

        void setRootPath(const QString& newPath) const;
        bool openFile(const NodeItem* node) const;
        void openTo(const QString &targetPath) const;
        void fetchMore(const QPersistentModelIndex& index) const;
        qint64 fileSize(const QPersistentModelIndex& index) const;

    public slots:
        void openSelectedNodes() const;
        void closeSelectedNodes() const;
        void halfCloseSelectedNodes() const;
        void addSceneBookmark(const QPoint& clickPos, const QString& name);

    protected:
        void drawBackground(QPainter* p, const QRectF& rec) override;
        void keyPressEvent(QKeyEvent *event) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    private slots:
        void onSelectionChange();
        void onRowsInserted(const QModelIndex& parent, int start, int end) const;
        void onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) const;
        void onRowsRemoved(const QModelIndex& parent, int start, int end) const;

    private:
        void deleteSelection();
        void rotateSelection(Rotation rot, bool page) const;
        QString gatherStats(const QModelIndexList& indices) const;

        QFileSystemModel* _model{nullptr};
        QSortFilterProxyModel* _proxyModel{nullptr};

        QList<EdgeItem*> _selectedEdges;
    };
}

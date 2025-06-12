/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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
        ~FileSystemScene() override;
        void addSceneBookmark(const QPoint& clickPos, const QString& name);
        void removeSceneBookmark(SceneBookmarkItem* bm);
        [[nodiscard]] QPersistentModelIndex rootIndex() const;
        bool isDir(const QModelIndex& index) const;
        [[nodiscard]] QString filePath(const QPersistentModelIndex& index) const;
        [[nodiscard]] QPersistentModelIndex index(const QString& paht) const;

        void setRootPath(const QString& newPath) const;
        bool openFile(const NodeItem* node) const;
        void openTo(const QString &targetPath) const;
        void fetchMore(const QPersistentModelIndex& index) const;

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

        QFileSystemModel* _model{nullptr};
        QSortFilterProxyModel* _proxyModel{nullptr};
    };
}

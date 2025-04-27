#pragma once

#include <QGraphicsScene>


class QFileSystemModel;
class QSortFilterProxyModel;


namespace core
{
    namespace nodes
    {
        class SceneBookmarkItem;
    }
    class Node;

    class FileSystemScene final : public QGraphicsScene
    {
        Q_OBJECT

    public:
        static void configure(FileSystemScene* scene);

        explicit FileSystemScene(QObject* parent = nullptr);
        ~FileSystemScene() override;
        void addSceneBookmark(const QPoint& pos, const QString& name);
        QPersistentModelIndex rootIndex() const;
        bool isDir(const QModelIndex& index) const;

    public slots:
        void openSelectedNodes() const;
        void closeSelectedNodes() const;
        void halfCloseSelectedNodes() const;

    protected:
        void drawBackground(QPainter* p, const QRectF& rec) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override;

    private slots:
        void onSelectionChange();
        void onRowsInserted(const QModelIndex& parent, int start, int end) const;
        void onRowsRemoved(const QModelIndex& parent, int start, int end) const;

    private:
        bool openFile(const Node* node) const;

        QFileSystemModel* _model{nullptr};
        QSortFilterProxyModel* _proxyModel{nullptr};
    };
}

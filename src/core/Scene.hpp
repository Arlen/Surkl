#pragma once

#include <QGraphicsScene>


class QFileSystemModel;

namespace  gui::view
{
    class Inode;
}

namespace core
{
    namespace nodes
    {
        class SceneBookmarkItem;
    }

    class FileSystemScene final : public QGraphicsScene
    {
        Q_OBJECT

    public:
        static void configure(FileSystemScene* scene);

        explicit FileSystemScene(QObject* parent = nullptr);
        ~FileSystemScene() override;
        void addSceneBookmark(const QPoint& pos, const QString& name);
        QPersistentModelIndex rootIndex() const;

    public slots:
        void openSelectedNodes() const;
        void closeSelectedNodes() const;
        void halfCloseSelectedNodes() const;

    protected:
        void drawBackground(QPainter* p, const QRectF& rec) override;

    private slots:
        void onSelectionChange();
        void onRowsInserted(const QModelIndex& parent, int start, int end) const;
        void onRowsRemoved(const QModelIndex& parent, int start, int end) const;

    private:
        QFileSystemModel* _model{nullptr};
    };
}

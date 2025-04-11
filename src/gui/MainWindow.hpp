#pragma once


#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QWidget>


namespace core
{
    class FileSystemScene;
}

namespace gui
{
    namespace view
    {
        class GraphicsView;
    }

    class MainWindow : public QWidget
    {
    public:
        MainWindow(core::FileSystemScene* scene, QWidget* parent = nullptr);

    private:
        QVBoxLayout* _layout;
        view::GraphicsView* _view;
    };
}
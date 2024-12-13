#pragma once


#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QWidget>

namespace gui
{
    namespace view
    {
        class GraphicsView;
    }

    class MainWindow : public QWidget
    {
    public:
        MainWindow(QGraphicsScene* scene, QWidget* parent = nullptr);

    private:
        QVBoxLayout* _layout;
        view::GraphicsView* _view;
    };
}
#include "MainWindow.hpp"
#include "GraphicsView.hpp"

#include <QShortcut>


using namespace gui;

MainWindow::MainWindow(QGraphicsScene* scene, QWidget* parent)
    : QWidget(parent)
{
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->setSpacing(0);

    _view = new GraphicsView(scene, this);
    _layout->addWidget(_view);

    auto* shortcut = new QShortcut(QKeySequence(Qt::Key_B), _view);
    connect(shortcut, &QShortcut::activated, _view, &GraphicsView::requestSceneBookmark);
}

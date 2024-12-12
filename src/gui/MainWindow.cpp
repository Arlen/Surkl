#include "MainWindow.hpp"
#include "view/GraphicsView.hpp"

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

    auto* quadShortcut1 = new QShortcut(QKeySequence(Qt::Key_1), _view);
    connect(quadShortcut1, &QShortcut::activated, _view, &GraphicsView::focusQuadrant1);

    auto* quadShortcut2 = new QShortcut(QKeySequence(Qt::Key_2), _view);
    connect(quadShortcut2, &QShortcut::activated, _view, &GraphicsView::focusQuadrant2);

    auto* quadShortcut3 = new QShortcut(QKeySequence(Qt::Key_3), _view);
    connect(quadShortcut3, &QShortcut::activated, _view, &GraphicsView::focusQuadrant3);

    auto* quadShortcut4 = new QShortcut(QKeySequence(Qt::Key_4), _view);
    connect(quadShortcut4, &QShortcut::activated, _view, &GraphicsView::focusQuadrant4);

    auto* allQuadShortcut = new QShortcut(QKeySequence(Qt::Key_5), _view);
    connect(allQuadShortcut, &QShortcut::activated, _view, &GraphicsView::focusAllQuadrants);
}

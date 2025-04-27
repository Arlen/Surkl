#include "MainWindow.hpp"
#include "view/GraphicsView.hpp"
#include "core/FileSystemScene.hpp"

#include <QShortcut>


using namespace gui;

MainWindow::MainWindow(core::FileSystemScene* scene, QWidget* parent)
    : QWidget(parent)
{
    using namespace view;

    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->setSpacing(0);

    _view = new GraphicsView(scene, this);
    _layout->addWidget(_view);

    auto* shortcut        = new QShortcut(QKeySequence(Qt::Key_B), _view, _view, &GraphicsView::requestSceneBookmark);
    auto* quadShortcut1   = new QShortcut(QKeySequence(Qt::Key_1), _view, _view, &GraphicsView::focusQuadrant1);
    auto* quadShortcut2   = new QShortcut(QKeySequence(Qt::Key_2), _view, _view, &GraphicsView::focusQuadrant2);
    auto* quadShortcut3   = new QShortcut(QKeySequence(Qt::Key_3), _view, _view, &GraphicsView::focusQuadrant3);
    auto* quadShortcut4   = new QShortcut(QKeySequence(Qt::Key_4), _view, _view, &GraphicsView::focusQuadrant4);
    auto* allQuadShortcut = new QShortcut(QKeySequence(Qt::Key_5), _view, _view, &GraphicsView::focusAllQuadrants);

    auto* openShortcut      = new QShortcut(QKeySequence::Open, this, scene, &core::FileSystemScene::openSelectedNodes);
    auto* closeShortcut     = new QShortcut(QKeySequence::Close, this, scene, &core::FileSystemScene::closeSelectedNodes);

    const QKeySequence closeKeySeq = QKeySequence::Close;
    Q_ASSERT(closeKeySeq.count() > 0);
    const auto halfCloseMod = closeKeySeq[0].keyboardModifiers() | Qt::ShiftModifier;
    const auto halfCloseKey = closeKeySeq[0].key();
    auto* halfCloseShortcut = new QShortcut(QKeySequence(QKeyCombination(halfCloseMod, halfCloseKey)),
        this, scene, &core::FileSystemScene::halfCloseSelectedNodes);
}
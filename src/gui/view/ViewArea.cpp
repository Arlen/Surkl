/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "ViewArea.hpp"
#include "GraphicsView.hpp"
#include "core/FileSystemScene.hpp"

#include <QShortcut>


using namespace gui::view;

ViewArea::ViewArea(core::FileSystemScene* scene, window::Window *parent)
    : AbstractWindowArea(parent)
{
    auto* view = new GraphicsView(scene, this);
    setWidget(AreaType::ViewArea, view);

    auto* shortcut        = new QShortcut(QKeySequence(Qt::Key_B), view, view, &GraphicsView::requestSceneBookmark);
    auto* quadShortcut1   = new QShortcut(QKeySequence(Qt::Key_1), view, view, &GraphicsView::focusQuadrant1);
    auto* quadShortcut2   = new QShortcut(QKeySequence(Qt::Key_2), view, view, &GraphicsView::focusQuadrant2);
    auto* quadShortcut3   = new QShortcut(QKeySequence(Qt::Key_3), view, view, &GraphicsView::focusQuadrant3);
    auto* quadShortcut4   = new QShortcut(QKeySequence(Qt::Key_4), view, view, &GraphicsView::focusQuadrant4);
    auto* allQuadShortcut = new QShortcut(QKeySequence(Qt::Key_5), view, view, &GraphicsView::focusAllQuadrants);
    auto* openShortcut    = new QShortcut(QKeySequence::Open, view, scene, &core::FileSystemScene::openSelectedNodes);
    auto* closeShortcut   = new QShortcut(QKeySequence::Close, view, scene, &core::FileSystemScene::closeSelectedNodes);

    const QKeySequence closeKeySeq = QKeySequence::Close;
    Q_ASSERT(closeKeySeq.count() > 0);
    const auto halfCloseMod = closeKeySeq[0].keyboardModifiers() | Qt::ShiftModifier;
    const auto halfCloseKey = closeKeySeq[0].key();
    auto* halfCloseShortcut = new QShortcut(QKeySequence(QKeyCombination(halfCloseMod, halfCloseKey)),
        view, scene, &core::FileSystemScene::halfCloseSelectedNodes);

    shortcut->setContext(Qt::WidgetShortcut);
    quadShortcut1->setContext(Qt::WidgetShortcut);
    quadShortcut2->setContext(Qt::WidgetShortcut);
    quadShortcut3->setContext(Qt::WidgetShortcut);
    quadShortcut4->setContext(Qt::WidgetShortcut);
    allQuadShortcut->setContext(Qt::WidgetShortcut);
    openShortcut->setContext(Qt::WidgetShortcut);
    closeShortcut->setContext(Qt::WidgetShortcut);
    halfCloseShortcut->setContext(Qt::WidgetShortcut);
}

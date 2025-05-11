/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "MainWindow.hpp"
#include "view/GraphicsView.hpp"
#include "ThemeSettings.hpp"
#include "core/FileSystemScene.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QShortcut>
#include <QTabWidget>


using namespace gui;

MainWindow::MainWindow(core::FileSystemScene* scene, QWidget* parent)
    : QWidget(parent)
{
    using namespace view;

    _layout = new QHBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->setSpacing(0);

    _view = new GraphicsView(scene, this);
    _layout->addWidget(_view);

    const auto* shortcut        = new QShortcut(QKeySequence(Qt::Key_B), _view, _view, &GraphicsView::requestSceneBookmark);
    const auto* quadShortcut1   = new QShortcut(QKeySequence(Qt::Key_1), _view, _view, &GraphicsView::focusQuadrant1);
    const auto* quadShortcut2   = new QShortcut(QKeySequence(Qt::Key_2), _view, _view, &GraphicsView::focusQuadrant2);
    const auto* quadShortcut3   = new QShortcut(QKeySequence(Qt::Key_3), _view, _view, &GraphicsView::focusQuadrant3);
    const auto* quadShortcut4   = new QShortcut(QKeySequence(Qt::Key_4), _view, _view, &GraphicsView::focusQuadrant4);
    const auto* allQuadShortcut = new QShortcut(QKeySequence(Qt::Key_5), _view, _view, &GraphicsView::focusAllQuadrants);

    const auto* openShortcut      = new QShortcut(QKeySequence::Open, this, scene, &core::FileSystemScene::openSelectedNodes);
    const auto* closeShortcut     = new QShortcut(QKeySequence::Close, this, scene, &core::FileSystemScene::closeSelectedNodes);

    const QKeySequence closeKeySeq = QKeySequence::Close;
    Q_ASSERT(closeKeySeq.count() > 0);
    const auto halfCloseMod = closeKeySeq[0].keyboardModifiers() | Qt::ShiftModifier;
    const auto halfCloseKey = closeKeySeq[0].key();
    const auto* halfCloseShortcut = new QShortcut(QKeySequence(QKeyCombination(halfCloseMod, halfCloseKey)),
        this, scene, &core::FileSystemScene::halfCloseSelectedNodes);

    /// TODO: park these here for now.
    auto* settings = new QTabWidget(this);
    settings->setMinimumWidth(512);
    settings->setMaximumWidth(512);
    settings->hide();
    _layout->addWidget(settings);

    auto* themeSettings = new ThemeSettings(this);
    settings->addTab(themeSettings, "Theme");

    auto* settingsButton = new QPushButton("Settings", this);
    settingsButton->setCheckable(true);

    connect(settingsButton, &QPushButton::toggled, settings, &QTabWidget::setVisible);
    connect(settingsButton, &QPushButton::toggled, [this, settingsButton](bool checked) {
        if (const auto offset = QPoint(settingsButton->width(), 0); checked) {
            settingsButton->move(_view->rect().topRight() - offset);
        } else {
            settingsButton->move(QPoint(width(), 0) - offset);
        }
    });


    Q_UNUSED(shortcut);
    Q_UNUSED(quadShortcut1);
    Q_UNUSED(quadShortcut2);
    Q_UNUSED(quadShortcut3);
    Q_UNUSED(quadShortcut4);
    Q_UNUSED(allQuadShortcut);
    Q_UNUSED(openShortcut);
    Q_UNUSED(closeShortcut);
    Q_UNUSED(halfCloseShortcut);
}

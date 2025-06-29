/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later


#include "AbstractWindowArea.hpp"
#include "TitleBar.hpp"
#include "Window.hpp"
#include "core/SessionManager.hpp"
#include "theme/ThemeArea.hpp"
#include "view/ViewArea.hpp"

#include <QMenu>
#include <QPushButton>


using namespace gui::window;

AbstractWindowArea::AbstractWindowArea(Window *parent)
    : QWidget(parent)
{

}

void AbstractWindowArea::setTitleBar(TitleBar *titleBar)
{
    _titleBar = titleBar;
    Q_ASSERT(_titleBar != nullptr);
    Q_ASSERT(qobject_cast<Window*>(parentWidget()) != nullptr);

    auto* menu = new QMenu(this);

    if (qobject_cast<view::ViewArea*>(this) == nullptr) {
        auto *action = new QAction("View", menu);
        connect(action, &QAction::triggered,
            this, &AbstractWindowArea::switchToView);
        menu->addAction(action);
    }

    if (qobject_cast<theme::ThemeArea *>(this) == nullptr) {
        auto *action = new QAction("Theme Settings", menu);
        connect(action, &QAction::triggered,
            this, &AbstractWindowArea::switchToThemeSettings);
        menu->addAction(action);
    }

    auto* moveTo = new QAction("Move to New Window");
    connect(moveTo, &QAction::triggered,
        this, &AbstractWindowArea::moveToNewMainWindow);
    menu->addAction(moveTo);
    menu->addSeparator();

    if (auto* button = qobject_cast<QPushButton*>(_titleBar->menuButton()); button) {
        if (auto* oldMenu = button->menu()) {
            oldMenu->deleteLater();
        }
        button->setMenu(menu);
        button->show();
    }
    _titleBar->titleButton()->setText(tr(".?."));
}

void AbstractWindowArea::switchToView()
{
    if (auto* window = qobject_cast<Window*>(parentWidget()); window) {
        auto* va = new view::ViewArea(core::SessionManager::scene(), window);
        window->setWidget(va);
    }
}

void AbstractWindowArea::switchToThemeSettings()
{
    if (auto* window = qobject_cast<Window*>(parentWidget()); window) {
        window->setWidget(new theme::ThemeArea(window));
    }
}

void AbstractWindowArea::moveToNewMainWindow()
{
    if (auto* window = qobject_cast<Window*>(parentWidget()); window) {
        window->moveToNewMainWindow();
    }
}

TitleBar *AbstractWindowArea::titleBar() const
{
    return _titleBar;
}

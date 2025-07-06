/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "MainWindow.hpp"
#include "Splitter.hpp"
#include "UiStorage.hpp"
#include "core/SessionManager.hpp"
#include "version.hpp"
#include "view/GraphicsView.hpp"
#include "window/Window.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>


using namespace gui;

/// The first MainWindow that's created is considered to be the main, and all
/// the other MainWindows are siblings of the main.  The main is responsible
/// for deleting the siblings from DB when they are closed.
MainWindow::MainWindow()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _splitter = new Splitter(Qt::Horizontal, this);
    layout->addWidget(_splitter);

    updateTitle();

    core::SessionManager::us()->saveMainWindow(this);
}

/// creates a new sibling.
void MainWindow::moveToNewMainWindow(window::Window *source)
{
    if (source) {
        auto* mw = new MainWindow();
        connect(mw, &MainWindow::closed, mw, &MainWindow::closeSibling);
        Q_ASSERT(mw->splitter());
        Q_ASSERT(mw->splitter()->count() == 1);
        auto* target = qobject_cast<window::Window*>(mw->splitter()->widget(0));
        Q_ASSERT(target);
        mw->resize(source->geometry().size());
        Splitter::swap(source, target);
        mw->show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    if (widgetId() == 0) {
        /// disconnect all siblings so that they aren't deleted from DB.
        for (auto xs = QApplication::topLevelWidgets(); auto* x : xs) {
            if (auto* mw = qobject_cast<MainWindow*>(x)) {
                disconnect(mw, &MainWindow::closed, nullptr, nullptr);
            }
        }
        qApp->quit();
    } else {
        emit closed(widgetId());
    }
}

void MainWindow::closeSibling(qint32 id)
{
    const auto windows = findChildren<window::Window*>();

    QList<qint32> winIds, viewIds;
    for (auto* win : windows) {
        const auto winId = win->widgetId();
        winIds.push_back(winId);
        if (qobject_cast<const view::GraphicsView*>(win->areaWidget()->widget())) {
            viewIds.push_back(winId);
        }
    }
    /// TODO: splitters
    core::SessionManager::us()->deleteWindow(winIds);
    core::SessionManager::us()->deleteView(viewIds);
    core::SessionManager::us()->deleteMainWindow(id);
}

void MainWindow::updateTitle()
{
    QSet<const MainWindow*> mainWindows;

    for (const auto topLevel = QApplication::topLevelWidgets(); const auto* widget : topLevel) {
        if (const auto* mw = qobject_cast<const MainWindow*>(widget)) {
            mainWindows.insert(mw);
        }
    }

    setWindowTitle(QString("Surkl %1 @ Window %2")
        .arg(version())
        .arg(mainWindows.size()));
}

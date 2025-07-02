/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "MainWindow.hpp"
#include "Splitter.hpp"
#include "version.hpp"
#include "window/Window.hpp"

#include <QCloseEvent>
#include <QHBoxLayout>


using namespace gui;

MainWindow::MainWindow()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _splitter = new Splitter(Qt::Horizontal, this);
    layout->addWidget(_splitter);

    updateTitle();
}

void MainWindow::moveToNewMainWindow(window::Window *source)
{
    if (source) {
        auto* mw = new MainWindow();
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
    if (widgetId() == 0) {
        event->accept();
        qApp->quit();
    } else {
        QWidget::closeEvent(event);
    }
}

void MainWindow::updateTitle()
{
    setWindowTitle(QString("Surkl %1 @ Window %2")
        .arg(version())
        .arg(widgetId() + 1));
}

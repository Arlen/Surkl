/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "MainWindow.hpp"
#include "InfoBar.hpp"
#include "Splitter.hpp"
#include "UiStorage.hpp"
#include "core/SessionManager.hpp"
#include "version.hpp"
#include "view/GraphicsView.hpp"
#include "window/Window.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include <set>
#include <stack>


using namespace gui;

namespace
{
    auto getMainWindows()
    {
        auto isMainWindow = [](QWidget* widget) -> bool {
            return qobject_cast<MainWindow*>(widget) != nullptr;
        };

        auto asMainWindow = [](QWidget* widget) {
            return qobject_cast<MainWindow*>(widget);
        };

        auto cmp = [](const MainWindow* alpha, const MainWindow* beta) -> bool {
            return alpha->widgetId() < beta->widgetId();
        };

        auto result = QApplication::topLevelWidgets()
            | std::views::filter(isMainWindow)
            | std::views::transform(asMainWindow)
            | std::ranges::to<QList<MainWindow*>>()
            ;

        std::ranges::sort(result, cmp);

        return result;
    }

    MainWindow* factoryDefault()
    {
        auto* mw = new MainWindow();
        mw->splitter()->addWindow();
        mw->resize(640*2, 480*2);

        return mw;
    }
}

/// The first MainWindow that's created is considered to be the main, and all
/// the other MainWindows are siblings of the main.
MainWindow::MainWindow()
    : MainWindow(new Splitter(Qt::Horizontal))
{

}

MainWindow::MainWindow(Splitter* splitter)
    : _splitter(splitter)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _splitter->setParent(this);
    layout->addWidget(_splitter);

    _showInfoBar = new QPushButton(this);
    _showInfoBar->setFixedSize(48, 12);
    _showInfoBar->hide();

    _infoBar = new InfoBar(this);
    layout->addWidget(_infoBar);

    connect(_infoBar, &InfoBar::hidden, _showInfoBar, &QPushButton::show);
    connect(_showInfoBar, &QPushButton::pressed, _showInfoBar, &QPushButton::hide);
    connect(_showInfoBar, &QPushButton::pressed, _infoBar, &QWidget::show);


    setTitle();

    connect(this
        , &MainWindow::stateChanged
        , core::SessionManager::us()
        , qOverload<const MainWindow*>(&UiStorage::stateChanged));

    emit stateChanged(this);
}

MainWindow* MainWindow::first()
{
    if (const auto mws = getMainWindows(); !mws.isEmpty()) {
        return mws.first();
    }

    return nullptr;
}

MainWindow* MainWindow::loadUi()
{
    auto state = core::SessionManager::us()->load();

    core::SessionManager::us()->clearTables();

    struct SplitterNode
    {
        Splitter* splitter{nullptr};
        storage::SplitterId spId{-1};
    };

    struct SplitterSizes
    {
        Splitter* splitter{nullptr};
        QList<int> sizes;
    };

    std::map<WidgetId::ValueType, MainWindow*> mainWindows;

    for (const auto [mwSize, spId] : state.mws | std::views::values) {
        auto* mw = new MainWindow();
        mw->resize(mwSize);

        std::vector<SplitterSizes> L;
        std::stack<SplitterNode> S;
        S.push({mw->splitter(), spId});

        while (!S.empty()) {
            auto [splitter, spId] = S.top(); S.pop();

            if (!state.splitters.contains(spId)) {
                continue;
            }

            QList<qint32> childSizes;
            for (auto [childIndex, childId] : state.splitters[spId].widgets) {
                Q_ASSERT(childIndex == splitter->count());
                if (state.windows.contains(childId)) {
                    auto* win = splitter->addWindow();
                    childSizes.push_back(state.windows[childId].size);
                    if (state.windows[childId].type == window::AbstractWindowArea::ThemeArea) {
                        win->switchToThemeSettings();
                    } else {
                        if (auto* view = qobject_cast<view::GraphicsView*>(win->areaWidget()->widget())) {
                            const auto [focus, zoom] = state.views[childId];
                            view->focusOn(focus, zoom);
                        }
                    }
                } else if (state.splitters.contains(childId)) {
                    auto* childSplitter = splitter->addSplitter();
                    const auto childSplitterSize = state.splitters[childId].size;
                    childSizes.push_back(childSplitterSize);
                    S.push({childSplitter, childId});
                } else {
                    Q_ASSERT(false);
                }
            }
            state.splitters.erase(spId);

            Q_ASSERT(splitter->count() == childSizes.count());
            L.push_back({splitter, childSizes});
        }

        for (const auto& [splitter, sizes] : L) {
            splitter->setSizes(sizes);
        }

        if (mw->splitter()->count() == 0) {
            mw->splitter()->addWindow();
        }

        mw->show();
        Q_ASSERT(!mainWindows.contains(mw->widgetId()));
        mainWindows[mw->widgetId()] = mw;
    }

    if (mainWindows.empty()) {
        return factoryDefault();
    }

    for (auto [id, sibling] : mainWindows | std::views::drop(1)) {
        connect(sibling, &MainWindow::closed, sibling, &MainWindow::deleteFromDb);
    }

    return mainWindows.begin()->second;
}

/// creates a new sibling.
void MainWindow::moveToNewMainWindow(window::Window *source)
{
    if (source) {
        auto* mw = new MainWindow();
        mw->splitter()->addWindow();
        connect(mw, &MainWindow::closed, mw, &MainWindow::deleteFromDb);
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

    if (auto mainWindows = getMainWindows(); mainWindows.front() == this) {
        /// disconnect all siblings so that they aren't deleted from DB.
        mainWindows.takeFirst();
        for (auto* mw : mainWindows) {
            disconnect(mw, &MainWindow::closed, nullptr, nullptr);
            mw->deleteLater();
        }
        qApp->quit();
    } else {
        Q_ASSERT(mainWindows.indexOf(this) != -1);
        emit closed(widgetId());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    stateChanged(this);

    const auto pos = rect().bottomRight();
    _showInfoBar->move(pos - QPoint(_showInfoBar->width(), _showInfoBar->height()));

    QWidget::resizeEvent(event);
}

void MainWindow::deleteFromDb(qint32 idOfMainWindow)
{
    const auto idsOfViewParents = findChildren<window::Window*>()
        | std::views::filter(
            [](const window::Window* win) -> bool {
                return win->areaWidget()->type() == window::AbstractWindowArea::ViewArea;
            })
        | std::views::transform(&window::Window::widgetId)
        | std::ranges::to<QList<WidgetId::ValueType>>()
        ;

    const auto idsOfWindows = findChildren<window::Window*>()
        | std::views::transform(&window::Window::widgetId)
        | std::ranges::to<QList<WidgetId::ValueType>>()
        ;

    const auto idsOfSplitters = findChildren<Splitter*>()
        | std::views::transform(&Splitter::widgetId)
        | std::ranges::to<QList<WidgetId::ValueType>>()
        ;

    auto* us = core::SessionManager::us();
    us->deleteWindow(idsOfWindows);
    us->deleteView(idsOfViewParents);
    us->deleteSplitter(idsOfSplitters);
    us->deleteMainWindow(idOfMainWindow);
}

void MainWindow::setTitle()
{
    setWindowTitle(QString("@ Window %1")
        .arg(getMainWindows().size()));
}

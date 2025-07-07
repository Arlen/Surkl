/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "Splitter.hpp"
#include "SplitterHandle.hpp"
#include "UiStorage.hpp"
#include "core/SessionManager.hpp"
#include "window/Window.hpp"


using namespace gui;
using namespace gui::window;

Splitter::Splitter(Qt::Orientation orientation, QWidget *parent)
    : Splitter(-1, orientation, parent)
{
}

Splitter::Splitter(qint32 id, Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
    , _id(id)
{
    setHandleWidth(7);
    setOpaqueResize(false);
    setChildrenCollapsible(false);
    addWindow(createEmptyWindow());

    /// See the FIX comment in Splitter::splitWindow.
    /// This is a similar fix, except for when just moving the SplitterHandle
    /// to resize the window.
    connect(this, &QSplitter::splitterMoved, [this](int pos, int index) {
        Q_UNUSED(pos)
        /// First save to DB (this has nothing to do with the comment above,
        /// just a needed save to DB).
        core::SessionManager::us()->saveSplitter(this);

        handle(index)->hide();
    });
}

void Splitter::addWindow(Window *window)
{
    connectWindowToThisSplitter(window);
    addWidget(window);

    if (!window->isVisible()) {
        window->show();
    }
}

void Splitter::insertWindow(int index, Window *window)
{
    connectWindowToThisSplitter(window);
    insertWidget(index, window);

    if (!window->isVisible()) {
        window->show();
    }
}

/// If the parent widget is another splitter, returns the index into that
/// parent splitter; otherwise, it returns -1.
int Splitter::row()
{
    if (const auto *parent = qobject_cast<Splitter *>(parentWidget())) {
        return parent->indexOf(this);
    }

    return -1;
}

/// This is almost the exact same code as 'splitWindow' else statement.  It
/// exists to accommodate MainWindow when reloading the UI and the splitter
/// id needs to be set externally during a split operation.
Splitter *Splitter::splitWindowAsSplitter(const QPoint& pos, Qt::Orientation splitOrientation, Window *child,
                                          qint32 newSplitterId)
{
    Q_ASSERT(newSplitterId != -1);
    const auto widgetsSizes = sizes();
    const auto childIndex   = indexOf(child);
    Q_ASSERT(childIndex != -1);
    Q_ASSERT(orientation() != splitOrientation);

    auto* splitter = new Splitter(newSplitterId, splitOrientation);
    const auto size = splitOrientation == Qt::Horizontal ? pos.x() : pos.y();

    splitter->addWindow(child);
    insertWidget(childIndex, splitter);
    setSizes(widgetsSizes);
    splitter->moveSplitter(size, 1);

    splitter->handle(1)->hide();

    return splitter;
}

void Splitter::splitWindow(const QPoint& pos, Qt::Orientation splitOrientation, Window *child)
{
    auto widgetsSizes = sizes();
    const auto childIndex = indexOf(child);
    Q_ASSERT(childIndex != -1);
    const auto childSize = widgetsSizes[childIndex];

    if (orientation() == splitOrientation) {
        const auto leftOrTopSize = orientation() == Qt::Vertical ? pos.y() : pos.x();
        widgetsSizes[childIndex] = leftOrTopSize;
        widgetsSizes.insert(childIndex + 1, childSize - leftOrTopSize - handleWidth());
        insertWindow(indexOf(child), createEmptyWindow());
        setSizes(widgetsSizes);

        /// FIX: This is a fix to a minor bug where the correct cursor is not
        /// shown when the mouse pointer is right on top of the SplitterHandle
        /// immediately after a split.  Repo steps:
        /// 1. Perform a  split
        /// 2. Release the mouse button without moving the mouse
        /// 3. The new SplitterHandle is under the mouse pointer but without
        ///    the correct cursor shape.
        /// Moving the mouse pointer  by just a pixel updates the cursor to the
        /// correct one.  That happens because then the Window receives a Leave
        /// event, and the SplitterHandle receives a Enter event.
        /// The only fix I found is this (i.e., hide).  No need to call show()
        /// on it, as the parent Splitter seem to do it.
        handle(indexOf(child))->hide();

        core::SessionManager::us()->saveSplitter(this);
    } else {
        const auto size = splitOrientation == Qt::Horizontal ? pos.x() : pos.y();

        auto* splitter = new Splitter(splitOrientation);
        splitter->addWindow(child);
        insertWidget(childIndex, splitter);
        setSizes(widgetsSizes);
        splitter->moveSplitter(size, 1);

        /// FIX: This is a fix to a minor bug where the correct cursor is not
        /// shown when the mouse pointer is right on top of the SplitterHandle
        /// immediately after a split.  Repo steps:
        /// 1. Perform a  split
        /// 2. Release the mouse button without moving the mouse
        /// 3. The new SplitterHandle is under the mouse pointer but without
        ///    the correct cursor shape.
        /// Moving the mouse pointer  by just a pixel updates the cursor to the
        /// correct one.  That happens because then the Window receives a Leave
        /// event, and the SplitterHandle receives a Enter event.
        /// The only fix I found is this (i.e., hide).  No need to call show()
        /// on it, as the parent Splitter seem to do it.
        splitter->handle(1)->hide();

        core::SessionManager::us()->saveSplitter(this);
        core::SessionManager::us()->saveSplitter(splitter);
    }
}

void Splitter::deleteChild(Window *child)
{
    Q_ASSERT(child != nullptr);
    Q_ASSERT(child->parentWidget() == this);

    if (count() > 2) {
        const auto childIndex = indexOf(child);
        auto sz = sizes();
        const auto childSize = sz[childIndex];
        const auto childSizeHalf = childSize / 2;

        /// half of the size is given to the window before the window that is
        /// going to be deleted, and the other half is given to the window
        /// after it.  this way the splitter handle can be moved to the middle
        /// position of where the soon-to-be-deleted window is.
        if (childIndex > 0 && childIndex < sz.size() - 1) {
            sz[childIndex - 1] += childSizeHalf;
            sz[childIndex + 1] += childSizeHalf + handleWidth();
        } else if (childIndex == 0) {
            sz[1] += childSize + handleWidth();
        } else if (childIndex == sz.size() - 1) {
            Q_ASSERT(sz.size() > 2);
            sz[sz.size() - 2] += childSize + handleWidth();
        }
        sz.remove(childIndex);
        //delete child;
        child->deleteLater();
        child->setParent(nullptr);
        setSizes(sz);
    } else if (count() == 2) {
        if (auto* parentSplitter = qobject_cast<Splitter *>(parentWidget())) {
            /// if the splitter is NOT root, then we'll have to transfer
            /// ownership of the orphan child to the parent splitter after the
            /// other child has been deleted.

            const int indexOfThisSplitter = parentSplitter->indexOf(this);
            //delete child;
            child->deleteLater();
            child->setParent(nullptr);

            if (auto *window = qobject_cast<Window *>(widget(0))) {
                parentSplitter->takeOwnershipOf(window, indexOfThisSplitter);
            } else if (auto* splitter = qobject_cast<Splitter *>(widget(0))) {
                parentSplitter->takeOwnershipOf(splitter, indexOfThisSplitter);
            }
        } else {
            child->deleteLater();
        }
    }
}

void Splitter::swap(Window *winA, Window *winB)
{
    if (winA == nullptr || winB == nullptr) {
        return;
    }

    auto *parentA = qobject_cast<Splitter *>(winA->parentWidget());
    Q_ASSERT(parentA);
    auto *parentB = qobject_cast<Splitter *>(winB->parentWidget());
    Q_ASSERT(parentB);

    const auto winAIndex = parentA->indexOf(winA);
    const auto winBIndex = parentB->indexOf(winB);

    const auto parentASizes = parentA->sizes();
    const auto parentBSizes = parentB->sizes();

    parentA->insertWindow(winAIndex, winB);
    parentB->insertWindow(winBIndex, winA);

    parentA->setSizes(parentASizes);
    core::SessionManager::us()->saveSplitter(parentA);

    if (parentA != parentB) {
        parentB->setSizes(parentBSizes);
        core::SessionManager::us()->saveSplitter(parentB);
    }
}

QSplitterHandle *Splitter::createHandle()
{
    return new SplitterHandle(orientation(), this);
}

Window *Splitter::createEmptyWindow() const
{
    auto* win = new Window();

    connect(win, &Window::swapRequested, this, &Splitter::swap);

    return win;
}

void Splitter::connectWindowToThisSplitter(const Window *child) const
{
    disconnect(child, &Window::splitWindowRequested, nullptr, nullptr);
    disconnect(child, &Window::closed, nullptr, nullptr);

    connect(child, &Window::splitWindowRequested, this, &Splitter::splitWindow);
    connect(child, &Window::closed, this, &Splitter::deleteChild);
}

void Splitter::takeOwnershipOf(Window *orphanWindow, int childSplitterIndex)
{
    const auto sz = sizes();
    auto childSplitter = qobject_cast<Splitter *>(widget(childSplitterIndex));
    Q_ASSERT(childSplitter->count() == 1);
    core::SessionManager::us()->deleteSplitter(childSplitter->widgetId());
    childSplitter->setParent(nullptr);
    childSplitter->deleteLater();

    insertWidget(childSplitterIndex, orphanWindow);
    connectWindowToThisSplitter(orphanWindow);
    setSizes(sz);
    core::SessionManager::us()->saveSplitter(this);
}

/// This function is a more complicated version of takeOwnershipOf(). it takes
/// care of taking ownership of a splitter. it does NOT actually take ownership
/// of orphanSplitter, but, rather, it takes ownership of its children (this is
/// basically unpacking the orphanSplitter). NOTE: the unpacking only occurs if
/// the orientation of orphanSplitter matches of that this splitter; otherwise,
/// the 'else' is executed.
void Splitter::takeOwnershipOf(Splitter *orphanSplitter, int childSplitterIndex)
{
    Q_ASSERT(count() > 0);
    Q_ASSERT(childSplitterIndex < count());
    if (orientation() == orphanSplitter->orientation()) {
        QWidget *toBeDeletedChild = widget(childSplitterIndex);
        Q_ASSERT(qobject_cast<Splitter *>(toBeDeletedChild));
        Q_ASSERT(orphanSplitter->parentWidget() == toBeDeletedChild);

        const auto sz = sizes();

        auto newSizes = sz.sliced(0, childSplitterIndex);
        newSizes.append(orphanSplitter->sizes());
        if (auto next = childSplitterIndex + 1; next < sz.size()) {
            newSizes.append(sz.sliced(next));
        }

        /// unpack widgets in orphanSplitter into this Splitter.
        for (int i = childSplitterIndex, j = 0; j < orphanSplitter->count(); ++i) {
            auto childOfOrphanSplitter = orphanSplitter->widget(0);
            if (auto window = qobject_cast<Window *>(childOfOrphanSplitter)) {
                connectWindowToThisSplitter(window);
            }
            insertWidget(i, childOfOrphanSplitter);
        }
        Q_ASSERT(orphanSplitter->count() == 0);
        core::SessionManager::us()->deleteSplitter(qobject_cast<Splitter*>(toBeDeletedChild)->widgetId());
        toBeDeletedChild->setParent(nullptr);
        toBeDeletedChild->deleteLater();

        core::SessionManager::us()->deleteSplitter(orphanSplitter->widgetId());
        orphanSplitter->setParent(nullptr);
        orphanSplitter->deleteLater();
        setSizes(newSizes);
    } else {
        if (auto* sp = qobject_cast<Splitter*>(widget(childSplitterIndex))) {
            core::SessionManager::us()->deleteSplitter(sp->widgetId());
        }
        widget(childSplitterIndex)->deleteLater();
        insertWidget(childSplitterIndex, orphanSplitter);
    }
    core::SessionManager::us()->saveSplitter(this);
}

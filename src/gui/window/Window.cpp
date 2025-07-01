/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "Window.hpp"
#include "AbstractWindowArea.hpp"
#include "TitleBar.hpp"
#include "core/SessionManager.hpp"
#include "theme/theme.hpp"
#include "view/ViewArea.hpp"

#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtDeprecationMarkers>

using namespace gui;
using namespace gui::window;


RubberBandState::RubberBandState(QWidget *parent)
{
    currOrientation = Qt::Horizontal;
    currGeom = QRect();
    currentPos = QPoint(32, 16);
    xDirLoad = 0;
    p = new RubberBand(parent);
}


Window::Window(QWidget *parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    initTitlebar(layout);
    setWidget(new view::ViewArea(core::SessionManager::scene(), this));
    setAcceptDrops(true);
}

TitleBar *Window::titleBar() const
{
    return _titleBar;
}

AbstractWindowArea *Window::widget() const
{
    return _widget;
}

void Window::setWidget(AbstractWindowArea *widget)
{
    Q_ASSERT(widget);

    if (_widget) {
        _widget->deleteLater();
    }

    _widget = widget;
    _widget->setParent(this);
    _widget->setTitleBar(_titleBar);
    _widget->setSizePolicy(QSizePolicy::MinimumExpanding,
        QSizePolicy::MinimumExpanding);
    layout()->addWidget(_widget);
}

void Window::moveToNewMainWindow()
{
    core::SessionManager::mw()->moveToNewMainWindow(this);
}

void Window::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("surkl/window-swap")) {
        if (event->source() == this) {
            event->accept();
        } else {
            event->acceptProposedAction();
            _overlay = new Overlay(Overlay::Destination, this);
            _overlay->setGeometry(QRect(QPoint(0, 0), size()));
            _overlay->show();
        }
    } else {
        event->ignore();
    }
}

void Window::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("surkl/window-swap")) {
        if (event->source() == this) {
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void Window::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (_overlay && !_overlay->isOrigin()) {
        _overlay->deleteLater();
        _overlay = nullptr;
    }

    QWidget::dragLeaveEvent(event);
}

void Window::dropEvent(QDropEvent *event)
{
    if (_overlay) {
        _overlay->deleteLater();
        _overlay = nullptr;
    }

    if (auto* other = qobject_cast<Window*>(event->source()); other != this) {
        emit swapRequested(this, other);
    }
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
    if (!_rbs.p) {
        return QWidget::mouseMoveEvent(event);
    }

    /// If _rbs.p is valid, then we are in the middle of splitting this Window.
    const auto newPos       = event->pos();
    const auto handleWidth  = splitterHandleWidth();
    const auto halfHandle   = handleWidth / 2;
    const auto velocity     = QVector2D(newPos - _rbs.currentPos).normalized();
    constexpr auto capacity = 32.0;

    _rbs.xDirLoad += qAbs(velocity.x());
    _rbs.xDirLoad -= qAbs(velocity.y());
    _rbs.xDirLoad  = qBound(-capacity, _rbs.xDirLoad, capacity);

    const QRect cr = contentsRect();

    QRect newGeom;

    /// when firstTime == true, newGeom is set based on the geometry of this
    /// window.  Meaning, if the widget has more width(), then the user most
    /// likely wants to split it horizontally.
    const auto firstTime = _rbs.currGeom.isNull();
    qreal perc = 0;

    if ((firstTime && cr.width() > cr.height()) || _rbs.xDirLoad > 4.0) /// left or right
    {
        const int x = qBound(0, newPos.x() - halfHandle, width() - handleWidth);
        perc = qreal(x) / qreal(width());
        newGeom = QRect(x, cr.y(), handleWidth, cr.height());

        _rbs.currGeom = newGeom;
        _rbs.currOrientation = Qt::Horizontal;
    } else if ((firstTime && cr.width() <= cr.height()) || _rbs.xDirLoad < -4.0) {
        const int y = qBound(0, newPos.y() - halfHandle, height() - handleWidth);
        perc = qreal(y) / qreal(height());
        newGeom = QRect(cr.x(), y, cr.width(), handleWidth);

        _rbs.currGeom = newGeom;
        _rbs.currOrientation = Qt::Vertical;
    } else {
        newGeom = _rbs.currGeom;
    }

    if (firstTime) {
        _rbs.savedTitle = titleBar()->titleButton()->text();
    } else {
        titleBar()->setTitle(QString::number(perc, 'f', 2));
    }

    _rbs.currentPos = newPos;
    _rbs.p->setGeometry(newGeom);
    _rbs.p->show();

    QWidget::mouseMoveEvent(event);
}

void Window::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && _rbs.p) {
        const auto handleWidth = splitterHandleWidth();
        const auto halfHandle  = handleWidth / 2;

        if (!_rbs.savedTitle.isNull()) {
            /// if savedTitle is null, that means the window was split with a
            /// simple left click (no left-click drag); therefore, there is no
            /// need to restore the title text to an incorrect empty string.
            _titleBar->setTitle(_rbs.savedTitle);
        }
        const QPoint offset = _rbs.currOrientation == Qt::Vertical
            ? QPoint(0, -halfHandle)
            : QPoint(-halfHandle, 0);

        QPoint splitPos;

        auto orient = _rbs.currOrientation;

        if (_rbs.currGeom.isNull()) {
            /// this usually happens with a quick click and release of the
            /// split button, so just go with the Golden Ratio.
            const auto sz = contentsRect().size().toSizeF() * 0.618;
            splitPos = QPoint(sz.width(), sz.height());

            /// The orientation is based on the greater extent of the
            /// content rect.
            const auto cr = contentsRect();
            orient = cr.width() > cr.height() ? Qt::Horizontal : Qt::Vertical;
        } else {
            splitPos = _rbs.p->geometry().center() + offset;
        }

        _rbs.p->deleteLater();
        emit splitWindowRequested(splitPos, orient, this);
        update();
    }

    QWidget::mouseReleaseEvent(event);
}

void Window::activateSplitMode()
{
    _rbs = RubberBandState(this);
}

void Window::activateSwapMode()
{
    if (_overlay == nullptr) {
        _overlay = new Overlay(Overlay::Origin, this);
        _overlay->setGeometry(QRect(QPoint(0, 0), size()));
        _overlay->show();
    }
    auto* mime = new QMimeData();
    mime->setData("surkl/window-swap", 0);

    auto* drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(WindowDragPixmap());
    drag->exec();

    if (_overlay) {
        _overlay->deleteLater();
        _overlay = nullptr;
    }
}

void Window::closeMe()
{
    emit closed(this);
}

void Window::initTitlebar(QVBoxLayout *layout)
{
    _titleBar = new TitleBar(this);
    layout->addWidget(_titleBar);

    connect(_titleBar->closeButton()
            , &QAbstractButton::released
            , this
            , &Window::closeMe);

    connect(_titleBar->splitButton()
            , &QAbstractButton::pressed
            , this
            , &Window::activateSplitMode);

    connect(_titleBar->titleButton()
            , &QAbstractButton::pressed
            , this
            , &Window::activateSwapMode);
}

int Window::splitterHandleWidth() const
{
    int result = 7;

    if (const auto *splitter = qobject_cast<QSplitter *>(parentWidget())) {
        result = splitter->handleWidth();
    }

    return result;
}


Overlay::Overlay(Movement movement, QWidget *parent)
    : QWidget(parent)
    , _movement(movement)
{
    raise();

    const auto* tm = core::SessionManager::tm();

    constexpr auto penW = 2;
    const auto space = movement == Origin ? 0.0 : 2.0;

    _pen1.setCapStyle(Qt::FlatCap);
    _pen1.setJoinStyle(Qt::MiterJoin);
    _pen1.setColor(tm->sceneLightColor());
    _pen1.setWidth(penW);
    _pen1.setDashPattern(QList<qreal>() << 2.0 << space << 2.0 << space);

    _pen2.setCapStyle(Qt::FlatCap);
    _pen2.setJoinStyle(Qt::MiterJoin);
    _pen2.setColor(tm->sceneDarkColor());
    _pen2.setWidth(penW);
}

void Overlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto offset = _pen1.width() / 2 + 2;
    const auto topL = rect().topLeft();
    const auto topR = rect().topRight();
    const auto botL = rect().bottomLeft();
    const auto botR = rect().bottomRight();

    QPainterPath path;
    path.moveTo(topL + QPoint(offset, offset));
    path.lineTo(topR + QPoint(-offset, offset));
    path.lineTo(botR - QPoint(offset, offset));
    path.lineTo(botL + QPoint(offset, -offset));
    path.closeSubpath();

    p.setBrush(Qt::NoBrush);
    p.setPen(_pen2);
    p.drawPath(path);
    p.setPen(_pen1);
    p.drawPath(path);
}


QPixmap window::WindowDragPixmap()
{
    constexpr qreal a = 3;
    constexpr qreal b = 1;
    const auto rec = QRect(0, 0, 16, 16);

    auto pix = QPixmap(rec.size());

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rec, Qt::black);
    p.fillRect(rec.adjusted(b, b + a, -b, -b), Qt::white);
    p.fillRect(rec.adjusted(b * 2, b * 2 + a, -b * 2, -b * 2), Qt::black);
    p.fillRect(QRect(b, b, rec.width() - b * 2, b), Qt::white);
    p.fillRect(QRect(b * 4, b, b, b), Qt::black);

    return pix;
}

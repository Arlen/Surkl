/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "buttons.hpp"
#include "core/SessionManager.hpp"
#include "theme.hpp"

#include <QPainter>
#include <QResizeEvent>


using namespace gui::window;

CloseButton::CloseButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void CloseButton::paintEvent(QPaintEvent * /*event*/)
{
    const auto* tm = core::SessionManager::tm();
    const auto fg  = tm->sceneLightColor();
    const auto bg  = tm->sceneMidarkColor();

    const QRectF rec = rect();
    const qreal s1 = qMin(rec.width(), rec.height());
    const qreal s2 = s1 * 0.15;

    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRect(rec);
    p.setRenderHint(QPainter::Antialiasing);

    p.translate(rec.center());
    p.rotate(40 + (isDown() ? 10 : 0));
    p.setBrush(fg);
    p.drawRect(QRectF(-s1 * 0.5, -s2 * 0.5, s1, s2));
    p.drawRect(QRectF(-s2 * 0.5, -s1 * 0.5, s2, s1));
}


MenuButton::MenuButton(QWidget* parent)
    : QPushButton(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

// This is reimplemented so that it is possible to shrink the widget down to
// QSize(0,0), just like, say, CloseButton.  Something about QPushButton that
// doesn't by default allow the size to shrink to QSize(0,0), but this fixed
// it.
QSize MenuButton::sizeHint() const
{
    return QSize(0, 0);
}

void MenuButton::paintEvent(QPaintEvent* /*event*/)
{
    const auto* tm = core::SessionManager::tm();
    const auto fg  = tm->sceneLightColor();
    const auto bg  = tm->sceneMidarkColor();

    const qreal size   = qMin(rect().width(), rect().height());
    const qreal height = size * 0.1;
    const qreal radius = height * 0.5;
    const qreal dy     = height * 2;
    const qreal ratio  = isDown() ? 1 : 0.6;
    const QPointF cent(QRectF(rect()).center());
    const QRectF rec(-size * ratio * 0.5, 0, size * ratio, height);

    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRect(rect());
    p.setRenderHint(QPainter::Antialiasing);

    p.translate(cent.x(), cent.y() - height * 2.5);
    p.setBrush(fg);
    p.drawRoundedRect(rec, radius, radius);
    p.translate(0, dy);
    p.drawRoundedRect(rec, radius, radius);
    p.translate(0, dy);
    p.drawRoundedRect(rec, radius, radius);
}


SplitButton::SplitButton(QWidget *parent)
    : QAbstractButton(parent)
      , _flip(false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    _updateTimer.setInterval(32);
    connect(&_updateTimer, &QTimer::timeout, [this]() {
        /*std::cout << "UPing" << std::endl;*/
        update();
    });

    _flipTimer.setInterval(250);
    connect(&_flipTimer, &QTimer::timeout, [this]() { _flip = !_flip; });
}

void SplitButton::paintEvent(QPaintEvent * /*event*/)
{
    const auto* tm = core::SessionManager::tm();
    const auto fg  = tm->sceneLightColor();
    const auto bg  = tm->sceneMidarkColor();

    const qreal size = qMin(rect().width(), rect().height());
    const qreal ratio = 0.15;
    const QRectF line(-size * ratio * 0.5, -size * 0.5, size * ratio, size);

    QPainter p(this);
    p.fillRect(rect(), bg);
    p.translate(QRectF(rect()).center());
    p.setRenderHint(QPainter::Antialiasing);

    if (_updateTimer.isActive()) {
        // draw square
        p.fillRect(QRectF(-size * 0.25, -size * 0.25, size * 0.5, size * 0.5), fg);
        // flip the line
        p.rotate(_flip ? 90 : 0);

        // draw line shadow
        p.save();
        p.scale(1.5, 1.5);
        p.fillRect(line, bg);
        p.restore();

        // then draw the real one
        p.fillRect(line, fg);
    } else {
        p.fillRect(QRectF(-size * 0.25, -size * 0.25, size * 0.5, size * 0.5), fg);
        p.fillRect(QRectF(-size * 0.15, -size * 0.15, size * 0.3, size * 0.3), bg);
        p.rotate(45);
        p.fillRect(line, fg);
    }
}

void SplitButton::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        event->ignore();
    } else {
        QAbstractButton::mouseMoveEvent(event);
    }
}

void SplitButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _updateTimer.start();
        _flipTimer.start();
    }
    QAbstractButton::mousePressEvent(event);
}

void SplitButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && _updateTimer.isActive()) {
        const auto remaining = _flipTimer.remainingTime();
        _flipTimer.stop();

        QTimer::singleShot(remaining, [this]() {
            _flip = !_flip;
        });

        QTimer::singleShot(remaining + 250, [this]() {
            _flip = !_flip;
            update();
            _updateTimer.stop();
            _flip = false;
        });
    }

    event->ignore();
}

void SplitButton::keyPressEvent(QKeyEvent *event)
{
    event->setAccepted(false);
}


TitleButton::TitleButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setCheckable(true);
}

void TitleButton::paintEvent(QPaintEvent * /*event*/)
{
    const auto* tm = core::SessionManager::tm();
    const auto fg  = tm->sceneLightColor();
    const auto bg  = tm->sceneMidarkColor();
    const auto mg  = tm->sceneColor();

    constexpr qreal border = 1;
    const QRectF rec = rect();

    QPainter p(this);
    p.setPen(Qt::transparent);
    p.setBrush(bg);
    p.drawRect(rec);

    p.setBrush(mg);
    p.drawRect(QRectF(rec.topLeft() + QPointF(0, border),
                      QSizeF(border, rec.height() - border * 2)));
    p.drawRect(QRectF(rec.topRight() + QPointF(-border, border),
                      QSizeF(border, rec.height() - border * 2)));

    p.setPen(fg);
    QFontMetrics fmt(font());
    p.drawText(rec, Qt::AlignCenter, fmt.elidedText(text(), Qt::ElideRight, rec.width()));
}

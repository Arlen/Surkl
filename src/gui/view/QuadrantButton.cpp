#include "QuadrantButton.hpp"

#include <QMouseEvent>
#include <QPainter>


using namespace gui::view;

QuadrantButton::QuadrantButton(QWidget* parent)
    : QPushButton(parent)
{
    setFixedSize(64, 64);
    setMouseTracking(true);

    const auto rec = rect();
    _q2 = QRect(rec.topLeft(), rec.center());
    _q4 = QRect(rec.center(), rec.bottomRight());
    _q3 = QRect(_q2.bottomLeft(), _q4.bottomLeft());
    _q1 = QRect(_q2.topRight(), _q4.topRight());
    _qc = QRect(_q2.center(), _q4.center());
}

void QuadrantButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);

    p.setPen(Qt::black);
    p.setBrush(QColor(67, 67, 67, 128));
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    p.setPen(QColor(128, 128, 128, 255));
    p.drawLine(_q2.topRight(), _q4.bottomLeft());
    p.drawLine(_q2.bottomLeft(), _q4.topRight());

    p.setRenderHint(QPainter::Antialiasing);
    p.drawEllipse(_qc);


    p.drawText(_q1.adjusted(6, 0, 0, -6), Qt::AlignCenter, "I");
    p.drawText(_q2.adjusted(0, 0, -6, -6), Qt::AlignCenter, "II");
    p.drawText(_q3.adjusted(0, 6, -6, 0), Qt::AlignCenter, "III");
    p.drawText(_q4.adjusted(6, 6, 0, 0), Qt::AlignCenter, "IV");
    p.drawText(_qc, Qt::AlignCenter, "C");

    if (_mousePos.isNull()) {
        return;
    }

    p.setBrush(QColor(128, 128, 128, 128));
    p.setPen(QColor(160, 160, 160, 255));

    if (_qc.contains(_mousePos)) {
        p.drawEllipse(_qc);
        p.drawText(_qc, Qt::AlignCenter, "C");
    } else {
        if (_q1.contains(_mousePos)) {
            p.drawRect(_q1);
            p.drawText(_q1.adjusted(6, 0, 0, -6), Qt::AlignCenter, "I");
        } else if (_q2.contains(_mousePos)) {
            p.drawRect(_q2);
            p.drawText(_q2.adjusted(0, 0, -6, -6), Qt::AlignCenter, "II");
        } else if (_q3.contains(_mousePos)) {
            p.drawRect(_q3);
            p.drawText(_q3.adjusted(0, 6, -6, 0), Qt::AlignCenter, "III");
        } else if (_q4.contains(_mousePos)) {
            p.drawRect(_q4);
            p.drawText(_q4.adjusted(6, 6, 0, 0), Qt::AlignCenter, "IV");
        }
        p.setBrush(QColor(67, 67, 67, 128));
        p.setPen(QColor(128, 128, 128, 255));
        p.drawEllipse(_qc);
        p.drawText(_qc, Qt::AlignCenter, "C");
    }
}

void QuadrantButton::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);

    if (_qc.contains(_mousePos)) {
        emit centerPressed();
    } else if (_q1.contains(_mousePos)) {
        emit quad1Pressed();
    } else if (_q2.contains(_mousePos)) {
        emit quad2Pressed();
    } else if (_q3.contains(_mousePos)) {
        emit quad3Pressed();
    } else if (_q4.contains(_mousePos)) {
        emit quad4Pressed();
    }
}

void QuadrantButton::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);

    _mousePos = event->pos();
    update();
}

void QuadrantButton::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);

    _mousePos = QPoint();
    update();
}
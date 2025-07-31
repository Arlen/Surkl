/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later


#include "DeleteDialog.hpp"
#include "SessionManager.hpp"
#include "theme/theme.hpp"

#include <QKeyEvent>
#include <QPainter>
#include <QTimeLine>


using namespace core;

DeletionDialog::DeletionDialog(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint)
{
    setFixedSize(512, 32*3);

    _tl = new QTimeLine(2000, this);
    _tl->setEasingCurve(QEasingCurve::Linear);

    connect(_tl, &QTimeLine::valueChanged, this, [this] {
        update();
    });

    connect(_tl, &QTimeLine::finished, this, &QDialog::accept);
    connect(this, &QDialog::finished, this, &QDialog::deleteLater);

    _tl->start();
}

void DeletionDialog::keyReleaseEvent(QKeyEvent *event)
{
    QDialog::keyReleaseEvent(event);

    if (!event->isAutoRepeat() && _tl->state() == QTimeLine::Running && event->key() == Qt::Key_Delete) {
        _tl->stop();
        reject();
    }
}

void DeletionDialog::paintEvent(QPaintEvent *event)
{
    const auto* tm = SessionManager::tm();
    const auto rec = rect();

    QPainter p(this);
    auto ft = font();
    ft.setPointSize(14);
    p.setFont(ft);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(tm->sceneLightColor(), 2));
    p.setBrush(tm->sceneMidarkColor());
    p.drawRect(rec);
    p.drawText(rec.adjusted(0, 8, 0, 0), tr("Delete"), Qt::AlignHCenter | Qt::AlignTop);

    const auto bar = QRect(0, 32, _tl->currentValue() * rec.width(), 32);
    const auto remaining = (_tl->duration() - _tl->currentTime()) / 1000.0;

    if (_tl->state() == QTimeLine::Running) {
        const auto opt = QTextOption(Qt::AlignHCenter);
        p.fillRect(bar, tm->sceneLightColor());
        p.setBrush(Qt::NoBrush);
        p.setPen(tm->sceneLightColor());
        p.setCompositionMode(QPainter::CompositionMode_Exclusion);
        p.drawText(bar, QString::number(remaining, remaining <= 1.0 ? 'f' : 'g', 1), opt);
    }
}


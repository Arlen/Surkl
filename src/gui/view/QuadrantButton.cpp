/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "QuadrantButton.hpp"
#include "core/SessionManager.hpp"
#include "theme/theme.hpp"

#include <QGridLayout>
#include <QPainter>
#include <QStyleOption>


using namespace gui::view;

TriggerButton::TriggerButton(const QString& text, Qt::Alignment alignment, QWidget* parent)
    : QPushButton(text, parent)
    , _alignment(alignment)
{
    setFocusPolicy(Qt::FocusPolicy::NoFocus);
}

void TriggerButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    QStyleOptionButton option;
    initStyleOption(&option);

    auto* tm = core::SessionManager::tm();
    const auto color0 = tm->sceneMidarkColor();
    const auto color1 = tm->sceneColor();
    const auto color2 = tm->sceneMidlightColor();
    const auto color3 = tm->sceneLightColor();

    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(color1);
    p.setBrush(color2);

    if (option.state & QStyle::State_MouseOver) {
        p.setBrush(color3);
    }
    if (option.state & QStyle::State_Sunken) {
        p.setBrush(color1);
    }

    p.drawRoundedRect(option.rect, 4, 4);

    if (option.state & QStyle::State_Sunken) {
        p.setPen(color3);
    } else {
        p.setPen(color0);
    }

    const auto x1 = _alignment & Qt::AlignLeft   ?  4 : 0;
    const auto y1 = _alignment & Qt::AlignTop    ?  2 : 0;
    const auto x2 = _alignment & Qt::AlignRight  ? -4 : 0;
    const auto y2 = _alignment & Qt::AlignBottom ? -2 : 0;

    p.drawText(option.rect.adjusted(x1, y1, x2, y2), _alignment, option.text);
}


QuadrantButton::QuadrantButton(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(64, 64);

    auto* q1 = new TriggerButton("1", Qt::AlignRight | Qt::AlignTop, this);
    auto* q2 = new TriggerButton("2", Qt::AlignLeft  | Qt::AlignTop, this);
    auto* q3 = new TriggerButton("3", Qt::AlignLeft  | Qt::AlignBottom, this);
    auto* q4 = new TriggerButton("4", Qt::AlignRight | Qt::AlignBottom, this);
    auto* qc = new TriggerButton("5", Qt::AlignCenter, this);

    q1->setFixedSize(32, 32);
    q2->setFixedSize(32, 32);
    q3->setFixedSize(32, 32);
    q4->setFixedSize(32, 32);
    qc->setFixedSize(32, 32);

    auto* layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(q1, 0, 1);
    layout->addWidget(q2, 0, 0);
    layout->addWidget(q3, 1, 0);
    layout->addWidget(q4, 1, 1);
    qc->move(16, 16);

    connect(q1, &QPushButton::clicked, this, &QuadrantButton::quad1Pressed);
    connect(q2, &QPushButton::clicked, this, &QuadrantButton::quad2Pressed);
    connect(q3, &QPushButton::clicked, this, &QuadrantButton::quad3Pressed);
    connect(q4, &QPushButton::clicked, this, &QuadrantButton::quad4Pressed);
    connect(qc, &QPushButton::clicked, this, &QuadrantButton::centerPressed);
}

/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "QuadrantButton.hpp"

#include <QGridLayout>


using namespace gui::view;

QuadrantButton::QuadrantButton(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(64, 64);

    auto* q1 = new QPushButton("  1", this);
    auto* q2 = new QPushButton("2  ", this);
    auto* q3 = new QPushButton("3  ", this);
    auto* q4 = new QPushButton("  4", this);
    auto* qc = new QPushButton("5", this);

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

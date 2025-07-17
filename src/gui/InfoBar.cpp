/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "InfoBar.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>


using namespace gui;

InfoBar::InfoBar(QWidget *parent)
    : QFrame(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* lineEdit = new QLineEdit(this);
    lineEdit->setFrame(false);
    lineEdit->setReadOnly(true);
    lineEdit->setTextMargins(4, 0, 4, 0);
    layout->addWidget(lineEdit);

    auto* button = new QPushButton(this);
    button->setFixedWidth(16);
    layout->addWidget(button);

    setMaximumHeight(32);
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Plain);

    connect(button, &QPushButton::pressed, this, &QWidget::hide);
    connect(button, &QPushButton::pressed, this, &InfoBar::hidden);

    connect(this, &InfoBar::cleared, lineEdit, &QLineEdit::clear);

    connect(this, &InfoBar::rightMsgPosted, lineEdit,
        [lineEdit](const QString& text) {
            lineEdit->setAlignment(Qt::AlignRight);
            lineEdit->setText(text);
    });

    connect(this, &InfoBar::leftMsgPosted, lineEdit,
        [lineEdit](const QString& text) {
            lineEdit->setAlignment(Qt::AlignLeft);
            lineEdit->setText(text);
    });


    _timer = new QTimer(this);
    _timer->setSingleShot(true);
    connect(_timer, &QTimer::timeout, lineEdit, &QLineEdit::clear);
}

void InfoBar::clear()
{
    emit cleared();
}

void InfoBar::setMsgR(const QString& text)
{
    emit rightMsgPosted(text);
}

void InfoBar::setTimedMsgL(const QString& text, int lifetime)
{
    _timer->start(lifetime);

    emit leftMsgPosted(text);
}
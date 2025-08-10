/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "InfoBar.hpp"
#include "core/SessionManager.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>


using namespace gui;

InfoBarController::InfoBarController(QObject* parent)
    : QObject(parent)
{
    _timer = new QTimer(this);
    _timer->setSingleShot(true);
    connect(_timer, &QTimer::timeout, this, &InfoBarController::cleared);
}

void InfoBarController::clear()
{
    emit cleared();
}

void InfoBarController::postMsgR(const QString& text)
{
    emit rightMsgPosted(text);
}

void InfoBarController::postMsgL(const QString& text, int lifetime)
{
    emit leftMsgPosted(text);

    if (lifetime > 0) {
        _timer->start(lifetime);
    }
}



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

    const auto* controller = core::SessionManager::ib();

    connect(controller, &InfoBarController::cleared, lineEdit, &QLineEdit::clear);

    connect(controller, &InfoBarController::rightMsgPosted, lineEdit,
        [lineEdit](const QString& text) {
            lineEdit->setAlignment(Qt::AlignRight);
            lineEdit->setText(text);
    });

    connect(controller, &InfoBarController::leftMsgPosted, lineEdit,
        [lineEdit](const QString& text) {
            lineEdit->setAlignment(Qt::AlignLeft);
            lineEdit->setText(text);
    });
}
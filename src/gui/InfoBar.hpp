/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>


class QTimer;

namespace gui
{
    class InfoBarController final : public QObject
    {
        Q_OBJECT

    signals:
        void cleared();
        void rightMsgPosted(const QString& text);
        void leftMsgPosted(const QString& text);

    public:
        explicit InfoBarController(QObject* parent = nullptr);
        void clear();
        void postMsgR(const QString& text);
        void postMsgL(const QString& text, int lifetime = -1);

    private:
        QTimer* _timer{nullptr};
    };


    class InfoBar final : public QFrame
    {
        Q_OBJECT

    signals:
        void hidden();

    public:
        explicit InfoBar(QWidget* parent = nullptr);
    };
}

/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>


class QTimer;

namespace gui
{
    class InfoBar final : public QFrame
    {
        Q_OBJECT

    signals:
        void hidden();
        void cleared();
        void rightMsgPosted(const QString& text);
        void leftMsgPosted(const QString& text);

    public:
        explicit InfoBar(QWidget* parent = nullptr);

        void clear();
        void setMsgR(const QString& text);
        void setTimedMsgL(const QString& text, int lifetime);

    private:
        QTimer* _timer{nullptr};
    };
}

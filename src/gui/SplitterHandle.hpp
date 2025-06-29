/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSplitterHandle>


namespace gui
{
    class SplitterHandle final : public QSplitterHandle
    {
        Q_OBJECT
    public:
        explicit SplitterHandle(Qt::Orientation ori, QSplitter *parent = nullptr);

    protected:
        void paintEvent(QPaintEvent *event) override;
    };
}

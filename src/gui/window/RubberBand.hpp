/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QRubberBand>


namespace gui::window
{
    /// RubberBand is only used when splitting a gui::Window by click-dragging
    /// the SplitButton of the window.  The rubberband used when resizing the
    /// window by moving the SplitterHandle is a QRubberBand that has a custom
    /// paint (drawControl) implemented in gui::SurklStyle.
    class RubberBand final : public QRubberBand
    {
    public:
        explicit RubberBand(QWidget *parent = nullptr);

    protected:
        void paintEvent(QPaintEvent *event) override;
    };
}

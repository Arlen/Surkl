/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QProxyStyle>


namespace gui
{
    /// SurklStyle is used to customize the QRubberBand used by SplitterHandle,
    /// so that it looks exactly like RubberBand used by the SplitterButton
    /// when performing a splitting of a Window.  SplitterHandle is only used
    /// to resize the windows.

    /// Why not use a gui::RubberBand for gui::Splitter/gui::SplitterHandle?
    /// Because, internally, QSplitter creates a QRubberBand
    /// (see QSplitter::setRubberBand).  Also, trying to use gui::RubberBand by
    /// reimplementing everything as Qt does, did not produce correct results.
    /// The reason being that, gui::RubberBand needs to be parented to
    /// gui::Splitter, but that causes issues because it's not possible to do
    /// 'QBoolBlocker b(d->blockChildAdd);' as QSplitter does.
    class SurklStyle final : public QProxyStyle
    {
    public:
        void drawControl(QStyle::ControlElement element,
                         const QStyleOption *opt,
                         QPainter *p,
                         const QWidget *widget) const override;
    };
}

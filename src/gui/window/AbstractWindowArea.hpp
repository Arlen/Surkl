/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>


namespace gui::window
{
    class Window;
    class TitleBar;

    class AbstractWindowArea : public QWidget
    {
        Q_OBJECT

    public:
        explicit AbstractWindowArea(Window *parent);

        virtual void setTitleBar(TitleBar *titleBar);

    public slots:
        void switchToView();

        void switchToThemeSettings();

        void moveToNewMainWindow();

    protected:
        TitleBar *titleBar() const;

        TitleBar *_titleBar{nullptr};
    };
}

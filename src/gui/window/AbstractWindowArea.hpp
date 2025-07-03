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
        enum AreaType
        {
            InvalidArea = -1,
            ViewArea = 0,
            ThemeArea = 1,
            /// others...
        };

        explicit AbstractWindowArea(Window *parent);

        virtual void setTitleBar(TitleBar *titleBar);

        AreaType areaType() const { return _areaType; }

    public slots:
        void switchToView();

        void switchToThemeSettings();

        void moveToNewMainWindow();

    protected:
        TitleBar *titleBar() const;

        TitleBar *_titleBar{nullptr};
        AreaType _areaType{InvalidArea};
    };
}

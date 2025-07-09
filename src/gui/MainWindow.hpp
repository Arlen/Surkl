/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "WidgetId.hpp"

#include <QWidget>


namespace gui
{
    class Splitter;

    namespace window
    {
        class Window;
    }


    class MainWindow final : public QWidget, public WidgetId
    {
        Q_OBJECT

    signals:
        void closed(qint32 id);

        void stateChanged(const MainWindow*);

    public:
        explicit MainWindow();

        void moveToNewMainWindow(window::Window* source);

        [[nodiscard]] Splitter* splitter() const { return _splitter; }

    protected:
        void closeEvent(QCloseEvent* event) override;

    private slots:
        void closeSibling(qint32 id);

    private:
        void updateTitle();

        Splitter* _splitter{nullptr};
    };
}
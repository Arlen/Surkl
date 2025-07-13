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
        MainWindow();

        explicit MainWindow(Splitter* splitter);

        [[nodiscard]] static MainWindow* first();

        static MainWindow* loadUi();

        void moveToNewMainWindow(window::Window* source);

        [[nodiscard]] Splitter* splitter() const { return _splitter; }

    protected:
        void closeEvent(QCloseEvent* event) override;

        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void deleteFromDb(qint32 idOfMainWindow);

    private:
        void setTitle();

        Splitter* _splitter{nullptr};
    };
}
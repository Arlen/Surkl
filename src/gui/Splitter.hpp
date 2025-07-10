/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "WidgetId.hpp"

#include <QSplitter>


namespace gui
{
    namespace window
    {
        class Window;
    }

    class RubberBand;


    class Splitter final : public QSplitter, public WidgetId
    {
        Q_OBJECT

    signals:
        void stateChanged(const Splitter*);

    public:
        Splitter() = delete;

        Splitter(const Splitter &) = delete;

        explicit Splitter(Qt::Orientation orientation, QWidget *parent = nullptr);

        void addWindow(window::Window *window);

        void insertWindow(int index, window::Window *window);

        void addWindow();

        Splitter* addSplitter();

        int row();

    public slots:
        void splitWindow(const QPoint& pos, Qt::Orientation splitOrientation, window::Window *child);

        void deleteChild(window::Window *child);

        static void swap(window::Window *winA, window::Window *winB);

    protected:
        QSplitterHandle *createHandle() override;

        void resizeEvent(QResizeEvent *event) override;

    private:
        [[nodiscard]] window::Window *createWindow() const;

        void connectWindowToThisSplitter(const window::Window *child) const;

        void takeOwnershipOf(window::Window *orphanWindow, int childSplitterIndex);

        void takeOwnershipOf(Splitter *orphanSplitter, int childSplitterIndex);
    };
}

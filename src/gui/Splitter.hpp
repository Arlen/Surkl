/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSplitter>


namespace gui
{
    namespace window
    {
        class Window;
    }

    class RubberBand;


    class Splitter final : public QSplitter
    {
        Q_OBJECT

    public:
        Splitter() = delete;

        Splitter(const Splitter &) = delete;

        explicit Splitter(Qt::Orientation orientation, QWidget *parent = nullptr);

        explicit Splitter(qint32 id, Qt::Orientation orientation, QWidget *parent = nullptr);

        void addWindow(window::Window *window);

        void insertWindow(int index, window::Window *window);

        [[nodiscard]] qint32 id() const { return _id; }

        int row();

        Splitter* splitWindowAsSplitter(const QPoint& pos, Qt::Orientation splitOrientation, window::Window *child
            , qint32 newSplitterId);

    public slots:
        void splitWindow(const QPoint& pos, Qt::Orientation splitOrientation, window::Window *child);

        void deleteChild(window::Window *child);

        static void swap(window::Window *winA, window::Window *winB);

    protected:
        QSplitterHandle *createHandle() override;

    private:
        [[nodiscard]] window::Window *createEmptyWindow() const;

        void connectWindowToThisSplitter(const window::Window *child) const;

        void takeOwnershipOf(window::Window *orphanWindow, int childSplitterIndex);

        void takeOwnershipOf(Splitter *orphanSplitter, int childSplitterIndex);

        /// the UI id used in persistent UI
        qint32 _id;
    };
}

/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPushButton>
#include <QTimer>


namespace gui::window
{
    class CloseButton final : public QAbstractButton
    {
    public:
        explicit CloseButton(QWidget *parent = nullptr);

    protected:
        void paintEvent(QPaintEvent *event) override;
    };


    class MenuButton final : public QPushButton
    {
    public:
        explicit MenuButton(QWidget* parent = nullptr);
        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
    };


    class SplitButton final : public QAbstractButton
    {
    public:
        explicit SplitButton(QWidget *parent = nullptr);

    protected:
        void paintEvent(QPaintEvent *event) override;

        void mouseMoveEvent(QMouseEvent *event) override;

        void mousePressEvent(QMouseEvent *event) override;

        void mouseReleaseEvent(QMouseEvent *event) override;

        void keyPressEvent(QKeyEvent *event) override;

    private:
        QTimer _updateTimer;
        QTimer _flipTimer;
        bool _flip;
    };


    class TitleButton final : public QAbstractButton
    {
    public:
        explicit TitleButton(QWidget *parent = nullptr);

    protected:
        void paintEvent(QPaintEvent *event) override;
    };
}


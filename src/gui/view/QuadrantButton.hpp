/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPushButton>


namespace gui::view
{
    class TriggerButton final : public QPushButton
    {
    public:
        explicit TriggerButton(const QString& text, Qt::Alignment alignment, QWidget* parent = nullptr);

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        Qt::Alignment _alignment;
    };


    class QuadrantButton final : public QWidget
    {
        Q_OBJECT

    signals:
        void quad1Pressed();
        void quad2Pressed();
        void quad3Pressed();
        void quad4Pressed();
        void centerPressed();

    public:
        explicit QuadrantButton(QWidget *parent = nullptr);
    };
}
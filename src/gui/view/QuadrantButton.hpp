#pragma once

#include <QPushButton>


namespace gui::view
{
    class QuadrantButton : public QPushButton
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

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;

        QRect _q1;
        QRect _q2;
        QRect _q3;
        QRect _q4;
        QRect _qc;
        QPoint _mousePos;
    };
}
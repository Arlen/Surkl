/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "RubberBand.hpp"
#include "WidgetId.hpp"

#include <QPen>
#include <QPointer>
#include <QWidget>


class QVBoxLayout;

namespace gui::window
{
    class AbstractWindowArea;
    class Overlay;
    class TitleBar;

    struct RubberBandState
    {
        RubberBandState() = default;

        explicit RubberBandState(QWidget *parent);

        Qt::Orientation currOrientation;
        QPoint currentPos;
        QRect currGeom;
        QString savedTitle;
        qreal xDirLoad;

        QPointer<RubberBand> p;
    };


    class Window final : public QWidget, public WidgetId
    {
        Q_OBJECT

    signals:
        void closed(Window *window);

        bool splitWindowRequested(QPoint pos, Qt::Orientation ori, Window *child);

        void swapRequested(Window *alpha, Window *beta);

        void stateChanged(const Window*);

    public:
        explicit Window(QWidget *parent = nullptr);

        [[nodiscard]] TitleBar *titleBar() const;

        [[nodiscard]] AbstractWindowArea *areaWidget() const;

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;

        void dragMoveEvent(QDragMoveEvent *event) override;

        void dragLeaveEvent(QDragLeaveEvent *event) override;

        void dropEvent(QDropEvent *event) override;

        void mouseMoveEvent(QMouseEvent *event) override;

        void mouseReleaseEvent(QMouseEvent *event) override;

        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void activateSplitMode();

        void activateSwapMode();

        void closeWindow();

    public slots:
        void switchToView();

        void switchToThemeSettings();

        void moveToNewMainWindow();

    private:
        void initTitlebar(QVBoxLayout *layout);

        void setAreaWidget(AbstractWindowArea *widget);

        void setupMenu();

        [[nodiscard]] int splitterHandleWidth() const;

        AbstractWindowArea *_areaWidget{nullptr};
        TitleBar *_titleBar{nullptr};
        Overlay *_overlay{nullptr};

        RubberBandState _rbs;
    };


    class Overlay final : public QWidget
    {
    public:
        enum Movement
        {
            Origin,
            Destination,
        };

        explicit Overlay(Movement movement, QWidget *parent = nullptr);

        [[nodiscard]] bool isOrigin() const { return _movement == Movement::Origin; }

    protected:
        void paintEvent(QPaintEvent *event) override;


    private:
        Movement _movement;
        QPen _pen1;
        QPen _pen2;
    };


    QPixmap WindowDragPixmap();
}

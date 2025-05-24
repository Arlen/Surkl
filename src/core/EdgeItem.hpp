/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QGraphicsLineItem>


namespace  core
{
    class EdgeLabelItem final : public QGraphicsSimpleTextItem
    {
    public:
        explicit EdgeLabelItem(QGraphicsItem* parent = nullptr);
        void alignToAxis(const QLineF& line);
        void alignToAxis(const QLineF& line, const QString& newText);
        void updatePos();

    private:
        QLineF _normal;
        QLineF _axis;
        QString _text;
    };


    class EdgeItem final : public QGraphicsLineItem
    {
    public:
        enum { Type = UserType + 1 };
        enum State { ActiveState, CollapsedState/*, InactiveState*/ };

        explicit EdgeItem(QGraphicsItem* source = nullptr, QGraphicsItem* target = nullptr);
        void setText(const QString& text) const;
        void adjust();
        void setState(State state);
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        QPainterPath shape() const override;

        [[nodiscard]] State state() const           { return _state; }
        [[nodiscard]] QGraphicsItem *source() const { return _source; }
        [[nodiscard]] QGraphicsItem *target() const { return _target; }
        [[nodiscard]] EdgeLabelItem* label() const  { return _label; }
        [[nodiscard]] QLineF lineWithMargin() const { return _lineWithMargin; }
        [[nodiscard]] int type() const override     { return Type; }

    private:
        State _state{ActiveState};
        QLineF _lineWithMargin;
        QGraphicsItem* _source{nullptr};
        QGraphicsItem* _target{nullptr};
        EdgeLabelItem* _label{nullptr};
    };
}
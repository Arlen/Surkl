/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QGraphicsObject>

#include <ranges>
#include <unordered_set>


class QTimeLine;

namespace core
{
    class SceneButton : public QGraphicsObject
    {
        Q_OBJECT

    signals:
        void pressed();

    public:
        enum { Type = UserType + 100 };

        explicit SceneButton(const QPointF& pos);
        ~SceneButton() override;
        [[nodiscard]] int type() const override { return Type; }

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void timerEvent(QTimerEvent *event) override;

        QRectF boundingRect() const override;
        QPainterPath shape() const override;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        virtual SceneButton* clone(const QPointF& pos) const = 0;
        virtual void deleteMe() = 0;

        static const std::unordered_set<SceneButton*>& buttons()
        {
            return _buttons;
        }

        template <class T>
        auto siblings() const
        {
            return buttons() | std::views::filter([](SceneButton* but) {
                return qgraphicsitem_cast<T*>(but) != nullptr;
            });
        }

    private:
        int _deleteTimerId{0};
        QTimeLine* _timeline{nullptr};
        inline static std::unordered_set<SceneButton*> _buttons;
    };


    class AboutButton final : public SceneButton
    {
        Q_OBJECT

    public:
        enum { Type = UserType + 101 };

        explicit AboutButton(const QPointF& pos);
        [[nodiscard]] int type() const override { return Type; }

    protected:
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        SceneButton* clone(const QPointF& pos) const override;
        void deleteMe() override;
    };


    class ThemeButton final : public SceneButton
    {
        Q_OBJECT

    public:
        enum { Type = UserType + 102 };

        explicit ThemeButton(const QPointF& pos);
        [[nodiscard]] int type() const override { return Type; }

    protected:
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        SceneButton* clone(const QPointF& pos) const override;
        void deleteMe() override;
    };
}
#pragma once

#include <QObject>
#include <QColor>


namespace gui
{
    namespace builtin
    {
        constexpr QColor bgColor() { return QColor(67, 67, 67, 255); }
    };


    class ThemeManager : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(QColor bgColor MEMBER _bg_color NOTIFY bgColorChanged)

    public slots:
        void reset() noexcept;

    signals:
        void bgColorChanged(const QColor& color);

    private:
        QColor _bg_color = builtin::bgColor();
    };
}


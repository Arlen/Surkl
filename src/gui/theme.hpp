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
        Q_PROPERTY(QColor bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)

    public:
        static constexpr auto TABLE_NAME        = QLatin1StringView("Theme");
        static constexpr auto PROPERTY_NAME_COL = QLatin1StringView("property_name");
        static constexpr auto COLOR_VALUE_COL   = QLatin1StringView("color_value");

        static void configure(ThemeManager* tm);
        static void saveToDatabase(ThemeManager* tm);

        ThemeManager(QObject* parent = nullptr);

        QColor bgColor() const { return _bg_color; }

    public slots:
        void setBgColor(const QColor& color) noexcept;
        void reset() noexcept;

    signals:
        void bgColorChanged(QColor color);
        void resetTriggered();

    private:
        static void saveToDatabase(QLatin1StringView name, const QColor& color);

        QColor _bg_color = builtin::bgColor();

    private slots:
        void saveBgColor() const;
    };
}


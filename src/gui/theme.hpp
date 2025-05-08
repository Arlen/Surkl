/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>

#include <array>
#include <unordered_map>


class QStandardItemModel;

namespace gui
{
    enum PaletteIndex
    {
        SCENE_BG_COLOR = 0,
        SCENE_FG_COLOR,
        NODE_OPEN_COLOR,
        NODE_OPEN_BORDER_COLOR,
        NODE_CLOSED_COLOR,
        NODE_CLOSED_BORDER_COLOR,
        EDGE_COLOR,
        EDGE_TEXT_COLOR,

        PaletteIndexSize
    };

    using PaletteId   = std::string;
    using PaletteName = std::string;
    using Palette     = std::array<QColor, PaletteIndexSize>;
    using Palettes    = std::unordered_map<PaletteId, PaletteName>;
    using Colors      = std::unordered_map<PaletteId, Palette>;


    class ThemeManager final : public QObject
    {
        Q_OBJECT

        static constexpr auto PALETTES_TABLE = QLatin1StringView("Palettes");
        static constexpr auto COLORS_TABLE   = QLatin1StringView("Colors");
        static constexpr auto PALETTE_ID     = QLatin1StringView("palette_id");
        static constexpr auto PALETTE_NAME   = QLatin1StringView("name");
        static constexpr auto COLOR_POSITION = QLatin1StringView("position");
        static constexpr auto COLOR_VALUE    = QLatin1StringView("value");

        static constexpr auto THEME_SETTINGS_TABLE = QLatin1StringView("ThemeSettings");
        static constexpr auto ATTRIBUTE_KEY        = QLatin1StringView("attr_key");
        static constexpr auto ATTRIBUTE_VALUE      = QLatin1StringView("attr_value");
        static constexpr auto ACTIVE_THEME_KEY     = QLatin1StringView("active_theme");

    public:
        enum ModelColumn
        {
            NameColumn = 0,
            PreviewColumn,
            ApplyColumn,
            DiscardColum,
            PaletteIdColumn,

            ModelColumnCount
        };

        static constexpr Palette factory()
        {
            Palette result;

            result[SCENE_BG_COLOR]           = {  67,  67,  67, 255 };
            result[SCENE_FG_COLOR]           = {  67,  67,  67, 255 };
            result[NODE_OPEN_COLOR]          = { 148, 148, 148, 255 };
            result[NODE_OPEN_BORDER_COLOR]   = { 220, 220, 220, 255 };
            result[NODE_CLOSED_COLOR]        = { 164, 164, 164, 255 };
            result[NODE_CLOSED_BORDER_COLOR] = {   8,   8,   8, 255 };
            result[EDGE_COLOR]               = {   8,   8,   8, 255 };
            result[EDGE_TEXT_COLOR]          = { 220, 220, 220, 255 };

            return result;
        }

        static void configure(ThemeManager *tm);

        explicit ThemeManager(QObject *parent = nullptr);

        void keep(const Palette &palette);

        static Palette paletteFromId(const PaletteId &id);

        static PaletteId idFromPalette(const Palette &palette);

        bool isFactory(const PaletteId& id) const
            { return id == _factoryId; }
        bool isActive(const PaletteId& id) const
            { return id == idFromPalette(_active); }
        const QColor& sceneBgColor() const
            { return _active[SCENE_BG_COLOR]; }
        const QColor& sceneFgColor() const
            { return _active[SCENE_FG_COLOR]; }
        const QColor& openNodeColor() const
            { return _active[NODE_OPEN_COLOR]; }
        const QColor& closedNodeColor() const
            { return _active[NODE_CLOSED_COLOR]; }
        const QColor& openNodeBorderColor() const
            { return _active[NODE_OPEN_BORDER_COLOR]; }
        const QColor& closedNodeBorderColor() const
            { return _active[NODE_CLOSED_BORDER_COLOR]; }
        const QColor& edgeColor() const
            { return _active[EDGE_COLOR]; }
        const QColor& edgeTextColor() const
            { return _active[EDGE_TEXT_COLOR]; }

        QStandardItemModel* model() const { return _model; }

    public slots:
        void setActivePalette(const Palette &palette);

        void switchTo(const PaletteId &id);

        void discard(const PaletteId &id);

    signals:
        void themeChanged();

    private:
        PaletteId addPalette(const Palette &palette, const PaletteName &name = "???");

        void removePalette(const PaletteId &id);

        void setName(const PaletteId &id, const PaletteName &name);

        void addToModel(const PaletteId &id);

        void removeFromModel(const PaletteId &id) const;

        static void createTables();

        void savePalettes(std::ranges::input_range auto &&rg);

        static void deletePalettes(std::ranges::input_range auto &&rg);

        static void saveActiveTheme(const std::string &id);

        static QString getActiveTheme();


        Palettes _palettes;
        Colors _colors;
        Palette _active;
        const PaletteId _factoryId{idFromPalette(factory())};

        QStandardItemModel* _model{nullptr};
    };
}


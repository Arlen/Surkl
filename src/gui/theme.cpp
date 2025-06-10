/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "gui/theme.hpp"
#include "core/db.hpp"

#include <QSqlRecord>
#include <QStandardItemModel>
#include <QRandomGenerator>

#include <ranges>
#include <set>


using namespace gui;

namespace
{
    template <std::size_t ... ints>
    void sortByGroups(Palette& palette, std::index_sequence<ints...>)
    {
        auto cmp = [](const QColor& a, const QColor& b)
            { return a.value() < b.value(); };

        std::array xs = {std::get<ints>(std::forward<Palette>(palette))... };
        std::ranges::sort(xs, cmp);

        for (int xi = 0; auto pi : {ints...}) {
            palette[pi] = xs[xi++];
        }
    };
}

void ThemeManager::configure(ThemeManager* tm)
{
    using namespace std::ranges;

    createTables();
    Q_ASSERT(core::db::doesTableExists(PALETTES_TABLE));
    Q_ASSERT(core::db::doesTableExists(COLORS_TABLE));

    const auto db = core::db::get();
    if (!db.isOpen()) {
        return;
    }

    QSqlQuery q(db);
    Palettes palettes;
    Colors colors;
    auto ok = false;

    ok = q.prepare(QLatin1StringView("SELECT %1,%2 FROM %3")
            .arg(PALETTE_ID)
            .arg(PALETTE_NAME)
            .arg(PALETTES_TABLE));
    Q_ASSERT(ok);

    if (q.exec()) {
        const auto rec       = q.record();
        const auto idIndex   = rec.indexOf(PALETTE_ID);
        const auto nameIndex = rec.indexOf(PALETTE_NAME);

        while (q.next())
        {
            const auto id   = q.value(idIndex).toString().toStdString();
            const auto name = q.value(nameIndex).toString().toStdString();
            palettes[id]    = name;
        }
    }

    ok = q.prepare(QLatin1StringView("SELECT %1,%2,%3 FROM %4")
            .arg(PALETTE_ID)
            .arg(COLOR_POSITION)
            .arg(COLOR_VALUE)
            .arg(COLORS_TABLE));
    Q_ASSERT(ok);

    if (q.exec()) {
        const auto rec      = q.record();
        const auto idIndex  = rec.indexOf(PALETTE_ID);
        const auto posIndex = rec.indexOf(COLOR_POSITION);
        const auto valIndex = rec.indexOf(COLOR_VALUE);

        while (q.next()) {
            const auto id    = q.value(idIndex).toString().toStdString();
            const auto pos   = q.value(posIndex).toInt(&ok);
            const auto value = q.value(valIndex).value<QColor>();
            const auto posi  = qBound(0, pos, static_cast<int>(PaletteIndexSize));

            Q_ASSERT(ok);
            Q_ASSERT(pos >= 0 && pos < PaletteIndexSize);
            Q_ASSERT(palettes.contains(id));

            colors[id][posi] = value;
        }
    }

    Q_ASSERT(tm->_palettes.size() == 1);
    Q_ASSERT(tm->_colors.size()   == 1);
    Q_ASSERT(palettes.size() == colors.size());
    Q_ASSERT(
        std::ranges::equal(
            views::keys(palettes) | std::ranges::to<std::set>(),
            views::keys(colors) | std::ranges::to<std::set>()));

    for (const auto& [id, name] : palettes) {
        if (colors.contains(id)) {
            tm->addPalette(colors[id], name);
        }
    }
    if (const auto active = tm->getActiveTheme().toStdString();
        palettes.contains(active) && colors.contains(active)) {
        tm->_active = colors[active];
    }
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    _model = new QStandardItemModel(0, ModelColumnCount, this);
    _model->setHeaderData(NameColumn,    Qt::Horizontal, tr("Name"));
    _model->setHeaderData(PreviewColumn, Qt::Horizontal, tr("Preview"));
    _model->setHeaderData(ApplyColumn,   Qt::Horizontal, tr("Yes"));
    _model->setHeaderData(DiscardColum,  Qt::Horizontal, tr("No"));

    connect(_model, &QStandardItemModel::itemChanged, [this](const QStandardItem* item) {
        /// palette name changed, update raw data.
        if (item->column() == NameColumn) {
            const auto nameIndex = item->index();
            Q_ASSERT(nameIndex.isValid());

            const auto name = nameIndex.data();
            Q_ASSERT(name.isValid());

            const auto paletteIdIndex = nameIndex.siblingAtColumn(PaletteIdColumn);
            Q_ASSERT(paletteIdIndex.isValid());

            const auto paletteId = paletteIdIndex.data();
            Q_ASSERT(paletteId.isValid());

            setName(paletteId.toString().toStdString(), name.toString().toStdString());
        }
    });

    addPalette(factory(), "Monochrom");
    _active = factory();
}


Palette ThemeManager::generatePalette(const HsvRange& range)
{
    auto isBounded = [](qreal min, qreal x, qreal max) -> bool {
        return min <= x && x <= max;
    };

    Q_ASSERT(0 <= range.hue.p1 && range.hue.p2 <= 360.0);
    Q_ASSERT(0 <= range.sat.p1 && range.sat.p2 <= 1.0);
    Q_ASSERT(0 <= range.val.p1 && range.val.p2 <= 1.0);

    const auto hueP1 = range.hue.p1/360.0;
    const auto hueP2 = range.hue.p2/360.0;
    const auto satP1 = range.sat.p1;
    const auto satP2 = range.sat.p2;
    const auto valP1 = range.val.p1;
    const auto valP2 = range.val.p2;


    auto hueDelta = hueP2 - hueP1;
    auto satDelta = satP2 - satP1;
    auto valDelta = valP2 - valP1;

    if (hueP2 < hueP1) { hueDelta = 1.0 - std::fabs(hueP2 - hueP1); }
    if (satP2 < satP1) { satDelta = 1.0 - std::fabs(satP2 - satP1); }
    if (valP2 < valP1) { valDelta = 1.0 - std::fabs(valP2 - valP1); }

    Palette result;

    for (int i = 0; i < PaletteIndexSize; ++i) {
        const auto lds = _golden.next();

        auto hue = std::lerp(hueP1, hueP1 + hueDelta, lds[0]);
        if (hue >= 1.0) { hue -= 1.0; }
        auto sat = std::lerp(satP1, satP1 + satDelta, lds[1]);
        if (sat >= 1.0) { sat -= 1.0; }
        auto val = std::lerp(valP1, valP1 + valDelta, lds[2]);
        if (val >= 1.0) { val -= 1.0; }

        Q_ASSERT(isBounded(hueP1, hue, hueP2) || isBounded(hueP1, hue, 1.0) || isBounded(0, hue, hueP2));
        Q_ASSERT(isBounded(satP1, sat, satP2) || isBounded(satP1, sat, 1.0) || isBounded(0, sat, satP2));
        Q_ASSERT(isBounded(valP1, val, valP2) || isBounded(valP1, val, 1.0) || isBounded(0, val, valP2));

        result[i] = QColor::fromHsvF(hue, sat, val).rgba();
    }

    sortByGroups(result, std::index_sequence<NODE_CLOSED_MIDLIGHT_COLOR, NODE_CLOSED_COLOR, NODE_CLOSED_MIDARK_COLOR, NODE_CLOSED_DARK_COLOR>{});
    sortByGroups(result, std::index_sequence<NODE_OPEN_LIGHT_COLOR, NODE_OPEN_MIDLIGHT_COLOR, NODE_OPEN_COLOR>{});
    sortByGroups(result, std::index_sequence<NODE_FILE_LIGHT_COLOR, NODE_FILE_MIDLIGHT_COLOR, NODE_FILE_COLOR>{});
    sortByGroups(result, std::index_sequence<EDGE_LIGHT_COLOR, EDGE_COLOR>{});

    return result;
}

void ThemeManager::keep(const Palette &palette)
{
    setActivePalette(palette);

    savePalettes(std::ranges::single_view{addPalette(palette)});
}

Palette ThemeManager::paletteFromId(const PaletteId& id)
{
    const auto bytes = QByteArray::fromBase64(QString::fromStdString(id).toUtf8());

    Palette result; result.fill(QColor(0, 0, 0, 0));

    /// J += 9 b/c HexArgb is '#AARRGGBB' long.
    for (int i = 0, j = 0; i < result.size() && j < bytes.size()-8; ++i, j += 9) {
        result[i] = QColor::fromString(bytes.sliced(j, 9));
    }

    return result;
}

PaletteId ThemeManager::idFromPalette(const Palette& palette)
{
    QByteArray bytes;

    for (const auto& color : palette) {
        bytes += color.name(QColor::HexArgb).toLatin1();
    }

    return bytes.toBase64().toStdString();
}

void ThemeManager::setActivePalette(const Palette& palette)
{
    _active = palette;

    emit themeChanged();
}

void ThemeManager::switchTo(const PaletteId& id)
{
    if (const auto found = _colors.find(id); found != _colors.end()) {
        saveActiveTheme(found->first);
        setActivePalette(found->second);
    }
}

void ThemeManager::discard(const PaletteId& id)
{
    /// never discard Monochrom.
    if (!isFactory(id)) {
        removePalette(id);

        /// if discarding an active theme, then switch to factory default.
        if (isActive(id)) {
            setActivePalette(factory());
        }
    }
}

PaletteId ThemeManager::addPalette(const Palette& palette, const std::string &name)
{
    const auto id = idFromPalette(palette);

    /// just a debug check; in reality it doesn't matter b/c the id is unique.
    if (_palettes.contains(id)) { qWarning() << "Palette already exists!"; }

    _palettes[id] = name;
    _colors[id]   = palette;

    addToModel(id);

    return id;
}

void ThemeManager::removePalette(const PaletteId& id)
{
    deletePalettes(std::ranges::single_view{id});
    removeFromModel(id);
    _palettes.erase(id);
    _colors.erase(id);
}

void ThemeManager::setName(const PaletteId& id, const PaletteName& name)
{
    if (const auto found = _palettes.find(id); found != _palettes.end()) {
        found->second = name;
        savePalettes(std::ranges::single_view{id});
    }
}

void ThemeManager::addToModel(const PaletteId& id)
{
    Q_ASSERT(_palettes.contains(id));
    Q_ASSERT(_colors.contains(id));

    const auto& name = _palettes[id];

    auto* nameItem      = new QStandardItem(QString::fromStdString(name));
    auto* previewItem   = new QStandardItem();
    auto* applyItem     = new QStandardItem();
    auto* discardItem   = new QStandardItem();
    auto* paletteIdItem = new QStandardItem();

    previewItem->setEnabled(false);

    if (id == idFromPalette(factory())) {
        discardItem->setEnabled(false);
    }

    paletteIdItem->setText(QString::fromStdString(id));

    _model->appendRow({nameItem, previewItem, applyItem, discardItem, paletteIdItem});
}

void ThemeManager::removeFromModel(const PaletteId& id) const
{
    for (const auto toBeRemoved = _model->findItems
            ( QString::fromStdString(id)
            , Qt::MatchExactly
            , PaletteIdColumn
            ); const auto item : toBeRemoved) {
        _model->removeRow(item->row());
    }
}

void ThemeManager::createTables()
{
    const auto db = core::db::get();

    if (!db.isOpen()) { return; }

    QSqlQuery q(db);

    /// CREATE TABLE Palettes (id TEXT, name TEXT)
    q.exec(
        QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 TEXT NOT NULL PRIMARY KEY
                            , %3 TEXT NOT NULL))")
            .arg(PALETTES_TABLE)
            .arg(PALETTE_ID)
            .arg(PALETTE_NAME));

    /// CREATE TABLE Colors (palette_id TEXT, position INTEGER, value INTEGER)
    q.exec(
        QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 TEXT NOT NULL
                            , %3 INTEGER NOT NULL
                            , %4 INTEGER NOT NULL))")
            .arg(COLORS_TABLE)
            .arg(PALETTE_ID)
            .arg(COLOR_POSITION)
            .arg(COLOR_VALUE));

    /// CREATE TABLE ThemeSettings (attr_key TEXT, attr_value TEXT)
    q.exec(
        QLatin1StringView(R"(CREATE TABLE IF NOT EXISTS %1
                            ( %2 TEXT PRIMARY KEY
                            , %3 TEXT NOT NULL))")
            .arg(THEME_SETTINGS_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE));
}

/// takes a range of PaletteIds.
void ThemeManager::savePalettes(std::ranges::input_range auto&& rg)
{
    if (auto db = core::db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery q1(db);
        QSqlQuery q2(db);

        q1.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2,%3) VALUES(?, ?)")
            .arg(PALETTES_TABLE)
            .arg(PALETTE_ID)
            .arg(PALETTE_NAME));

        q2.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2,%3,%4) VALUES(?, ?, ?)")
            .arg(COLORS_TABLE)
            .arg(PALETTE_ID)
            .arg(COLOR_POSITION)
            .arg(COLOR_VALUE));

        for (const auto& id : rg) {
            Q_ASSERT(_palettes.contains(id));
            if (auto found = _palettes.find(id); found != _palettes.end()) {
                q1.addBindValue(QString::fromStdString(found->first));  // id
                q1.addBindValue(QString::fromStdString(found->second)); // name
                if (!q1.exec()) {
                    qWarning() << q1.lastError();
                }
            }
            Q_ASSERT(_colors.contains(id));
            if (auto found = _colors.find(id); found != _colors.end()) {
                const auto& palette = found->second;
                for (int pos = 0; pos < PaletteIndexSize; ++pos) {
                    q2.addBindValue(QString::fromStdString(id));
                    q2.addBindValue(pos);
                    q2.addBindValue(palette[pos]);
                    if (!q2.exec()) {
                        qWarning() << q2.lastError();
                    }
                }
            }
        }

        if (!db.commit()) {
            qWarning() << db.lastError();
        }
    }
}

/// takes a range of PaletteIds.
void ThemeManager::deletePalettes(std::ranges::input_range auto&& rg)
{
    if (auto db = core::db::get(); db.isOpen()) {
        db.transaction();
        QSqlQuery q1(db);
        QSqlQuery q2(db);

        q1.prepare(QLatin1StringView("DELETE FROM %1 WHERE %2=:key")
            .arg(PALETTES_TABLE)
            .arg(PALETTE_ID));

        q2.prepare(QLatin1StringView("DELETE FROM %1 WHERE %2=:key")
            .arg(COLORS_TABLE)
            .arg(PALETTE_ID));

        for (const auto& id : rg) {
            q1.bindValue(":key", QString::fromStdString(id));
            q2.bindValue(":key", QString::fromStdString(id));
            const auto ok1 = q1.exec();
            const auto ok2 = q2.exec();
            Q_ASSERT(ok1 && ok2);
        }

        if (!db.commit()) {
            qWarning() << db.lastError();
        }
    }
}

void ThemeManager::saveActiveTheme(const std::string& id)
{
    if (const auto db = core::db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(QLatin1StringView("INSERT OR REPLACE INTO %1 (%2, %3) VALUES(?, ?)")
            .arg(THEME_SETTINGS_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE));

        q.addBindValue(ACTIVE_THEME_KEY);
        q.addBindValue(QString::fromStdString(id));

        if (!q.exec()) {
            qWarning()
                << "ThemeManager: failed to save active theme ("
                << q.lastError()
                << ")";
        }
    }
}

QString ThemeManager::getActiveTheme()
{
    QString active;

    if (const auto db = core::db::get(); db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(QLatin1StringView("SELECT %2 FROM %3 WHERE %1=:key")
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE)
            .arg(THEME_SETTINGS_TABLE));

        q.bindValue(":key", ACTIVE_THEME_KEY);

        if (q.exec()) {
            const auto rec = q.record();
            const auto val = rec.indexOf(ATTRIBUTE_VALUE);
            if (q.next()) {
                active = q.value(val).toString();
            }
        }
    }

    return active;
}
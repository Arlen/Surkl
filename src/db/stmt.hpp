/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLatin1StringView>


using namespace Qt::Literals::StringLiterals;

namespace stmt::bm
{
    constexpr auto TABLE_NAME     = "SceneBookmarks"_L1;
    constexpr auto POSITION_X_COL = "position_x"_L1;
    constexpr auto POSITION_Y_COL = "position_y"_L1;
    constexpr auto NAME_COL       = "name"_L1;

    constexpr auto CREATE_TABLE = R"(CREATE TABLE IF NOT EXISTS %1
                                     ( %2 INTEGER NOT NULL
                                     , %3 INTEGER NOT NULL
                                     , %4 TEXT NOT NULL
                                     , UNIQUE(%2, %3) )
                                    )"_L1;

    constexpr auto SELECT_PALETTE  = "SELECT * FROM %1"_L1;
    constexpr auto INSERT_SCENE_BM = "INSERT OR REPLACE INTO %1 ( %2, %3, %4 ) VALUES ( ?, ?, ? )"_L1;
    constexpr auto DELETE_SCENE_BM = "DELETE FROM %1 WHERE %2=? AND %3=?"_L1;


    static const auto CREATE_SCENE_BOOKMARKS_TABLE
        = CREATE_TABLE.arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL)
            .arg(NAME_COL);

    static const auto SELECT_ALL_PALETTES
        = SELECT_PALETTE.arg(TABLE_NAME);

    static const auto INSERT_BM
        = INSERT_SCENE_BM.arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL)
            .arg(NAME_COL);

    static const auto DELETE_BM
        = DELETE_SCENE_BM.arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL);
}


namespace stmt::scene
{
    constexpr auto NODES_TABLE = "Nodes"_L1;
    constexpr auto NODE_ID     = "node_id"_L1;
    constexpr auto NODE_TYPE   = "node_type"_L1;
    constexpr auto FIRST_ROW   = "first_row"_L1; // row number of the first child node.
    constexpr auto NODE_XPOS   = "node_xpos"_L1;
    constexpr auto NODE_YPOS   = "node_ypos"_L1;
    constexpr auto EDGE_LEN    = "edge_len"_L1;

    constexpr auto CREATE_TABLE = R"(CREATE TABLE IF NOT EXISTS %1
                                     ( %2 TEXT PRIMARY KEY
                                     , %3 INTEGER
                                     , %4 INTEGER
                                     , %5 REAL
                                     , %6 REAL
                                     , %7 REAL)
                                    )"_L1;
    constexpr auto SELECT_NODE = "SELECT * FROM %1"_L1;
    constexpr auto INSERT_NODE = "INSERT OR REPLACE INTO %1 ( %2, %3, %4, %5, %6, %7 ) VALUES ( ?, ?, ?, ?, ?, ? )"_L1;
    constexpr auto DELETE_NODE = "DELETE FROM %1 WHERE %2=:id"_L1;
}

namespace stmt::theme
{
    constexpr auto PALETTES_TABLE = "Palettes"_L1;
    constexpr auto COLORS_TABLE   = "Colors"_L1;
    constexpr auto PALETTE_ID     = "palette_id"_L1;
    constexpr auto PALETTE_NAME   = "name"_L1;
    constexpr auto COLOR_POSITION = "position"_L1;
    constexpr auto COLOR_VALUE    = "value"_L1;

    constexpr auto THEME_SETTINGS_TABLE = "ThemeSettings"_L1;
    constexpr auto ATTRIBUTE_KEY        = "attr_key"_L1;
    constexpr auto ATTRIBUTE_VALUE      = "attr_value"_L1;
    constexpr auto ACTIVE_THEME_KEY     = "active_theme"_L1;

    constexpr auto CREATE_PALETTES_TABLE = R"(CREATE TABLE IF NOT EXISTS %1
                                              ( %2 TEXT NOT NULL PRIMARY KEY
                                              , %3 TEXT NOT NULL )
                                             )"_L1;
    constexpr auto CREATE_COLORS_TABLE = R"(CREATE TABLE IF NOT EXISTS %1
                                            ( %2 TEXT NOT NULL
                                            , %3 INTEGER NOT NULL
                                            , %4 INTEGER NOT NULL )
                                           )"_L1;

    constexpr auto CREATE_SETTINGS_TABLE = R"(CREATE TABLE IF NOT EXISTS %1
                                              ( %2 TEXT PRIMARY KEY
                                              , %3 TEXT NOT NULL )
                                             )"_L1;


    constexpr auto INSERT_PALETTE   = "INSERT OR REPLACE INTO %1 ( %2, %3 ) VALUES( ?, ? )"_L1;
    constexpr auto INSERT_COLOR     = "INSERT OR REPLACE INTO %1 ( %2, %3, %4 ) VALUES( ?, ?, ? )"_L1;
    constexpr auto INSERT_ATTRIBUTE = "INSERT OR REPLACE INTO %1 ( %2, %3 ) VALUES( ?, ? )"_L1;
    constexpr auto SELECT_ATTRIBUTE = "SELECT %2 FROM %3 WHERE %1=:key"_L1;
    constexpr auto DELETE_PALETTE   = "DELETE FROM %1 WHERE %2=:key"_L1;
    constexpr auto DELETE_COLOR     = "DELETE FROM %1 WHERE %2=:key"_L1;
}
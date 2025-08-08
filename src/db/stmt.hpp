/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLatin1StringView>


using namespace Qt::Literals::StringLiterals;

/// used in core/bookmark.cpp
namespace stmt::bm
{
    constexpr auto TABLE_NAME     = "SceneBookmarks"_L1;
    constexpr auto POSITION_X_COL = "position_x"_L1;
    constexpr auto POSITION_Y_COL = "position_y"_L1;
    constexpr auto NAME_COL       = "name"_L1;

    constexpr auto CREATE_TABLE_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 INTEGER NOT NULL
             , %3 INTEGER NOT NULL
             , %4 TEXT NOT NULL
             , UNIQUE(%2, %3) )
            )"_L1;

    constexpr auto SELECT_TPL = "SELECT * FROM %1"_L1;
    constexpr auto INSERT_TPL = "INSERT OR REPLACE INTO %1 ( %2, %3, %4 ) VALUES ( ?, ?, ? )"_L1;
    constexpr auto DELETE_TPL = "DELETE FROM %1 WHERE %2=? AND %3=?"_L1;


    static const auto CREATE_SCENE_BOOKMARKS_TABLE
        = CREATE_TABLE_TPL.arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL)
            .arg(NAME_COL);

    static const auto SELECT_ALL_PALETTES
        = SELECT_TPL.arg(TABLE_NAME);

    static const auto INSERT_BM
        = INSERT_TPL.arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL)
            .arg(NAME_COL);

    static const auto DELETE_BM
        = DELETE_TPL.arg(TABLE_NAME)
            .arg(POSITION_X_COL)
            .arg(POSITION_Y_COL);
}

/// used in core/SceneStorage.cpp
namespace stmt::scene
{
    constexpr auto NODES_TABLE = "Nodes"_L1;
    constexpr auto NODE_ID     = "node_id"_L1;
    constexpr auto NODE_TYPE   = "type"_L1;
    constexpr auto NODE_POS_X  = "pos_x"_L1;
    constexpr auto NODE_POS_Y  = "pos_y"_L1;
    constexpr auto EDGE_LEN    = "edge_len"_L1;

    constexpr auto NODES_DIR_ATTR_TABLE = "NodesDirAttributes"_L1;
    constexpr auto FIRST_ROW            = "first_row"_L1; // row number of the first child node.
    constexpr auto NODE_ROT             = "rotation"_L1;  // angle of external rotation

    constexpr auto CREATE_TABLE_A_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 TEXT PRIMARY KEY
             , %3 INTEGER
             , %4 REAL
             , %5 REAL
             , %6 REAL)
            )"_L1;

    constexpr auto CREATE_TABLE_B_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 TEXT PRIMARY KEY
             , %3 INTEGER
             , %4 REAL)
            )"_L1;

    constexpr auto SELECT_TPL      = "SELECT * FROM %1"_L1;
    constexpr auto INSERT_A_TPL    = "INSERT OR REPLACE INTO %1 ( %2, %3, %4, %5, %6 ) VALUES ( ?, ?, ?, ?, ? )"_L1;
    constexpr auto INSERT_B_TPL    = "INSERT OR REPLACE INTO %1 ( %2, %3, %4 ) VALUES ( ?, ?, ? )"_L1;
    constexpr auto DELETE_FILE_TPL = "DELETE FROM %1 WHERE %2=?"_L1;
    constexpr auto DELETE_DIR_TPL  = "DELETE FROM %1 WHERE %2 LIKE ? || '%'"_L1;

    static const auto CREATE_NODES_TABLE
        = CREATE_TABLE_A_TPL.arg(NODES_TABLE)
            .arg(NODE_ID)
            .arg(NODE_TYPE)
            .arg(NODE_POS_X)
            .arg(NODE_POS_Y)
            .arg(EDGE_LEN);

    static const auto CREATE_NODES_DIR_ATTR_TABLE
        = CREATE_TABLE_B_TPL.arg(NODES_DIR_ATTR_TABLE)
            .arg(NODE_ID)
            .arg(FIRST_ROW)
            .arg(NODE_ROT);

    static const auto SELECT_ALL_NODES
        = SELECT_TPL.arg(NODES_TABLE);

    static const auto SELECT_ALL_NODES_DIR_ATTRS
        = SELECT_TPL.arg(NODES_DIR_ATTR_TABLE);

    static const auto INSERT_NODE
        = INSERT_A_TPL.arg(NODES_TABLE)
            .arg(NODE_ID)
            .arg(NODE_TYPE)
            .arg(NODE_POS_X)
            .arg(NODE_POS_Y)
            .arg(EDGE_LEN);

    static const auto INSERT_NODE_DIR_ATTR
        = INSERT_B_TPL.arg(NODES_DIR_ATTR_TABLE)
            .arg(NODE_ID)
            .arg(FIRST_ROW)
            .arg(NODE_ROT);

    static const auto DELETE_FILE_NODE
        = DELETE_FILE_TPL.arg(NODES_TABLE)
            .arg(NODE_ID);

    static const auto DELETE_DIR_NODE
        = DELETE_DIR_TPL.arg(NODES_TABLE)
            .arg(NODE_ID);

    static const auto DELETE_NODE_DIR_ATTR
        = DELETE_DIR_TPL.arg(NODES_DIR_ATTR_TABLE)
            .arg(NODE_ID);
}

/// used in gui/theme/theme.cpp
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

    constexpr auto CREATE_PALETTES_TABLE_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 TEXT NOT NULL PRIMARY KEY
             , %3 TEXT NOT NULL )
            )"_L1;

    constexpr auto CREATE_COLORS_TABLE_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 TEXT NOT NULL
             , %3 INTEGER NOT NULL
             , %4 INTEGER NOT NULL )
            )"_L1;

    constexpr auto CREATE_SETTINGS_TABLE_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 TEXT PRIMARY KEY
             , %3 TEXT NOT NULL )
            )"_L1;

    constexpr auto INSERT_PALETTE_TPL   = "INSERT OR REPLACE INTO %1 ( %2, %3 ) VALUES( ?, ? )"_L1;
    constexpr auto INSERT_COLOR_TPL     = "INSERT OR REPLACE INTO %1 ( %2, %3, %4 ) VALUES( ?, ?, ? )"_L1;
    constexpr auto INSERT_ATTRIBUTE_TPL = "INSERT OR REPLACE INTO %1 ( %2, %3 ) VALUES( ?, ? )"_L1;
    constexpr auto SELECT_ATTRIBUTE_TPL = "SELECT %3 FROM %1 WHERE %2=?"_L1;
    constexpr auto SELECT_PALETTES_TPL  = "SELECT %3,%2 FROM %1"_L1;
    constexpr auto SELECT_COLORS_TPL    = "SELECT %2,%3,%4 FROM %1"_L1;
    constexpr auto DELETE_TPL           = "DELETE FROM %1 WHERE %2=?"_L1;

    static const auto CREATE_PALETTES_TABLE
        = CREATE_PALETTES_TABLE_TPL.arg(PALETTES_TABLE)
            .arg(PALETTE_ID)
            .arg(PALETTE_NAME);

    static const auto CREATE_COLORS_TABLE
        = CREATE_COLORS_TABLE_TPL.arg(COLORS_TABLE)
            .arg(PALETTE_ID)
            .arg(COLOR_POSITION)
            .arg(COLOR_VALUE);

    static const auto CREATE_SETTINGS_TABLE
        = CREATE_SETTINGS_TABLE_TPL.arg(THEME_SETTINGS_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE);

    static const auto INSERT_PALETTES
        = INSERT_PALETTE_TPL.arg(PALETTES_TABLE)
            .arg(PALETTE_ID)
            .arg(PALETTE_NAME);

    static const auto INSERT_COLORS
        = INSERT_COLOR_TPL.arg(COLORS_TABLE)
            .arg(PALETTE_ID)
            .arg(COLOR_POSITION)
            .arg(COLOR_VALUE);

    static const auto INSERT_ATTRIBUTE
        = INSERT_ATTRIBUTE_TPL.arg(THEME_SETTINGS_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE);

    static const auto SELECT_ATTRIBUTE
        = SELECT_ATTRIBUTE_TPL.arg(THEME_SETTINGS_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE);

    static const auto SELECT_PALETTES
        = SELECT_PALETTES_TPL.arg(PALETTES_TABLE)
            .arg(PALETTE_NAME)
            .arg(PALETTE_ID);

    static const auto SELECT_COLORS
        = SELECT_COLORS_TPL.arg(COLORS_TABLE)
            .arg(PALETTE_ID)
            .arg(COLOR_POSITION)
            .arg(COLOR_VALUE);

    static const auto DELETE_PALETTES
        = DELETE_TPL.arg(PALETTES_TABLE)
            .arg(PALETTE_ID);

    static const auto DELETE_COLORS
        = DELETE_TPL.arg(COLORS_TABLE)
            .arg(PALETTE_ID);
}
namespace stmt::surkl
{
    constexpr auto SURKL_TABLE     = "Surkl"_L1;
    constexpr auto ATTRIBUTE_KEY   = "attr_key"_L1;
    constexpr auto ATTRIBUTE_VALUE = "attr_value"_L1;


    constexpr auto CREATE_SURKL_TABLE_TPL
        = R"(CREATE TABLE IF NOT EXISTS %1
             ( %2 TEXT NOT NULL PRIMARY KEY
             , %3 )
            )"_L1;

    constexpr auto INSERT_TPL = "INSERT OR REPLACE INTO %1 ( %2, %3 ) VALUES( ?, ? )"_L1;
    constexpr auto SELECT_TPL = "SELECT %3,%2 FROM %1"_L1;


    static const auto CREATE_SURKL_TABLE
        = CREATE_SURKL_TABLE_TPL.arg(SURKL_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE);

    static const auto INSERT_ATTRIBUTE
        = INSERT_TPL.arg(SURKL_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE);

    static const auto SELECT_ATTRIBUTE
        = SELECT_TPL.arg(SURKL_TABLE)
            .arg(ATTRIBUTE_KEY)
            .arg(ATTRIBUTE_VALUE);
}
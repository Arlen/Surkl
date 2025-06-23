/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "theme.hpp"

#include <QWidget>


class QLabel;
class QPushButton;
class QVBoxLayout;
class QTableView;
class QButtonGroup;

namespace gui
{
    class ThemeSettings final : public QWidget
    {
        Q_OBJECT

    signals:
        void generated(const Palette& pal);

    public:
        explicit ThemeSettings(QWidget* parent);

    protected:
        void resizeEvent(QResizeEvent *event) override;
        void hideEvent(QHideEvent *event) override;

    private slots:
        void generatePalette();
        void shuffle();
        void prevPermutation();
        void nextPermutation();

    private:
        template <class Range>
        void buildRangeLineEdits(QVBoxLayout* parentLayout, QString name, qreal min, qreal max, Range& range);

        static QBrush paletteToBrush(const Palette& palette, int w = 128);
        static QPixmap paletteToPixmap(const Palette& palette, QSize sz = QSize());
        void setupItemWidgets(int start, int end);
        void saveLastApplied(int groupId, bool checked);

        QPushButton* _applyGenerated{nullptr};
        QButtonGroup* _group{nullptr};
        QTableView* _tv{nullptr};
        int _lastApplied{0};

        std::array<int, std::tuple_size<Palette>{}> _permutation;
        HsvRange _hsvRange;
        Palette _generated;
    };
}
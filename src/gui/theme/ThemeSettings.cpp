/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "theme/ThemeSettings.hpp"
#include "core/SessionManager.hpp"

#include <QButtonGroup>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include <ranges>


using namespace gui::theme;

ThemeSettings::ThemeSettings(QWidget *parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    _previewLabel = new QLabel(this);
    _previewLabel->setFrameShape(QFrame::Panel);
    _previewLabel->setFrameShadow(QFrame::Sunken);
    _previewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _previewLabel->setMaximumHeight(32);
    connect(this, &ThemeSettings::generated, [this](const Palette& palette) {
        _previewLabel->setPixmap(paletteToPixmap(palette, _previewLabel->size()));
    });
    layout->addWidget(_previewLabel, 1);

    buildRangeLineEdits(layout, "Hue Range", 0, 360, _hsvRange.hue);
    buildRangeLineEdits(layout, "Sat Range", 0, 1,   _hsvRange.sat);
    buildRangeLineEdits(layout, "Val Range", 0, 1,   _hsvRange.val);

    _applyGenerated = new QPushButton(tr("Apply"), this);
    _applyGenerated->setCheckable(true);
    _applyGenerated->hide();
    _applyGenerated->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _applyGenerated->setMaximumHeight(26);
    connect(_applyGenerated, &QPushButton::toggled, [this](bool checked) {
        if (checked) { emit generated(_generated); }
    });

    auto* shuffleButton = new QPushButton(tr("Shuffle"), this);
    shuffleButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    shuffleButton->setMaximumHeight(26);
    connect(shuffleButton, &QPushButton::clicked, this, &ThemeSettings::shuffle);
    shuffleButton->hide();

    auto* prevPermButton = new QPushButton(tr("Prev. Perm."), this);
    prevPermButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    prevPermButton->setMaximumHeight(26);
    connect(prevPermButton, &QPushButton::clicked, this, &ThemeSettings::prevPermutation);
    prevPermButton->hide();

    auto* nextPermButton = new QPushButton(tr("Next Perm."), this);
    nextPermButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    nextPermButton->setMaximumHeight(26);
    connect(nextPermButton, &QPushButton::clicked, this, &ThemeSettings::nextPermutation);
    nextPermButton->hide();

    auto* keepButton = new QPushButton(tr("Keep"), this);
    keepButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    keepButton->setMaximumHeight(26);
    keepButton->hide();
    connect(keepButton, &QPushButton::clicked, [this, keepButton] {

        Palette result;
        /// apply permutation, if any.
        for (auto [i, pi] : std::ranges::views::enumerate(_permutation)) {
            result[i] = _generated[pi];
        }
        core::SessionManager::tm()->keep(result);
        _previewLabel->setPixmap(QPixmap());
        keepButton->hide();
        _applyGenerated->hide();
    });
    connect(keepButton, &QPushButton::clicked, shuffleButton, &QWidget::hide);
    connect(keepButton, &QPushButton::clicked, prevPermButton, &QWidget::hide);
    connect(keepButton, &QPushButton::clicked, nextPermButton, &QWidget::hide);

    auto* generateButton = new QPushButton(tr("Generate"), this);
    generateButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    generateButton->setMaximumHeight(26);
    connect(generateButton, &QPushButton::clicked, this, &ThemeSettings::generatePalette);
    connect(generateButton, &QPushButton::clicked, shuffleButton, &QWidget::show);
    connect(generateButton, &QPushButton::clicked, prevPermButton, &QWidget::show);
    connect(generateButton, &QPushButton::clicked, nextPermButton, &QWidget::show);
    connect(generateButton, &QPushButton::clicked, keepButton, &QWidget::show);
    connect(generateButton, &QPushButton::clicked, _previewLabel, &QWidget::show);

    auto* hl = new QHBoxLayout();
    hl->addWidget(_applyGenerated, 1);
    hl->addSpacerItem(new QSpacerItem(1, 0, QSizePolicy::MinimumExpanding));
    hl->addWidget(shuffleButton, 1);
    hl->addWidget(prevPermButton, 1);
    hl->addWidget(nextPermButton, 1);
    hl->addWidget(keepButton, 1);
    hl->addWidget(generateButton, 1);
    layout->addLayout(hl);

    _group = new QButtonGroup(this);
    _group->setExclusive(true);
    _group->addButton(_applyGenerated);

    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    _tv = new QTableView(this);
    _tv->setModel(core::SessionManager::tm()->model());
    _tv->verticalHeader()->setVisible(false);
    _tv->setColumnHidden(ThemeManager::PaletteIdColumn, true);
    _tv->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _tv->horizontalHeader()->setSectionResizeMode(ThemeManager::PreviewColumn, QHeaderView::Stretch);
    _tv->setSelectionMode(QAbstractItemView::NoSelection);
    _tv->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    layout->addWidget(_tv, 0);

    connect(_group, &QButtonGroup::idToggled, this, &ThemeSettings::saveLastApplied);

    connect(this, &ThemeSettings::generated,
        core::SessionManager::tm(),
        qOverload<const Palette&>(&ThemeManager::setActivePalette));

    connect(_tv->model(), &QStandardItemModel::rowsInserted, [this](const QModelIndex&, int start, int end) {
        setupItemWidgets(start, end);
        _tv->resizeColumnToContents(ThemeManager::DiscardColum);
    });

    connect(_tv->model(), &QStandardItemModel::rowsAboutToBeRemoved, [this](const QModelIndex&, int start, int) {
        auto* tm = core::SessionManager::tm();
        const auto* model = qobject_cast<QStandardItemModel*>(_tv->model());
        Q_ASSERT(model);
        const auto removedIndex = model->index(start, ThemeManager::PaletteIdColumn, QModelIndex());

        if (const auto removedId = removedIndex.data().toString().toStdString(); tm->isActive(removedId)) {
            /// factory becomes active, if an active theme is discarded, so
            /// set the factory apply button to checked.

            const auto foundRow = model->findItems(QString::fromStdString(tm->idFromPalette(tm->factory()))
                , Qt::MatchExactly, ThemeManager::PaletteIdColumn);
            Q_ASSERT(foundRow.size() == 1);
            for (const auto* item : foundRow) {
                const auto applyIndex = model->index(item->row(), ThemeManager::ApplyColumn);
                auto* applyWidget = _tv->indexWidget(applyIndex);

                if (auto* applyButton = qobject_cast<QPushButton*>(applyWidget); applyButton) {
                    applyButton->setChecked(true);
                }
                break;
            }
        }
    });

     QTimer::singleShot(32, [this] {
         /// TODO: the first time _tv is shown, the Preview column does not have
         /// correct preview size.  This is the only fix I've tried that works.
         /// Might need a custom delegate to render the Preview column.
         setupItemWidgets(0, _tv->model()->rowCount() - 1);
     });
}

void ThemeSettings::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    setupItemWidgets(0, _tv->model()->rowCount() - 1);

    if (_applyGenerated->isVisible()) {
        _previewLabel->setPixmap(paletteToPixmap(_generated, _previewLabel->size()));
    }
}

void ThemeSettings::hideEvent(QHideEvent* event)
{
    /// if _applyGenerated is checked, it means the user has generated a palette
    /// but hasn't kept it; attempt to restore back to an applied palette.
    if (_applyGenerated->isChecked()) {
        if (auto* button = qobject_cast<QPushButton*>( _group->button(_lastApplied)); button) {
            Q_ASSERT(button != _applyGenerated);
            button->click();
        }
        /// a palette was discarded after being applied, so try to find the
        /// factory default palette:
        /// -1 is reserved by Qt
        /// -2 is _applyGenerated
        /// -3 should be the factory palette.
        else if (button = qobject_cast<QPushButton*>( _group->button(-3)); button) {
            button->click();
        }
    }

    QWidget::hideEvent(event);
}

void ThemeSettings::generatePalette()
{
    for (auto x : std::views::iota(0, std::ssize(_permutation))) {
        _permutation[x] = x;
    }

    _generated = core::SessionManager::tm()->generatePalette(_hsvRange);

    if (_applyGenerated->isChecked()) {
        emit generated(_generated);
    } else {
        _applyGenerated->show();
        _applyGenerated->toggle();
    }
}

void ThemeSettings::shuffle()
{
    std::ranges::shuffle(_permutation, *QRandomGenerator::global());
    Palette result;

    for (auto [i, pi] : std::ranges::views::enumerate(_permutation)) {
        result[i] = _generated[pi];
    }

    emit generated(result);
}

void ThemeSettings::prevPermutation()
{
    std::ranges::prev_permutation(_permutation);
    Palette result;

    for (auto [i, pi] : std::ranges::views::enumerate(_permutation)) {
        result[i] = _generated[pi];
    }

    emit generated(result);
}

void ThemeSettings::nextPermutation()
{
    std::ranges::next_permutation(_permutation);
    Palette result;

    for (auto [i, pi] : std::ranges::views::enumerate(_permutation)) {
        result[i] = _generated[pi];
    }

    emit generated(result);
}

template <class Range>
void ThemeSettings::buildRangeLineEdits(QVBoxLayout* parentLayout, QString name, qreal min, qreal max, Range& range)
{
    Q_ASSERT(min < max);

    auto* layout = new QHBoxLayout();

    auto* label = new QLabel(name, this);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    label->setMaximumHeight(26);
    layout->addWidget(label, 1);

    auto* p1 = new QLineEdit(this);
    p1->setText(QString::number(min, 'g', 4));
    p1->setClearButtonEnabled(true);
    p1->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    p1->setMaximumHeight(26);

    auto* p2 = new QLineEdit(this);
    p2->setText(QString::number(max, 'g', 4));
    p2->setClearButtonEnabled(true);
    p2->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    p2->setMaximumHeight(26);

    auto* validator = new QDoubleValidator(min, max, 4, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
    p1->setValidator(validator);
    p2->setValidator(validator);
    layout->addWidget(p1, 2);
    layout->addWidget(p2, 2);

    parentLayout->addLayout(layout);

    connect(p1, &QLineEdit::textEdited, [p1, validator, &range] {
        bool ok;
        if (auto value = p1->text().toDouble(&ok); ok) {
            if (value < validator->bottom()) {
                value = validator->bottom();
                p1->setText(QString::number(value, 'g', 4));
            } else if (value > validator->top()) {
                value = validator->top();
                p1->setText(QString::number(value, 'g', 4));
            }
            range.p1 = value;
        }
    });

    connect(p2, &QLineEdit::textEdited, [p2, validator, &range] {
        bool ok;
        if (auto value = p2->text().toDouble(&ok); ok) {
            if (value < validator->bottom()) {
                value = validator->bottom();
                p2->setText(QString::number(value, 'g', 4));
            } else if (value > validator->top()) {
                value = validator->top();
                p2->setText(QString::number(value, 'g', 4));
            }
            range.p2 = value;
        }
    });
}

QBrush ThemeSettings::paletteToBrush(const Palette &palette, int w)
{
    const qreal stride = 1.0 / qreal(PaletteIndexSize);

    QLinearGradient gradient(QPointF(0, 0), QPointF(w, 0));
    gradient.setCoordinateMode(QGradient::LogicalMode);

    qreal pos = 0.0;
    for (int i = 0; i < PaletteIndexSize; ++i) {
        gradient.setColorAt(pos, palette[i]);
        gradient.setColorAt(pos + stride - 0.001, palette[i]);
        pos += stride;
    }

    gradient.setColorAt(1.0, QColor(0, 0, 0, 0));

    return gradient;
}

QPixmap ThemeSettings::paletteToPixmap(const Palette& palette, QSize sz)
{
    const auto brush = paletteToBrush(palette, sz.width());

    auto image = QImage(sz, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter p(&image);
    p.fillRect(image.rect(), brush);

    return QPixmap::fromImage(image);
}

void ThemeSettings::setupItemWidgets(int start, int end)
{
    const auto* model = _tv->model();
    auto* tm = core::SessionManager::tm();

    for (int i = start; i <= end; ++i) {
        const auto idIndex   = model->index(i, ThemeManager::PaletteIdColumn, QModelIndex());
        const auto paletteId = idIndex.data().toString().toStdString();
        const auto& palette  = tm->paletteFromId(paletteId);

        /// create or update the preview widget
        const auto previewIndex = model->index(i, ThemeManager::PreviewColumn);
        QLabel* previewWidget = nullptr;
        if (auto* widget = _tv->indexWidget(previewIndex); widget) {
            previewWidget = qobject_cast<QLabel*>(widget);
            Q_ASSERT(previewWidget);
        } else {
            previewWidget = new QLabel();
            previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            previewWidget->setFrameShape(QFrame::Panel);
            previewWidget->setFrameShadow(QFrame::Plain);
            _tv->setIndexWidget(previewIndex, previewWidget);
        }
        previewWidget->setPixmap(paletteToPixmap(palette, previewWidget->size()));

        /// create the Apply widget, if needed.
        const auto applyIndex = model->index(i, ThemeManager::ApplyColumn);
        if (const auto* widget = _tv->indexWidget(applyIndex); widget == nullptr) {
            auto* applyButton = new QPushButton(tr("Apply"));

            applyButton->setCheckable(true);
            _group->addButton(applyButton);
            if (tm->isActive(paletteId)) { applyButton->setChecked(true); }
            _tv->setIndexWidget(applyIndex, applyButton);

            connect(applyButton, &QPushButton::toggled, [tm, paletteId](bool checked) {
                if (checked) {
                    tm->switchTo(paletteId);
                }
            });
        }

        /// create the Discard widget, if needed.
        const auto discardIndex = model->index(i, ThemeManager::DiscardColum);
        if (const auto* widget = _tv->indexWidget(discardIndex); widget == nullptr) {
            if (!tm->isFactory(paletteId)) {
                auto* discardWidget = new QPushButton(tr("Discard"));
                _tv->setIndexWidget(discardIndex, discardWidget);

                connect(discardWidget, &QPushButton::clicked, [tm, paletteId] {
                    tm->discard(paletteId);
                });
            }
        }
    }
}

/// Need to keep track of the last palette that was applied, excluding the
/// generated one. If a user generates a palette but never keeps it, we restore
/// back to the last applied palette (if possible) before exiting.
void ThemeSettings::saveLastApplied(int groupId, bool checked)
{
    if (checked) {
        if (auto* button = qobject_cast<QPushButton*>( _group->button(groupId)); button && button != _applyGenerated) {
            _lastApplied = groupId;
        }
    }
}
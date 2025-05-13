/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "ThemeSettings.hpp"
#include "SessionManager.hpp"

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
#include <QVBoxLayout>



using namespace gui;

ThemeSettings::ThemeSettings(QWidget *parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    auto* previewLabel = new QLabel(this);
    previewLabel->setFixedHeight(32);
    previewLabel->setFrameShape(QFrame::Panel);
    previewLabel->setFrameShadow(QFrame::Sunken);
    connect(this, &ThemeSettings::generated, [previewLabel](const Palette& palette) {
        previewLabel->setPixmap(paletteToPixmap(palette, previewLabel->size()));
    });
    layout->addWidget(previewLabel);

    buildRangeLineEdits(layout, "Hue Range", 0, 360, _hsvRange.hue);
    buildRangeLineEdits(layout, "Sat Range", 0, 1,   _hsvRange.sat);
    buildRangeLineEdits(layout, "Val Range", 0, 1,   _hsvRange.val);

    _applyGenerated = new QPushButton(tr("Apply"), this);
    _applyGenerated->setCheckable(true);
    _applyGenerated->hide();
    connect(_applyGenerated, &QPushButton::toggled, [this](bool checked) {
        if (checked) { emit generated(_generated); }
    });

    auto* keepButton = new QPushButton("Keep", this);
    keepButton->hide();
    connect(keepButton, &QPushButton::clicked, [this, keepButton, previewLabel] {
        core::SessionManager::tm()->keep(_generated);
        previewLabel->setPixmap(QPixmap());
        keepButton->hide();
        _applyGenerated->hide();
    });

    auto* generateButton = new QPushButton("Generate", this);
    connect(generateButton, &QPushButton::clicked, this, &ThemeSettings::generatePalette);
    connect(generateButton, &QPushButton::clicked, keepButton, &QWidget::show);
    connect(generateButton, &QPushButton::clicked, previewLabel, &QWidget::show);

    auto* hl = new QHBoxLayout();
    hl->addWidget(_applyGenerated);
    hl->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    hl->addWidget(keepButton);
    hl->addWidget(generateButton);
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
    layout->addWidget(_tv);

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
}

void ThemeSettings::resizeEvent(QResizeEvent *event)
{
    setupItemWidgets(0, _tv->model()->rowCount() - 1);

    QWidget::resizeEvent(event);
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
    _generated = core::SessionManager::tm()->generatePalette(_hsvRange);

    if (_applyGenerated->isChecked()) {
        emit generated(_generated);
    } else {
        _applyGenerated->show();
        _applyGenerated->toggle();
    }
}

template <class Range>
void ThemeSettings::buildRangeLineEdits(QVBoxLayout* parentLayout, QString name, qreal min, qreal max, Range& range)
{
    Q_ASSERT(min < max);

    auto* layout = new QHBoxLayout();

    auto* label = new QLabel(name, this);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(label);

    auto* p1 = new QLineEdit(this);
    p1->setText(QString::number(min, 'g', 4));
    p1->setClearButtonEnabled(true);

    auto* p2 = new QLineEdit(this);
    p2->setText(QString::number(max, 'g', 4));
    p2->setClearButtonEnabled(true);

    auto* validator = new QDoubleValidator(min, max, 4, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
    p1->setValidator(validator);
    p2->setValidator(validator);
    layout->addWidget(p1);
    layout->addWidget(p2);

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
        auto previewIndex = model->index(i, ThemeManager::PreviewColumn, QModelIndex());
        QLabel* previewWidget = nullptr;
        if (auto* widget = _tv->indexWidget(previewIndex); widget) {
            previewWidget = qobject_cast<QLabel*>(widget);
            Q_ASSERT(previewWidget);
        } else {
            previewWidget = new QLabel(this);
            previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            previewWidget->setFrameShape(QFrame::Panel);
            previewWidget->setFrameShadow(QFrame::Plain);
            _tv->setIndexWidget(previewIndex, previewWidget);
        }
        previewWidget->setPixmap(paletteToPixmap(palette, previewWidget->size()));

        /// create the Apply widget, if needed.
        const auto applyIndex = model->index(i, ThemeManager::ApplyColumn, QModelIndex());
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
        auto discardIndex = model->index(i, ThemeManager::DiscardColum, QModelIndex());
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
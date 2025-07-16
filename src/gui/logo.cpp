/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "logo.hpp"
#include "SessionManager.hpp"
#include "theme/theme.hpp"

#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QSvgGenerator>


using namespace gui;

QPixmap gui::createLogo(int size)
{
    auto result = QPixmap(size, size);
    result.fill(Qt::transparent);

    QPainter p(&result);
    drawLogo(&p, result.rect());

    return result;
}

void gui::exportLogoSVG(const QString& path)
{
    constexpr auto rec = QRect(0, 0, 128, 128);

    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(rec.size());
    generator.setViewBox(rec);
    generator.setTitle("Surkl logo");
    generator.setDescription("a circle and eight digits of pi");

    QPainter p(&generator);
    drawLogo(&p, rec);
}

void gui::exportLogoPNG(int size, const QString& path)
{
    if (const auto pix = createLogo(size); !pix.save(path)) {
        qWarning() << "failed to export PNG";
    }
}

void gui::exportLogo(const QString& path)
{
    const auto FILE_NAME    = QString("surkl");
    const auto LOGO_DIR     = QString("logo");
    const auto SCALABLE_DIR = QString("scalable");

    auto dir = QDir(path);

    if (!dir.exists(LOGO_DIR) && !dir.mkdir(LOGO_DIR)) {
        qWarning() << "failed to mkdir " + LOGO_DIR;
        return;
    }
    dir.cd(LOGO_DIR);

    if (!dir.exists(SCALABLE_DIR) && !dir.mkdir(SCALABLE_DIR)) {
        qWarning() << "failed to mkdir " + SCALABLE_DIR;
        return;
    }

    dir.cd(SCALABLE_DIR);
    exportLogoSVG(dir.absoluteFilePath(FILE_NAME+".svg"));
    dir.cdUp();

    for (const auto sz : {16, 24, 32, 48, 64, 128, 256}) {
        if (const auto folder = QString("%1").arg(sz); !dir.exists(folder)) {
            if (!dir.mkdir(folder)) {
                qWarning() << "failed to mkdir " << folder;
                continue;
            }
        }
        const auto pngPath = QString("%1/%2/%3.png")
            .arg(dir.absolutePath())
            .arg(sz)
            .arg(FILE_NAME);

        exportLogoPNG(sz, pngPath);
    }
}

void gui::drawLogo(QPainter* p, const QRect& region)
{
    constexpr auto palette = theme::ThemeManager::factory();
    constexpr auto color1  = palette[theme::PaletteIndex::SCENE_LIGHT_COLOR];
    constexpr auto color2  = palette[theme::PaletteIndex::SCENE_DARK_COLOR];
    constexpr auto space   = 1.0;

    const auto penW   = region.width() / 8.0;
    const auto margin = penW / 2.0;
    const auto rec    = region.adjusted(margin, margin, -margin, -margin);

    auto pattern = QList<qreal>()
        << 3.0 << space
        << 1.0 << space
        << 4.0 << space
        << 1.0 << space
        << 5.0 << space
        << 9.0 << space
        << 2.0 << space
        << 6.0 << 0.0;

    const auto patternLen = std::ranges::fold_left(pattern, 0.0, std::plus{}) * penW;

    auto path = QPainterPath();
    path.arcMoveTo(rec, 0);
    path.arcTo(rec, 0, 270);

    const auto scale = path.length() / patternLen;
    const auto apl   = 270.0 / path.length();

    std::ranges::transform(pattern.begin(), pattern.end(), pattern.begin(),
        [scale](qreal x) { return x * scale; });

    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(color1, penW));
    p->drawEllipse(rec);

    auto pen = QPen();
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setColor(color2);
    pen.setWidth(penW);
    pen.setDashPattern(pattern);

    p->setPen(pen);
    p->drawPath(path);

    pen.setDashPattern(QList<qreal>() << 0.5*scale << 0.5*scale);
    p->setPen(pen);
    path.clear();

    path.arcMoveTo(rec, 271 + apl*scale*2.0);
    path.arcTo(rec, 271 + apl*scale*2.0, 85);
    p->drawPath(path);
}

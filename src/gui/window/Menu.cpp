/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "Menu.hpp"
#include "core/SessionManager.hpp"
#include "theme.hpp"

#include <QPainter>
#include <QResizeEvent>


using namespace gui::window;

Menu::Menu(QWidget *parent)
    : QMenu(parent)
{
    QFont ft = font();
    ft.setPixelSize(12.0);
    setFont(ft);
}

void Menu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto* tm = core::SessionManager::tm();
    const auto fg  = tm->sceneLightColor();
    const auto bg  = tm->sceneMidarkColor();
    const auto mg  = tm->sceneColor();
    const auto sh  = tm->sceneDarkColor();

    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.fillRect(rect(), sh);
    p.fillRect(rect().adjusted(1, 1, -1, -1), bg);

    for (const auto actionItems = actions(); auto item: actionItems) {
        const QRect itemRect = actionGeometry(item);
        if (item->isSeparator()) {
            int margin = itemRect.height() / 4;
            p.fillRect(itemRect.adjusted(margin, margin, -margin, -margin), sh);
            continue;
        }

        if (item->isChecked()) {
            p.fillRect(itemRect.adjusted(1, 1, -1, -1), mg);
        }

        if (item == activeAction()) {
            p.fillRect(itemRect.adjusted(1, 1, -1, -1), fg);
            p.setPen(bg);
            p.setBrush(Qt::NoBrush);
        } else {
            p.setPen(fg);
            p.setBrush(Qt::NoBrush);
        }
        p.drawText(itemRect, Qt::AlignCenter, item->text());
    }
}

void Menu::mouseReleaseEvent(QMouseEvent *event)
{
    QMenu::mouseReleaseEvent(event);

    hide();
}

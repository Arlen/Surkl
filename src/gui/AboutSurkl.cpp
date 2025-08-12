/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "AboutSurkl.hpp"
#include "MainWindow.hpp"
#include "logo.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>


using namespace gui;

namespace
{
    QString buildInfo()
    {
        return QString("Using Qt %1 on %2")
            .arg(QLatin1String(qVersion()), QSysInfo::buildCpuArchitecture());
    }

    QString aboutInfo()
    {
        const auto wrapBr = [br=QLatin1String("<br>%1<br/>")](const QString &str) {
            return br.arg(str);
        };

        const auto info = QString("<h2>%1 %2</h2> %3%4%5%6%7%8")
        .arg(
            QApplication::applicationDisplayName(),
            QApplication::applicationVersion(),
            buildInfo(),
            wrapBr(wrapBr(R"(&#9888; Currently in development. Expect bugs and missing features! &#9888;)")),

            wrapBr(R"(Copyright (C) 2025 <a href="https://github.com/Arlen/"> Arlen Avakian </a>)"),

            wrapBr(R"(This program is free software: you can redistribute it and/or modify
                    it under the terms of the GNU General Public License as published by
                    the Free Software Foundation, either version 3 of the License, or
                    (at your option) any later version.)"),

            wrapBr(R"(This program is distributed in the hope that it will be useful,
                    but WITHOUT ANY WARRANTY; without even the implied warranty of
                    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
                    GNU General Public License for more details.)"),

            wrapBr(R"(You should have received a copy of the GNU General Public License
                    along with this program.  If not, see
                    <a href="http://www.gnu.org/licenses/">http://www.gnu.org/licenses/</a>.)"));

        return info;
    }
}


AboutDialog::AboutDialog()
    : QDialog(gui::MainWindow::first())
{
    setWindowTitle(QString("About %1").arg(QApplication::applicationDisplayName()));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    auto* logoLabel = new QLabel;
    const auto scaleFactor = screen()->devicePixelRatio();
    auto logoPix = createLogo(96 * scaleFactor);
    logoPix.setDevicePixelRatio(scaleFactor);
    logoLabel->setPixmap(logoPix);
    logoLabel->setFixedSize(logoPix.deviceIndependentSize().toSize());
    logoLabel->setContentsMargins(0, 0, 0, 0);

    auto* copyRightLabel = new QLabel(aboutInfo());
    copyRightLabel->setWordWrap(true);
    copyRightLabel->setOpenExternalLinks(true);
    copyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    auto* copyButton = buttonBox->addButton("Copy and Close", QDialogButtonBox::ApplyRole);

    auto* hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(8);
    hboxLayout->setContentsMargins(0, 0, 0, 0);

    auto* vboxLayout1 = new QVBoxLayout();
    vboxLayout1->setSpacing(0);
    vboxLayout1->setContentsMargins(8, 8, 8, 8);
    vboxLayout1->addWidget(logoLabel);
    vboxLayout1->addSpacerItem(new QSpacerItem(0, 1, QSizePolicy::Ignored, QSizePolicy::Expanding));

    auto* vboxLayout2 = new QVBoxLayout();
    vboxLayout2->setSpacing(0);
    vboxLayout2->setContentsMargins(0, 0, 0, 0);
    vboxLayout2->addWidget(copyRightLabel);

    hboxLayout->addLayout(vboxLayout1);
    hboxLayout->addLayout(vboxLayout2);
    mainLayout->addLayout(hboxLayout);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(copyButton, &QPushButton::pressed, this,
        [text=aboutInfo(), this] {
            auto *clipboard = QApplication::clipboard();
            clipboard->setText(text);
            if (clipboard->supportsSelection()) {
                clipboard->setText(text, QClipboard::Selection);
            }
            accept();
    });
}
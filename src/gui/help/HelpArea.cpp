/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#include "HelpArea.hpp"

#include <QApplication>
#include <QTextEdit>


using namespace gui::help;

namespace
{
    const char docs_file[] {
#embed "docs.md"
    };


    QString docMarkdown()
    {
        QString text = QString("%1 %2 documentation\n===\n")
            .arg(QApplication::applicationDisplayName(),
                 QApplication::applicationVersion());

        text += QString(docs_file);

        return text;
    }
}

HelpArea::HelpArea(window::Window *parent)
    : AbstractWindowArea(parent)
{
    setWidget(textEdit());
}

gui::window::AbstractWindowArea::AreaType HelpArea::type() const
{
    return AreaType::HelpArea;
}

QTextEdit* HelpArea::textEdit()
{
    auto* text = new QTextEdit(this);

    text->setMarkdown(docMarkdown());

    return text;
}
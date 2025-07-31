/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>


class QTimeLine;

namespace core
{
    class DeletionDialog final : public QDialog
    {
    public:
        explicit DeletionDialog(QWidget* parent);

    protected:
        void keyReleaseEvent(QKeyEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

    private:
        QTimeLine* _tl{nullptr};
    };
}
/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <MainWindow.hpp>

#include <QObject>


namespace gui
{
    namespace theme
    {
        class ThemeManager;
    }
    class MainWindow;
    class UiStorage;
}

namespace core
{
    class BookmarkManager;
    class FileSystemScene;
    class SceneStorage;

    class SessionManager final : public QObject
    {
        Q_OBJECT

    public:
        explicit SessionManager(QObject* parent = nullptr);
        ~SessionManager() override;
        static BookmarkManager* bm();
        static FileSystemScene* scene();
        static SceneStorage* ss();
        static gui::MainWindow* mw();
        static gui::UiStorage* us();
        static gui::theme::ThemeManager* tm();

    private slots:
        void cleanup() const;

    private:
        void init();
        static SessionManager* session();

        BookmarkManager*   _bm{nullptr};
        FileSystemScene*   _sc{nullptr};
        SceneStorage*      _ss{nullptr};
        gui::MainWindow*   _mw{nullptr};
        gui::UiStorage*    _us{nullptr};
        gui::theme::ThemeManager* _tm{nullptr};
    };
}

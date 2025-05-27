/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <MainWindow.hpp>
#include <QtCore/QObject>


namespace gui
{
    class ThemeManager;
    class MainWindow;
}

namespace core
{
    class SceneStorage;
    class FileSystemScene;
    class BookmarkManager;

    class SessionManager final : public QObject
    {
        Q_OBJECT

    public:
        explicit SessionManager(QObject* parent = nullptr);
        ~SessionManager() override;
        static SceneStorage* ss();
        static FileSystemScene* scene();
        static BookmarkManager* bm();
        static gui::ThemeManager* tm();
        static gui::MainWindow* mw();

    private slots:
        void cleanup() const;

    private:
        void init();
        static SessionManager* session();

        SceneStorage*      _ss{nullptr};
        FileSystemScene*   _sc{nullptr};
        BookmarkManager*   _bm{nullptr};
        gui::ThemeManager* _tm{nullptr};
        gui::MainWindow*   _mw{nullptr};
    };
}

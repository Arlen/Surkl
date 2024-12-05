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
    class Scene;
    class BookmarkManager;

    class SessionManager final : public QObject
    {
    public:
        explicit SessionManager(QObject* parent = nullptr);

        ~SessionManager() override;

        void start();

        [[nodiscard]] Scene* scene() const;
        [[nodiscard]] BookmarkManager* bm() const;
        [[nodiscard]] gui::ThemeManager* tm() const;
        [[nodiscard]] gui::MainWindow* mw() const;

    private:
        void initialize();

        Scene* _scene = nullptr;
        BookmarkManager* _bm = nullptr;
        gui::ThemeManager* _tm = nullptr;
        gui::MainWindow* _mw = nullptr;

        bool _initialized;
    };

    SessionManager* session();
}

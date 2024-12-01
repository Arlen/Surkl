#pragma once

#include <QtCore/QObject>


namespace gui
{
    class ThemeManager;
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

    private:
        void initialize();

        Scene* _scene = nullptr;
        BookmarkManager* _bm = nullptr;
        gui::ThemeManager* _tm = nullptr;

        bool _initialized;
    };

    SessionManager* session();
}

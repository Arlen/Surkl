#include "gui/theme.hpp"


using namespace gui;

void ThemeManager::reset() noexcept
{
    _bg_color = builtin::bgColor();
}
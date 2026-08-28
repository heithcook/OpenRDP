#include "rdp/RdpInput.h"

#include <algorithm>
#include <winpr/input.h>

namespace openrdp {
QPoint scaleMousePosition(const QPoint& p, const QSize& local, const QSize& remote)
{
    if (local.isEmpty() || remote.isEmpty()) return {};
    const int x = std::clamp(static_cast<int>((static_cast<qint64>(p.x()) * remote.width()) / local.width()), 0, remote.width() - 1);
    const int y = std::clamp(static_cast<int>((static_cast<qint64>(p.y()) * remote.height()) / local.height()), 0, remote.height() - 1);
    return {x, y};
}

std::uint32_t qtKeyToVirtualKey(const Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z) return VK_KEY_A + (key - Qt::Key_A);
    if (key >= Qt::Key_0 && key <= Qt::Key_9) return VK_KEY_0 + (key - Qt::Key_0);
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) return VK_F1 + (key - Qt::Key_F1);
    switch (key) {
    case Qt::Key_Backspace: return VK_BACK;
    case Qt::Key_Tab: return VK_TAB;
    case Qt::Key_Return: case Qt::Key_Enter: return VK_RETURN;
    // Qt does not expose left/right in Qt::Key. Use the left-side Windows
    // virtual keys as the reliable Phase 1 baseline; WinPR maps these to
    // concrete scancodes whereas the generic modifier VKs are ambiguous.
    case Qt::Key_Shift: return VK_LSHIFT;
    case Qt::Key_Control: return VK_LCONTROL;
    case Qt::Key_Alt: return VK_LMENU;
    case Qt::Key_CapsLock: return VK_CAPITAL;
    case Qt::Key_Escape: return VK_ESCAPE;
    case Qt::Key_Space: return VK_SPACE;
    case Qt::Key_PageUp: return VK_PRIOR;
    case Qt::Key_PageDown: return VK_NEXT;
    case Qt::Key_End: return VK_END;
    case Qt::Key_Home: return VK_HOME;
    case Qt::Key_Left: return VK_LEFT;
    case Qt::Key_Up: return VK_UP;
    case Qt::Key_Right: return VK_RIGHT;
    case Qt::Key_Down: return VK_DOWN;
    case Qt::Key_Insert: return VK_INSERT;
    case Qt::Key_Delete: return VK_DELETE;
    case Qt::Key_Meta: return VK_LWIN;
    // On Qt, Shift+number and Shift+OEM keys are reported as the resulting
    // symbol. The separate Shift key event supplies the modifier remotely;
    // map both forms to the same physical Windows key.
    case Qt::Key_Exclam: return VK_KEY_1;
    case Qt::Key_At: return VK_KEY_2;
    case Qt::Key_NumberSign: return VK_KEY_3;
    case Qt::Key_Dollar: return VK_KEY_4;
    case Qt::Key_Percent: return VK_KEY_5;
    case Qt::Key_AsciiCircum: return VK_KEY_6;
    case Qt::Key_Ampersand: return VK_KEY_7;
    case Qt::Key_Asterisk: return VK_KEY_8;
    case Qt::Key_ParenLeft: return VK_KEY_9;
    case Qt::Key_ParenRight: return VK_KEY_0;
    case Qt::Key_Minus: case Qt::Key_Underscore: return VK_OEM_MINUS;
    case Qt::Key_Equal: case Qt::Key_Plus: return VK_OEM_PLUS;
    case Qt::Key_BracketLeft: case Qt::Key_BraceLeft: return VK_OEM_4;
    case Qt::Key_BracketRight: case Qt::Key_BraceRight: return VK_OEM_6;
    case Qt::Key_Backslash: case Qt::Key_Bar: return VK_OEM_5;
    case Qt::Key_Semicolon: case Qt::Key_Colon: return VK_OEM_1;
    case Qt::Key_Apostrophe: case Qt::Key_QuoteDbl: return VK_OEM_7;
    case Qt::Key_Comma: case Qt::Key_Less: return VK_OEM_COMMA;
    case Qt::Key_Period: case Qt::Key_Greater: return VK_OEM_PERIOD;
    case Qt::Key_Slash: case Qt::Key_Question: return VK_OEM_2;
    case Qt::Key_QuoteLeft: case Qt::Key_AsciiTilde: return VK_OEM_3;
    default: return 0;
    }
}
} // namespace openrdp

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
    case Qt::Key_Shift: return VK_SHIFT;
    case Qt::Key_Control: return VK_CONTROL;
    case Qt::Key_Alt: return VK_MENU;
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
    default: return 0;
    }
}
} // namespace openrdp

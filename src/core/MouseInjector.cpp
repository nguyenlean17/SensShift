#include "MouseInjector.h"

MouseInjector::MouseInjector() = default;

bool MouseInjector::InjectRelativeMove(int dx, int dy) {
    if (dx == 0 && dy == 0) {
        return true;
    }

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.mouseData = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.time = 0;
    input.mi.dwExtraInfo = SENS_INJECTED_MAGIC;

    UINT sent = SendInput(1, &input, sizeof(INPUT));
    if (sent > 0) {
        m_totalInjectedMoves.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    return false;
}

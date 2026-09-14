#pragma once

#include "InputDefines.h"
#include <atomic>

class MouseInjector {
public:
    MouseInjector();
    ~MouseInjector() = default;

    // Injects relative mouse movement with safety signature
    bool InjectRelativeMove(int dx, int dy);

    uint64_t GetTotalInjectedMoves() const { return m_totalInjectedMoves.load(std::memory_order_relaxed); }
    void ResetCounters() { m_totalInjectedMoves.store(0, std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> m_totalInjectedMoves{0};
};

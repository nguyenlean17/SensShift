#include "MouseInterceptor.h"
#include "ProfileManager.h"
#include "TransformationPipeline.h"
#include "KeyboardHotkeyManager.h"

MouseInterceptor* MouseInterceptor::s_instance = nullptr;

MouseInterceptor::MouseInterceptor(ProfileManager& profileManager,
                                   TransformationPipeline& pipeline,
                                   MouseInjector& injector,
                                   KeyboardHotkeyManager& keyboardManager)
    : m_profileManager(profileManager),
      m_pipeline(pipeline),
      m_injector(injector),
      m_keyboardManager(keyboardManager) {
    s_instance = this;
}

MouseInterceptor::~MouseInterceptor() {
    Stop();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool MouseInterceptor::Start() {
    if (m_running.load(std::memory_order_relaxed)) return true;

    if (m_hookThread.joinable()) {
        m_hookThread.join();
    }

    m_hasLastCursorPos = false;
    m_running.store(true, std::memory_order_release);
    m_hookThread = std::thread(&MouseInterceptor::HookThreadWorker, this);

    // Wait briefly for thread to initialize message queue and hook
    int retry = 0;
    while (m_hook.load(std::memory_order_acquire) == NULL &&
           m_running.load(std::memory_order_acquire) &&
           retry++ < 50) {
        Sleep(10);
    }

    return m_hook.load(std::memory_order_acquire) != NULL;
}

void MouseInterceptor::Stop() {
    m_hasLastCursorPos = false;
    bool wasRunning = m_running.exchange(false, std::memory_order_acq_rel);
    if (wasRunning) {
        DWORD tid = m_hookThreadId.load(std::memory_order_acquire);
        if (tid != 0) {
            PostThreadMessageW(tid, WM_QUIT, 0, 0);
        }
    }

    if (m_hookThread.joinable()) {
        m_hookThread.join();
    }

    m_hookThreadId.store(0, std::memory_order_relaxed);
}

void MouseInterceptor::HookThreadWorker() {
    // Force Win32 message queue creation for this thread before publishing thread ID
    MSG initMsg;
    PeekMessageW(&initMsg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    m_hookThreadId.store(GetCurrentThreadId(), std::memory_order_release);

    // Maximize hook responsiveness
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    HHOOK hook = SetWindowsHookExW(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandleW(NULL),
        0
    );

    if (!hook) {
        m_running.store(false, std::memory_order_relaxed);
        return;
    }

    m_hook.store(hook, std::memory_order_release);

    // Non-blocking message pump: process messages when available,
    // periodically check m_running flag to allow clean shutdown
    // even if PostThreadMessageW(WM_QUIT) fails.
    while (m_running.load(std::memory_order_acquire)) {
        DWORD result = MsgWaitForMultipleObjects(0, NULL, FALSE, 150, QS_ALLINPUT);
        if (result == WAIT_OBJECT_0) {
            MSG msg;
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    m_running.store(false, std::memory_order_relaxed);
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        // WAIT_TIMEOUT: loop back and check m_running
    }

    // Unhook on the SAME thread that installed the hook — this is the correct
    // way per MSDN. Cross-thread UnhookWindowsHookEx is unreliable for LL hooks.
    if (hook) {
        UnhookWindowsHookEx(hook);
    }
    m_hook.store(NULL, std::memory_order_release);
}

LRESULT CALLBACK MouseInterceptor::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance) {
        const auto* p = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        return s_instance->ProcessMouseEvent(nCode, wParam, p);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT MouseInterceptor::ProcessMouseEvent(int nCode, WPARAM wParam, const MSLLHOOKSTRUCT* p) {
    // Update hook health heartbeat
    m_lastEventTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    // 1. Injected Input Guard: Prevent infinite feedback loops
    bool isInjected = (p->dwExtraInfo == SENS_INJECTED_MAGIC) || ((p->flags & LLMHF_INJECTED) != 0);
    if (isInjected) {
        // Track the cursor position after injection so subsequent physical moves calculate accurately
        m_lastCursorPos = p->pt;
        m_hasLastCursorPos = true;
        return CallNextHookEx(NULL, nCode, wParam, reinterpret_cast<LPARAM>(p));
    }

    uint32_t currentMods = m_keyboardManager.GetCurrentModifiers();

    // 2. Mouse Button Detection for Hotkeys
    MouseButton btn = MouseButton::None;
    bool isDown = false;
    bool isUp = false;

    switch (wParam) {
        case WM_LBUTTONDOWN: btn = MouseButton::Left; isDown = true; break;
        case WM_LBUTTONUP:   btn = MouseButton::Left; isUp = true; break;
        case WM_RBUTTONDOWN: btn = MouseButton::Right; isDown = true; break;
        case WM_RBUTTONUP:   btn = MouseButton::Right; isUp = true; break;
        case WM_MBUTTONDOWN: btn = MouseButton::Middle; isDown = true; break;
        case WM_MBUTTONUP:   btn = MouseButton::Middle; isUp = true; break;
        case WM_XBUTTONDOWN: {
            WORD xBtn = HIWORD(p->mouseData);
            btn = (xBtn == XBUTTON1) ? MouseButton::XButton1 : MouseButton::XButton2;
            isDown = true;
            break;
        }
        case WM_XBUTTONUP: {
            WORD xBtn = HIWORD(p->mouseData);
            btn = (xBtn == XBUTTON1) ? MouseButton::XButton1 : MouseButton::XButton2;
            isUp = true;
            break;
        }
        default:
            break;
    }

    if (btn != MouseButton::None) {
        if (isDown) {
            // Check recorder callback (lock-free when not recording)
            if (m_hasRecorderCallback.load(std::memory_order_relaxed)) {
                std::unique_lock<std::mutex> lock(m_callbackMutex, std::try_to_lock);
                if (lock.owns_lock() && m_recorderCallback) {
                    if (m_recorderCallback(0, btn, currentMods)) {
                        return 1;
                    }
                }
            }

            if (m_profileManager.OnMouseButtonPressed(btn, currentMods)) {
                return 1;
            }
        } else if (isUp) {
            if (m_profileManager.OnMouseButtonReleased(btn, currentMods)) {
                return 1;
            }
        }
    }

    // 3. Movement Transformation
    if (wParam == WM_MOUSEMOVE) {
        if (!m_hasLastCursorPos) {
            m_lastCursorPos = p->pt;
            m_hasLastCursorPos = true;
            return CallNextHookEx(NULL, nCode, wParam, reinterpret_cast<LPARAM>(p));
        }

        int dx = p->pt.x - m_lastCursorPos.x;
        int dy = p->pt.y - m_lastCursorPos.y;
        m_lastCursorPos = p->pt;

        if (dx == 0 && dy == 0) {
            return CallNextHookEx(NULL, nCode, wParam, reinterpret_cast<LPARAM>(p));
        }

        // If emergency disabled or no modification needed, update telemetry and pass through
        if (m_profileManager.IsEmergencyDisabled() || !m_profileManager.IsModifierActive()) {
            int outX = 0, outY = 0;
            m_pipeline.Transform(dx, dy, outX, outY);
            return CallNextHookEx(NULL, nCode, wParam, reinterpret_cast<LPARAM>(p));
        }

        // Active modification: calculate transformed deltas, inject, and swallow physical event
        int outX = 0, outY = 0;
        m_pipeline.Transform(dx, dy, outX, outY);

        m_injector.InjectRelativeMove(outX, outY);

        // Returning 1 instructs Windows to drop this physical move event
        return 1;
    }

    return CallNextHookEx(NULL, nCode, wParam, reinterpret_cast<LPARAM>(p));
}

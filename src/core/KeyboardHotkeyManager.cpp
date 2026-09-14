#include "KeyboardHotkeyManager.h"
#include "ProfileManager.h"

KeyboardHotkeyManager* KeyboardHotkeyManager::s_instance = nullptr;

KeyboardHotkeyManager::KeyboardHotkeyManager(ProfileManager& profileManager)
    : m_profileManager(profileManager) {
    s_instance = this;
}

KeyboardHotkeyManager::~KeyboardHotkeyManager() {
    Stop();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool KeyboardHotkeyManager::Start() {
    if (m_running.load(std::memory_order_relaxed)) return true;

    if (m_hookThread.joinable()) {
        m_hookThread.join();
    }

    m_running.store(true, std::memory_order_release);
    m_hookThread = std::thread(&KeyboardHotkeyManager::HookThreadWorker, this);

    // Wait briefly for thread to initialize message queue and hook
    int retry = 0;
    while (m_hook.load(std::memory_order_acquire) == NULL &&
           m_running.load(std::memory_order_acquire) &&
           retry++ < 50) {
        Sleep(10);
    }

    return m_hook.load(std::memory_order_acquire) != NULL;
}

void KeyboardHotkeyManager::Stop() {
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

void KeyboardHotkeyManager::HookThreadWorker() {
    // Force Win32 message queue creation for this thread before publishing thread ID
    MSG initMsg;
    PeekMessageW(&initMsg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    m_hookThreadId.store(GetCurrentThreadId(), std::memory_order_release);

    // Maximize hook responsiveness — this thread only handles the hook
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    HHOOK hook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
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

LRESULT CALLBACK KeyboardHotkeyManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance) {
        const auto* p = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        return s_instance->ProcessKeyboardEvent(nCode, wParam, p);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT KeyboardHotkeyManager::ProcessKeyboardEvent(int nCode, WPARAM wParam, const KBDLLHOOKSTRUCT* p) {
    bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

    uint32_t vk = p->vkCode;

    // Update modifier states
    uint32_t mods = m_currentModifiers.load(std::memory_order_relaxed);
    if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) {
        if (isDown) mods |= ModShift; else mods &= ~ModShift;
    } else if (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) {
        if (isDown) mods |= ModCtrl; else mods &= ~ModCtrl;
    } else if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) {
        if (isDown) mods |= ModAlt; else mods &= ~ModAlt;
    } else if (vk == VK_LWIN || vk == VK_RWIN) {
        if (isDown) mods |= ModWin; else mods &= ~ModWin;
    }
    m_currentModifiers.store(mods, std::memory_order_relaxed);

    // Also check asynchronous key state for maximum reliability
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) mods |= ModShift;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= ModCtrl;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) mods |= ModAlt;

    if (isDown) {
        // Check recorder callback (lock-free when not recording)
        if (m_hasRecorderCallback.load(std::memory_order_relaxed)) {
            if (vk != VK_LSHIFT && vk != VK_RSHIFT && vk != VK_LCONTROL && vk != VK_RCONTROL && vk != VK_LMENU && vk != VK_RMENU) {
                std::unique_lock<std::mutex> lock(m_callbackMutex, std::try_to_lock);
                if (lock.owns_lock() && m_recorderCallback) {
                    if (m_recorderCallback(vk, MouseButton::None, mods)) {
                        return 1;
                    }
                }
            }
        }

        if (m_profileManager.OnKeyPressed(vk, mods)) {
            return 1; // Block key
        }
    } else if (isUp) {
        if (m_profileManager.OnKeyReleased(vk, mods)) {
            return 1;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, reinterpret_cast<LPARAM>(p));
}

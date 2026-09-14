#pragma once

#include "InputDefines.h"
#include <windows.h>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

class ProfileManager;

class KeyboardHotkeyManager {
public:
    explicit KeyboardHotkeyManager(ProfileManager& profileManager);
    ~KeyboardHotkeyManager();

    bool Start();
    void Stop();

    uint32_t GetCurrentModifiers() const { return m_currentModifiers.load(std::memory_order_relaxed); }

    using InputRecorderCallback = std::function<bool(uint32_t vkCode, MouseButton btn, uint32_t mods)>;
    void SetRecorderCallback(InputRecorderCallback cb) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_recorderCallback = std::move(cb);
        m_hasRecorderCallback.store(m_recorderCallback != nullptr, std::memory_order_release);
    }
    void ClearRecorderCallback() {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_hasRecorderCallback.store(false, std::memory_order_release);
        m_recorderCallback = nullptr;
    }

private:
    ProfileManager& m_profileManager;
    std::atomic<uint32_t> m_currentModifiers{ModNone};

    std::atomic<bool> m_hasRecorderCallback{false};
    std::mutex m_callbackMutex;
    InputRecorderCallback m_recorderCallback;

    std::atomic<bool> m_running{false};
    std::thread m_hookThread;
    std::atomic<DWORD> m_hookThreadId{0};
    std::atomic<HHOOK> m_hook{NULL};

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static KeyboardHotkeyManager* s_instance;

    void HookThreadWorker();
    LRESULT ProcessKeyboardEvent(int nCode, WPARAM wParam, const KBDLLHOOKSTRUCT* p);
};

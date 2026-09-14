#pragma once

#include "InputDefines.h"
#include "MouseInjector.h"
#include <windows.h>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <chrono>

class ProfileManager;
class TransformationPipeline;
class KeyboardHotkeyManager;

class MouseInterceptor {
public:
    MouseInterceptor(ProfileManager& profileManager,
                     TransformationPipeline& pipeline,
                     MouseInjector& injector,
                     KeyboardHotkeyManager& keyboardManager);
    ~MouseInterceptor();

    bool Start();
    void Stop();

    bool IsRunning() const { return m_running.load(std::memory_order_relaxed); }

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
    TransformationPipeline& m_pipeline;
    MouseInjector& m_injector;
    KeyboardHotkeyManager& m_keyboardManager;
    std::atomic<bool> m_hasRecorderCallback{false};
    std::mutex m_callbackMutex;
    InputRecorderCallback m_recorderCallback;

    std::atomic<bool> m_running{false};
    std::thread m_hookThread;
    std::atomic<DWORD> m_hookThreadId{0};
    std::atomic<HHOOK> m_hook{NULL};

    POINT m_lastCursorPos{0, 0};
    bool m_hasLastCursorPos = false;

    // Hook health: updated on every hook callback for liveness detection
    std::atomic<std::chrono::steady_clock::time_point> m_lastEventTime{
        std::chrono::steady_clock::time_point{}
    };

public:
    // Returns milliseconds since the last hook event (0 if never received)
    int64_t GetLastEventAgeMs() const {
        auto last = m_lastEventTime.load(std::memory_order_relaxed);
        if (last == std::chrono::steady_clock::time_point{}) return -1;
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last).count();
    }

private:
    static MouseInterceptor* s_instance;
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    void HookThreadWorker();
    LRESULT ProcessMouseEvent(int nCode, WPARAM wParam, const MSLLHOOKSTRUCT* p);
};

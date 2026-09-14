#pragma once

#include <windows.h>
#include <string>
#include <functional>

class AppTracker {
public:
    using AppChangeCallback = std::function<void(const std::string& processName)>;

    AppTracker();
    ~AppTracker();

    bool Start(AppChangeCallback callback);
    void Stop();

    std::string GetCurrentForegroundProcessName() const;

private:
    HWINEVENTHOOK m_hook = NULL;
    AppChangeCallback m_callback;

    static void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD idEventThread,
        DWORD dwmsEventTime
    );

    static AppTracker* s_instance;
    std::string GetProcessNameFromHwnd(HWND hwnd) const;
};

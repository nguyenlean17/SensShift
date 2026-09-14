#include "AppTracker.h"
#include <shlwapi.h>
#include <algorithm>

AppTracker* AppTracker::s_instance = nullptr;

AppTracker::AppTracker() {
    s_instance = this;
}

AppTracker::~AppTracker() {
    Stop();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool AppTracker::Start(AppChangeCallback callback) {
    m_callback = std::move(callback);
    m_hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        NULL,
        WinEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );
    return m_hook != NULL;
}

void AppTracker::Stop() {
    if (m_hook) {
        UnhookWinEvent(m_hook);
        m_hook = NULL;
    }
}

void CALLBACK AppTracker::WinEventProc(
    HWINEVENTHOOK /*hWinEventHook*/,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD /*idEventThread*/,
    DWORD /*dwmsEventTime*/
) {
    if (event == EVENT_SYSTEM_FOREGROUND && idObject == OBJID_WINDOW && idChild == INDEXID_CONTAINER) {
        if (s_instance && s_instance->m_callback && hwnd) {
            std::string procName = s_instance->GetProcessNameFromHwnd(hwnd);
            if (!procName.empty()) {
                s_instance->m_callback(procName);
            }
        }
    }
}

std::string AppTracker::GetProcessNameFromHwnd(HWND hwnd) const {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return "";

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return "";

    WCHAR fullPath[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    std::string result;
    if (QueryFullProcessImageNameW(hProc, 0, fullPath, &size)) {
        WCHAR* fileName = PathFindFileNameW(fullPath);
        char mbName[MAX_PATH] = {0};
        WideCharToMultiByte(CP_UTF8, 0, fileName, -1, mbName, sizeof(mbName), NULL, NULL);
        result = mbName;
        // Convert to lowercase
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    }
    CloseHandle(hProc);
    return result;
}

std::string AppTracker::GetCurrentForegroundProcessName() const {
    HWND fg = GetForegroundWindow();
    if (fg) {
        return GetProcessNameFromHwnd(fg);
    }
    return "";
}

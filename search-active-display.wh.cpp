// ==WindhawkMod==
// @id              search-active-display
// @name            Search on Active Display
// @description     Opens Win+S search on the monitor where the mouse cursor is located, or in a custom monitor of choice
// @version         1.4
// @author          ereinaimer
// @github          https://github.com/ereinaimer
// @include         SearchHost.exe
// @architecture    x86-64
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Search on Active Display

Opens the Windows Search (Win+S) on the monitor where the mouse cursor is
located, or on a specific monitor of choice.

Works for both Win+S shortcut and taskbar search icon click.

## How it works

The mod hooks MonitorFromWindow and MonitorFromRect inside SearchHost.exe.
When SearchHost queries which monitor its CoreWindow is on, the mod returns
the target monitor (where the cursor is, or a fixed monitor). It also
physically repositions the CoreWindow to the correct monitor coordinates
so the DWM compositor renders it there.

## Selecting a monitor

Set the **Monitor** setting to 0 to follow the mouse cursor, or to a
specific monitor number (1, 2, 3...). You can also use a monitor interface
name for stable identification across reboots.

## Supported Windows Builds

- Windows 11 22H2 and 24H2.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- monitor: 0
  $name: Monitor
  $description: >-
    The monitor number that the search will appear on. Set to zero to use
    the monitor where the mouse cursor is located.
- monitorInterfaceName: ""
  $name: Monitor interface name
  $description: >-
    If not empty, the given monitor interface name (can also be an interface
    name substring) will be used instead of the monitor number. Can be useful if
    the monitor numbers change often. To see all available interface names, set
    any interface name, enable mod logs, open the search and look for "Found
    display device" messages.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

struct {
    int monitor;
    WindhawkUtils::StringSetting monitorInterfaceName;
} g_settings;

// ============================================================================
// Monitor resolution helpers
// ============================================================================

HMONITOR GetMonitorById(int monitorId) {
    HMONITOR monitorResult = nullptr;
    int currentMonitorId = 0;

    auto monitorEnumProc = [&](HMONITOR hMonitor) -> BOOL {
        if (currentMonitorId == monitorId) {
            monitorResult = hMonitor;
            return FALSE;
        }
        currentMonitorId++;
        return TRUE;
    };

    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor,
           LPARAM dwData) -> BOOL {
            auto& proc = *reinterpret_cast<decltype(monitorEnumProc)*>(dwData);
            return proc(hMonitor);
        },
        reinterpret_cast<LPARAM>(&monitorEnumProc));

    return monitorResult;
}

HMONITOR GetMonitorByInterfaceNameSubstr(PCWSTR interfaceNameSubstr) {
    HMONITOR monitorResult = nullptr;

    auto monitorEnumProc = [&](HMONITOR hMonitor) -> BOOL {
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);

        if (GetMonitorInfo(hMonitor, &monitorInfo)) {
            DISPLAY_DEVICE displayDevice = {
                .cb = sizeof(displayDevice),
            };

            if (EnumDisplayDevices(monitorInfo.szDevice, 0, &displayDevice,
                                   EDD_GET_DEVICE_INTERFACE_NAME)) {
                Wh_Log(L"Found display device %s, interface name: %s",
                       monitorInfo.szDevice, displayDevice.DeviceID);

                if (wcsstr(displayDevice.DeviceID, interfaceNameSubstr)) {
                    Wh_Log(L"Matched display device");
                    monitorResult = hMonitor;
                    return FALSE;
                }
            }
        }
        return TRUE;
    };

    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor,
           LPARAM dwData) -> BOOL {
            auto& proc = *reinterpret_cast<decltype(monitorEnumProc)*>(dwData);
            return proc(hMonitor);
        },
        reinterpret_cast<LPARAM>(&monitorEnumProc));

    return monitorResult;
}

HMONITOR GetTargetMonitor() {
    if (*g_settings.monitorInterfaceName.get()) {
        return GetMonitorByInterfaceNameSubstr(
            g_settings.monitorInterfaceName.get());
    } else if (g_settings.monitor == 0) {
        POINT pt;
        GetCursorPos(&pt);
        return MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    } else if (g_settings.monitor >= 1) {
        return GetMonitorById(g_settings.monitor - 1);
    }
    return nullptr;
}

// ============================================================================
// ============================================================================
// Win32 API hooks in SearchHost.exe
// ============================================================================

using MonitorFromWindow_t = decltype(&MonitorFromWindow);
MonitorFromWindow_t MonitorFromWindow_Original;

using MonitorFromRect_t = decltype(&MonitorFromRect);
MonitorFromRect_t MonitorFromRect_Original;

// ============================================================================
// CoreWindow detection and repositioning
// ============================================================================

bool IsCoreWindow(HWND hwnd) {
    WCHAR className[256] = {0};
    GetClassName(hwnd, className, ARRAYSIZE(className));
    return _wcsicmp(className, L"Windows.UI.Core.CoreWindow") == 0;
}

// Translate a window rect from its current monitor to the target monitor,
// preserving relative position within the monitor's work area.
void MoveWindowToMonitor(HWND hwnd, HMONITOR targetMonitor) {
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return;
    }

    HMONITOR currentMonitor = MonitorFromRect_Original 
        ? MonitorFromRect_Original(&windowRect, MONITOR_DEFAULTTONEAREST) 
        : MonitorFromRect(&windowRect, MONITOR_DEFAULTTONEAREST);

    if (currentMonitor == targetMonitor) {
        return;  // Already on the right monitor
    }

    MONITORINFO currentMi{.cbSize = sizeof(MONITORINFO)};
    MONITORINFO targetMi{.cbSize = sizeof(MONITORINFO)};
    GetMonitorInfo(currentMonitor, &currentMi);
    GetMonitorInfo(targetMonitor, &targetMi);

    // Compute the window's relative position within the current monitor
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int currentMonWidth = currentMi.rcWork.right - currentMi.rcWork.left;
    int currentMonHeight = currentMi.rcWork.bottom - currentMi.rcWork.top;
    int targetMonWidth = targetMi.rcWork.right - targetMi.rcWork.left;
    int targetMonHeight = targetMi.rcWork.bottom - targetMi.rcWork.top;

    // Center the window on the target monitor's work area (same as native behavior)
    int newX = targetMi.rcWork.left + (targetMonWidth - windowWidth) / 2;
    int newY = targetMi.rcWork.top + (targetMonHeight - windowHeight) / 2;

    Wh_Log(L"Moving CoreWindow %p from monitor %p to %p, new pos=(%d,%d)",
           hwnd, currentMonitor, targetMonitor, newX, newY);

    SetWindowPos(hwnd, nullptr, newX, newY, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
}

HMONITOR WINAPI MonitorFromWindow_Hook(HWND hwnd, DWORD dwFlags) {
    HMONITOR original = MonitorFromWindow_Original(hwnd, dwFlags);

    if (IsCoreWindow(hwnd)) {
        HMONITOR target = GetTargetMonitor();
        if (target && target != original) {
            Wh_Log(L"MonitorFromWindow override: hwnd=%p original=%p -> target=%p",
                   hwnd, original, target);

            // Physically move the window to the target monitor
            MoveWindowToMonitor(hwnd, target);

            return target;
        }
    }

    return original;
}

HMONITOR WINAPI MonitorFromRect_Hook(LPCRECT lprc, DWORD dwFlags) {
    HMONITOR original = MonitorFromRect_Original(lprc, dwFlags);

    if (!lprc) {
        return original;
    }

    // Check if this rect belongs to a search window by checking its size.
    // The search window is typically a large centered flyout (800x750ish).
    // We override for any rect that the search host queries.
    HMONITOR target = GetTargetMonitor();
    if (target && target != original) {
        // Only override for rects that are clearly a window rect (not tiny UI elements)
        int width = lprc->right - lprc->left;
        int height = lprc->bottom - lprc->top;
        if (width > 200 && height > 200) {
            Wh_Log(L"MonitorFromRect override: rect=(%d,%d)-(%d,%d) original=%p -> target=%p",
                   lprc->left, lprc->top, lprc->right, lprc->bottom, original, target);
            return target;
        }
    }

    return original;
}

// ============================================================================
// Windhawk callbacks
// ============================================================================

void LoadSettings() {
    g_settings.monitor = Wh_GetIntSetting(L"monitor");
    g_settings.monitorInterfaceName =
        WindhawkUtils::StringSetting::make(L"monitorInterfaceName");
}

BOOL Wh_ModInit() {
    Wh_Log(L">");

    LoadSettings();

    HMODULE user32Module = GetModuleHandle(L"user32.dll");
    if (!user32Module) {
        Wh_Log(L"Couldn't get user32.dll");
        return FALSE;
    }

    WindhawkUtils::SetFunctionHook(
        (void*)GetProcAddress(user32Module, "MonitorFromWindow"),
        (void*)MonitorFromWindow_Hook,
        (void**)&MonitorFromWindow_Original);

    WindhawkUtils::SetFunctionHook(
        (void*)GetProcAddress(user32Module, "MonitorFromRect"),
        (void*)MonitorFromRect_Hook,
        (void**)&MonitorFromRect_Original);

    Wh_Log(L"Hooks installed in SearchHost.exe");

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L">");
}

void Wh_ModSettingsChanged() {
    Wh_Log(L">");
    LoadSettings();
}
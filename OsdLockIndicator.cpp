///////////////////////////////////////////////////////////////////////////////
//
//  OSD LOCK INDICATOR - Customizable On-Screen Display for Caps/Num Lock
//
//  Author: Dope M.S.R. (github.com/DopeMSR)
//  License: MIT
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "psapi.lib")

using namespace Gdiplus;

///////////////////////////////////////////////////////////////////////////////
//  USER SETTINGS - Edit values below, then rebuild (Ctrl+B in Visual Studio)
///////////////////////////////////////////////////////////////////////////////

// =============================================================================
// WINDOW SIZE & SHAPE
// =============================================================================

constexpr int OSD_WIDTH = 175;    // Width of the indicator (pixels)
constexpr int OSD_HEIGHT = 60;     // Height of the indicator (pixels)
constexpr int CORNER_RADIUS = 20;     // Roundness of corners (0 = square)

// =============================================================================
// POSITION ON SCREEN
// =============================================================================

constexpr int DISTANCE_FROM_BOTTOM = 15;    // How far from the bottom of the screen (pixels)
// Increase to move the indicator higher

// =============================================================================
// COLORS - Format: (Alpha, Red, Green, Blue) - Values 0-255
// =============================================================================

// Background
constexpr int BG_ALPHA = 80;     // Background transparency (0=invisible, 255=solid)
constexpr int BG_RED = 0;      // Background red component
constexpr int BG_GREEN = 0;      // Background green component  
constexpr int BG_BLUE = 0;      // Background blue component

// Text Label ("CapsLock:" / "NumLock:")
constexpr int TEXT_RED = 255;    // Label text red
constexpr int TEXT_GREEN = 255;    // Label text green
constexpr int TEXT_BLUE = 255;    // Label text blue (255,255,255 = white)

// "ON" Status Color (default: soft green)
constexpr int ON_RED = 76;     // ON text red
constexpr int ON_GREEN = 217;    // ON text green
constexpr int ON_BLUE = 100;    // ON text blue

// "OFF" Status Color (default: soft coral-red)
constexpr int OFF_RED = 255;    // OFF text red
constexpr int OFF_GREEN = 95;     // OFF text green
constexpr int OFF_BLUE = 87;     // OFF text blue

// =============================================================================
// ANIMATION TIMING
// =============================================================================

constexpr int FADE_SPEED = 25;     // Fade speed (higher = faster, 1-50 recommended)
constexpr int ANIM_INTERVAL = 10;     // Milliseconds between animation frames
constexpr int DISPLAY_TIME = 1500;   // How long to show before fading out (milliseconds)
constexpr bool EASE_ANIMATION = true;   // true = smooth easing, false = linear fade

// =============================================================================
// FONT SETTINGS
// =============================================================================

constexpr float FONT_SIZE = 14.0f;          // Font size in points
const wchar_t* FONT_NAME = L"Segoe UI";    // Font family name

///////////////////////////////////////////////////////////////////////////////
//
//  END OF USER SETTINGS - Code below handles functionality
//
///////////////////////////////////////////////////////////////////////////////


// =============================================================================
// Internal State & Constants
// =============================================================================

enum AnimationState { STATE_HIDDEN, STATE_FADING_IN, STATE_VISIBLE, STATE_FADING_OUT };

AnimationState g_animState = STATE_HIDDEN;
int g_currentAlpha = 0;

HWND g_hwndOSD = NULL;
HHOOK g_keyboardHook = NULL;
std::wstring g_text = L"";
ULONG_PTR g_gdiplusToken;

constexpr UINT_PTR TIMER_ANIM = 1;
constexpr UINT_PTR TIMER_STAY = 2;
constexpr UINT WM_KEYSTATE_CHANGED = WM_USER + 1;

// =============================================================================
// RAII Wrappers for GDI Resources (automatic cleanup)
// =============================================================================

struct ScreenDCReleaser {
    HDC hdc;
    ~ScreenDCReleaser() { if (hdc) ReleaseDC(NULL, hdc); }
};

struct MemoryDCDeleter {
    HDC hdc;
    ~MemoryDCDeleter() { if (hdc) DeleteDC(hdc); }
};

struct BitmapDeleter {
    HBITMAP hbm;
    ~BitmapDeleter() { if (hbm) DeleteObject(hbm); }
};

struct GdiPlusObjectSelector {
    HDC hdc;
    HGDIOBJ hOld;
    ~GdiPlusObjectSelector() { if (hdc && hOld) SelectObject(hdc, hOld); }
};

// =============================================================================
// Forward Declarations
// =============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void UpdateOSD();
void CenterOnActiveMonitor(HWND hwnd);
void ShowIndicator();
bool RemoveFromStartup();
void CleanupAllSettings();
bool IsInStartup();
bool AddToStartup();
bool HasCompletedSetup();
void MarkSetupCompleted();
bool TerminateOtherInstances();

// =============================================================================
// Animation Easing Function
// =============================================================================

inline int CalculateNextAlpha(int current, int target, bool fadeIn)
{
    if (!EASE_ANIMATION) {
        // Linear animation
        if (fadeIn) {
            return min(current + FADE_SPEED, target);
        }
        else {
            return max(current - FADE_SPEED, target);
        }
    }

    // Ease-out quadratic for smooth deceleration
    float progress = static_cast<float>(abs(target - current)) / 255.0f;
    int step = static_cast<int>(FADE_SPEED * (0.5f + progress * 1.5f));
    step = max(step, 3); // Minimum step to ensure animation completes

    if (fadeIn) {
        return min(current + step, target);
    }
    else {
        return max(current - step, target);
    }
}

// =============================================================================
// Helper: Create Rounded Rectangle Path
// =============================================================================

void CreateRoundedRectPath(GraphicsPath& path, int x, int y, int width, int height, int radius)
{
    int d = radius * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + width - d, y, d, d, 270, 90);
    path.AddArc(x + width - d, y + height - d, d, d, 0, 90);
    path.AddArc(x, y + height - d, d, d, 90, 90);
    path.CloseFigure();
}

// =============================================================================
// Process Management
// =============================================================================

bool TerminateOtherInstances()
{
    DWORD currentPID = GetCurrentProcessId();

    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);

    std::wstring currentPathLower = currentPath;
    for (auto& c : currentPathLower) c = towlower(c);

    bool terminatedAny = false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == currentPID) {
                continue;
            }

            if (_wcsicmp(pe32.szExeFile, L"OsdLockIndicator.exe") == 0) {
                HANDLE hProcess = OpenProcess(
                    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
                    FALSE, pe32.th32ProcessID);

                if (hProcess != NULL) {
                    wchar_t processPath[MAX_PATH];
                    if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH) > 0) {
                        std::wstring processPathLower = processPath;
                        for (auto& c : processPathLower) c = towlower(c);

                        if (processPathLower == currentPathLower) {
                            TerminateProcess(hProcess, 0);
                            terminatedAny = true;
                        }
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return terminatedAny;
}

bool RemoveFromStartup()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegDeleteValueW(hKey, L"OsdLockIndicator");
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

void CleanupAllSettings()
{
    // Remove our app settings key (so reinstall shows setup prompt again)
    RegDeleteKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\OsdLockIndicator");
}

bool IsInStartup()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegQueryValueExW(hKey, L"OsdLockIndicator", NULL, NULL, NULL, NULL);
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

bool AddToStartup()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {

        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);

        LONG result = RegSetValueExW(hKey, L"OsdLockIndicator", 0, REG_SZ,
            (LPBYTE)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));

        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }
    return false;
}

bool HasCompletedSetup()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\OsdLockIndicator",
        0, KEY_QUERY_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegQueryValueExW(hKey, L"SetupCompleted", NULL, NULL, NULL, NULL);
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

void MarkSetupCompleted()
{
    HKEY hKey;
    DWORD disposition;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\OsdLockIndicator",
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {

        DWORD value = 1;
        RegSetValueExW(hKey, L"SetupCompleted", 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nShowCmd);

    // --- Handle Uninstall Command ---
    std::string cmdLine(lpCmdLine);
    if (cmdLine.find("/uninstall") != std::string::npos ||
        cmdLine.find("--uninstall") != std::string::npos ||
        cmdLine.find("-uninstall") != std::string::npos) {

        bool terminatedProcesses = TerminateOtherInstances();
        if (terminatedProcesses) {
            Sleep(500);
        }

        bool removedStartup = RemoveFromStartup();
        CleanupAllSettings();

        if (removedStartup) {
            MessageBoxW(NULL,
                L"OSD Lock Indicator has been uninstalled:\n\n"
                L"[OK] Running processes terminated\n"
                L"[OK] Removed from Windows startup\n"
                L"[OK] Settings cleared\n\n"
                L"You can now safely delete the executable file.",
                L"Uninstall Complete",
                MB_OK | MB_ICONINFORMATION);
        }
        else {
            MessageBoxW(NULL,
                L"Processes were terminated, but could not remove from startup registry.\n\n"
                L"Please manually remove from:\n"
                L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                L"Uninstall Error",
                MB_OK | MB_ICONWARNING);
        }

        return 0;
    }

    // --- Handle Install Command (force show startup prompt) ---
    if (cmdLine.find("/install") != std::string::npos ||
        cmdLine.find("--install") != std::string::npos ||
        cmdLine.find("-install") != std::string::npos) {

        int result = MessageBoxW(NULL,
            L"Would you like OSD Lock Indicator to start automatically with Windows?\n\n"
            L"You can change this later by running:\n"
            L"- OsdLockIndicator.exe /install   (to enable)\n"
            L"- OsdLockIndicator.exe /uninstall (to disable)",
            L"OSD Lock Indicator - Setup",
            MB_YESNO | MB_ICONQUESTION);

        if (result == IDYES) {
            if (AddToStartup()) {
                MessageBoxW(NULL,
                    L"[OK] OSD Lock Indicator will now start with Windows.",
                    L"Setup Complete",
                    MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBoxW(NULL,
                    L"Could not add to Windows startup.\n"
                    L"Please try running as administrator.",
                    L"Setup Error",
                    MB_OK | MB_ICONWARNING);
            }
        }
        else {
            RemoveFromStartup();
            MessageBoxW(NULL,
                L"[OK] OSD Lock Indicator will NOT start with Windows.\n\n"
                L"You can run the program manually when needed.",
                L"Setup Complete",
                MB_OK | MB_ICONINFORMATION);
        }

        MarkSetupCompleted();
        return 0;
    }

    // --- Initialize GDI+ ---
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // --- Prevent Multiple Instances ---
    HANDLE mutex = CreateMutexW(NULL, TRUE, L"Global\\OsdLockIndicator_Unique_ID");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    // --- First Run: Ask User About Windows Startup ---
    if (!HasCompletedSetup()) {
        int result = MessageBoxW(NULL,
            L"Welcome to OSD Lock Indicator!\n\n"
            L"Would you like this program to start automatically with Windows?\n\n"
            L"You can change this later by running:\n"
            L"- OsdLockIndicator.exe /install   (to enable)\n"
            L"- OsdLockIndicator.exe /uninstall (to disable)",
            L"OSD Lock Indicator - First Run Setup",
            MB_YESNO | MB_ICONQUESTION);

        if (result == IDYES) {
            AddToStartup();
        }

        MarkSetupCompleted();
    }

    // --- Create Window Class ---
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OsdLockIndicatorClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    RegisterClassExW(&wc);

    // --- Create OSD Window ---
    g_hwndOSD = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        L"OsdLockIndicatorClass", L"OSD",
        WS_POPUP,
        0, 0, OSD_WIDTH, OSD_HEIGHT,
        NULL, NULL, hInstance, NULL);

    g_currentAlpha = 0;
    UpdateOSD();

    // --- Install Keyboard Hook ---
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

    // --- Message Loop ---
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // --- Cleanup ---
    if (g_keyboardHook) UnhookWindowsHookEx(g_keyboardHook);
    GdiplusShutdown(g_gdiplusToken);
    if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }

    return 0;
}

// =============================================================================
// Position Window on Active Monitor
// =============================================================================

void CenterOnActiveMonitor(HWND hwnd)
{
    POINT pt;
    if (GetCursorPos(&pt)) {
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(hMon, &mi)) {
            int monWidth = mi.rcWork.right - mi.rcWork.left;
            int x = mi.rcWork.left + (monWidth - OSD_WIDTH) / 2;
            int y = mi.rcWork.bottom - DISTANCE_FROM_BOTTOM - OSD_HEIGHT;

            SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

// =============================================================================
// Render the OSD
// =============================================================================

void UpdateOSD()
{
    if (!g_hwndOSD) return;

    // Create off-screen bitmap
    Bitmap bmp(OSD_WIDTH, OSD_HEIGHT, PixelFormat32bppARGB);
    Graphics graphics(&bmp);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.Clear(Color(0, 0, 0, 0));

    // --- Draw Background ---
    GraphicsPath bgPath;
    CreateRoundedRectPath(bgPath, 0, 0, OSD_WIDTH, OSD_HEIGHT, CORNER_RADIUS);
    SolidBrush bgBrush(Color(BG_ALPHA, BG_RED, BG_GREEN, BG_BLUE));
    graphics.FillPath(&bgBrush, &bgPath);

    // --- Draw Text ---
    Font font(FONT_NAME, FONT_SIZE, FontStyleBold);
    SolidBrush textBrush(Color(255, TEXT_RED, TEXT_GREEN, TEXT_BLUE));
    SolidBrush onBrush(Color(255, ON_RED, ON_GREEN, ON_BLUE));
    SolidBrush offBrush(Color(255, OFF_RED, OFF_GREEN, OFF_BLUE));

    std::wstring text = g_text;
    size_t colonPos = text.find(L':');

    if (colonPos != std::wstring::npos) {
        std::wstring keyPart = text.substr(0, colonPos + 1);
        std::wstring statusPart = text.substr(colonPos + 2);

        RectF boxKey, boxWidestStatus, boxSpace;
        graphics.MeasureString(keyPart.c_str(), -1, &font, PointF(0, 0), &boxKey);
        graphics.MeasureString(L"OFF", -1, &font, PointF(0, 0), &boxWidestStatus);
        graphics.MeasureString(L" ", -1, &font, PointF(0, 0), &boxSpace);

        float spaceWidth = boxSpace.Width / 2.0f;
        float totalStableWidth = boxKey.Width + spaceWidth + boxWidestStatus.Width;
        float startX = (OSD_WIDTH - totalStableWidth) / 2.0f;
        float startY = (OSD_HEIGHT - boxKey.Height) / 2.0f;

        graphics.DrawString(keyPart.c_str(), -1, &font, PointF(startX, startY), &textBrush);
        graphics.DrawString(statusPart.c_str(), -1, &font,
            PointF(startX + boxKey.Width + spaceWidth, startY),
            (statusPart == L"ON") ? &onBrush : &offBrush);
    }

    // --- Update Layered Window ---
    HDC hdcScreen = GetDC(NULL);
    ScreenDCReleaser screenReleaser{ hdcScreen };

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    MemoryDCDeleter memDeleter{ hdcMem };

    HBITMAP hBitmap = NULL;
    bmp.GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);
    BitmapDeleter bitmapDeleter{ hBitmap };

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    GdiPlusObjectSelector selector{ hdcMem, hOldBitmap };

    SIZE size = { OSD_WIDTH, OSD_HEIGHT };
    POINT ptSrc = { 0, 0 };
    POINT ptDst = { 0, 0 };

    RECT rc;
    GetWindowRect(g_hwndOSD, &rc);
    ptDst.x = rc.left;
    ptDst.y = rc.top;

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = (BYTE)g_currentAlpha;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(g_hwndOSD, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
}

// =============================================================================
// Show Indicator with Animation
// =============================================================================

void ShowIndicator()
{
    KillTimer(g_hwndOSD, TIMER_STAY);
    KillTimer(g_hwndOSD, TIMER_ANIM);

    CenterOnActiveMonitor(g_hwndOSD);

    if (g_animState == STATE_HIDDEN || g_animState == STATE_FADING_OUT) {
        g_animState = STATE_FADING_IN;
        SetTimer(g_hwndOSD, TIMER_ANIM, ANIM_INTERVAL, NULL);
        ShowWindow(g_hwndOSD, SW_SHOWNOACTIVATE);
    }
    else if (g_animState == STATE_VISIBLE || g_animState == STATE_FADING_IN) {
        // Already visible - just reset the stay timer
        g_currentAlpha = 255;
        g_animState = STATE_VISIBLE;
        UpdateOSD();
        SetTimer(g_hwndOSD, TIMER_STAY, DISPLAY_TIME, NULL);
    }
}

// =============================================================================
// Window Procedure
// =============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    case WM_KEYSTATE_CHANGED:
    {
        UINT vkCode = static_cast<UINT>(wParam);
        const wchar_t* keyName = (vkCode == VK_CAPITAL) ? L"CapsLock" : L"NumLock";
        bool isOn = (GetKeyState(vkCode) & 0x0001) != 0;
        g_text = std::wstring(keyName) + L": " + (isOn ? L"ON" : L"OFF");

        ShowIndicator();
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_ANIM) {
            if (g_animState == STATE_FADING_IN) {
                g_currentAlpha = CalculateNextAlpha(g_currentAlpha, 255, true);
                if (g_currentAlpha >= 255) {
                    g_currentAlpha = 255;
                    g_animState = STATE_VISIBLE;
                    KillTimer(hwnd, TIMER_ANIM);
                    SetTimer(hwnd, TIMER_STAY, DISPLAY_TIME, NULL);
                }
            }
            else if (g_animState == STATE_FADING_OUT) {
                g_currentAlpha = CalculateNextAlpha(g_currentAlpha, 0, false);
                if (g_currentAlpha <= 0) {
                    g_currentAlpha = 0;
                    g_animState = STATE_HIDDEN;
                    KillTimer(hwnd, TIMER_ANIM);
                    ShowWindow(hwnd, SW_HIDE);
                }
            }
            UpdateOSD();
        }
        else if (wParam == TIMER_STAY) {
            KillTimer(hwnd, TIMER_STAY);
            g_animState = STATE_FADING_OUT;
            SetTimer(hwnd, TIMER_ANIM, ANIM_INTERVAL, NULL);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// =============================================================================
// Keyboard Hook
// =============================================================================

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_KEYUP) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;

        if (pKey->vkCode == VK_CAPITAL || pKey->vkCode == VK_NUMLOCK) {
            // Post message to handle in the main thread (avoids Sleep hack)
            PostMessage(g_hwndOSD, WM_KEYSTATE_CHANGED, pKey->vkCode, 0);
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}
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

// --- SETTINGS ---

// Key Detection Settings (true = enabled, false = disabled)
constexpr bool ENABLE_CAPSLOCK = true;    // Show indicator for Caps Lock
constexpr bool ENABLE_NUMLOCK = true;     // Show indicator for Num Lock
constexpr bool ENABLE_SCROLLLOCK = false; // Show indicator for Scroll Lock (disabled by default)

// Window Dimensions
constexpr int OSD_WIDTH = 175;
constexpr int OSD_HEIGHT = 60;
constexpr int CORNER_RADIUS = 20;
constexpr int BG_ALPHA_MAX = 80; // 0=Invisible, 255=Solid Black
constexpr int SCREEN_HEIGHT = 75; // Changes the height of the indicator on the screen.

// Animation Settings
constexpr int FADE_SPEED = 25;
constexpr int ANIM_DELAY = 10;
constexpr int STAY_TIME = 1500;

// Enums / Globals
enum AnimationState { STATE_HIDDEN, STATE_FADING_IN, STATE_VISIBLE, STATE_FADING_OUT };
AnimationState g_animState = STATE_HIDDEN;
int g_currentAlpha = 0;

HWND g_hwndOSD = NULL;
HHOOK g_keyboardHook = NULL;
std::wstring g_text = L"";
ULONG_PTR g_gdiplusToken;

constexpr UINT_PTR TIMER_ANIM = 1;
constexpr UINT_PTR TIMER_STAY = 2;

// Forward Declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void UpdateOSD();
void CenterOnActiveMonitor(HWND hwnd);
bool RemoveFromStartup();
bool TerminateOtherInstances();

// Terminate other running instances of this program
bool TerminateOtherInstances()
{
    // Get current process ID
    DWORD currentPID = GetCurrentProcessId();

    // Get the full path of the current executable
    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);

    // Convert to lowercase for comparison
    std::wstring currentPathLower = currentPath;
    for (auto& c : currentPathLower) c = towlower(c);

    bool terminatedAny = false;

    // Create snapshot of all processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    // Iterate through all processes
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            // Skip current process
            if (pe32.th32ProcessID == currentPID) {
                continue;
            }

            // Check if process name matches
            if (_wcsicmp(pe32.szExeFile, L"OsdLockIndicator.exe") == 0) {
                // Open the process to get its path
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
                    FALSE, pe32.th32ProcessID);

                if (hProcess != NULL) {
                    wchar_t processPath[MAX_PATH];
                    if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH) > 0) {
                        // Convert to lowercase for comparison
                        std::wstring processPathLower = processPath;
                        for (auto& c : processPathLower) c = towlower(c);

                        // Only terminate if it's the same executable
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

// Remove from Windows startup
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

// Main Entry Point - Updated with SAL Annotations to fix warnings
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    // these macros prevent "Unreferenced Parameter" warnings
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nShowCmd);

    // Check for uninstall command
    std::string cmdLine(lpCmdLine);
    if (cmdLine.find("/uninstall") != std::string::npos ||
        cmdLine.find("--uninstall") != std::string::npos ||
        cmdLine.find("-uninstall") != std::string::npos) {

        // Terminate any running instances
        bool terminatedProcesses = TerminateOtherInstances();

        // Small delay to ensure processes are fully terminated
        if (terminatedProcesses) {
            Sleep(500);
        }

        // Remove from startup
        bool removedStartup = RemoveFromStartup();

        if (removedStartup) {
            MessageBoxW(NULL,
                L"OSD Lock Indicator has been uninstalled:\n\n"
                L"✓ Running processes terminated\n"
                L"✓ Removed from Windows startup\n\n"
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

        return 0; // Exit without starting the program
    }

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // Prevent multiple instances
    HANDLE mutex = CreateMutexW(NULL, TRUE, L"Global\\OsdLockIndicator_Unique_ID");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    // --- STARTUP REGISTRATION (User-Prompted on First Run) ---
    // Check if already registered in startup
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {

        // Check if already registered
        if (RegQueryValueExW(hKey, L"OsdLockIndicator", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND) {
            RegCloseKey(hKey);

            // Ask user on first run
            int result = MessageBoxW(NULL,
                L"Would you like OSD Lock Indicator to start automatically with Windows?",
                L"OSD Lock Indicator",
                MB_YESNO | MB_ICONQUESTION);

            if (result == IDYES) {
                // Then add to startup
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                    0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                    wchar_t exePath[MAX_PATH];
                    GetModuleFileNameW(NULL, exePath, MAX_PATH);
                    RegSetValueExW(hKey, L"OsdLockIndicator", 0, REG_SZ,
                        (LPBYTE)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
                    RegCloseKey(hKey);
                }
            }
        }
        else {
            RegCloseKey(hKey);
        }
    }
    // ------------------------

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MultiMonOSD";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    RegisterClassExW(&wc);

    // Create window (Initially hidden/offscreen)
    g_hwndOSD = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        L"MultiMonOSD", L"OSD",
        WS_POPUP,
        0, 0, OSD_WIDTH, OSD_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    g_currentAlpha = 0;
    UpdateOSD(); // Draw initial invisible state

    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_keyboardHook) UnhookWindowsHookEx(g_keyboardHook);
    GdiplusShutdown(g_gdiplusToken);
    if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }
    return 0;
}

// Moves the window to the monitor containing the mouse cursor
void CenterOnActiveMonitor(HWND hwnd)
{
    POINT pt;
    if (GetCursorPos(&pt)) {
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(hMon, &mi)) {
            int monWidth = mi.rcWork.right - mi.rcWork.left;
            int x = mi.rcWork.left + (monWidth - OSD_WIDTH) / 2;
            int y = mi.rcWork.bottom - SCREEN_HEIGHT; // Changes the height on where it is from the bottom of the screen. 

            SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

void UpdateOSD()
{
    if (!g_hwndOSD) return;

    Bitmap bmp(OSD_WIDTH, OSD_HEIGHT, PixelFormat32bppARGB);
    Graphics graphics(&bmp);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.Clear(Color(0, 0, 0, 0));

    // Draw Background
    SolidBrush bgBrush(Color(BG_ALPHA_MAX, 0, 0, 0));

    // Create rounded rectangle path (on stack - no memory leak)
    GraphicsPath path;
    int d = CORNER_RADIUS * 2;
    path.AddArc(0, 0, d, d, 180, 90);
    path.AddArc(OSD_WIDTH - d, 0, d, d, 270, 90);
    path.AddArc(OSD_WIDTH - d, OSD_HEIGHT - d, d, d, 0, 90);
    path.AddArc(0, OSD_HEIGHT - d, d, d, 90, 90);
    path.CloseFigure();

    graphics.FillPath(&bgBrush, &path);

    // Draw Text (Stabilized)
    Font font(L"Segoe UI", 14, FontStyleBold);
    SolidBrush textBrush(Color(255, 255, 255, 255));
    SolidBrush onBrush(Color(255, 0, 255, 0));
    SolidBrush offBrush(Color(255, 255, 50, 50));

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

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = NULL;
    bmp.GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    SIZE size = { OSD_WIDTH, OSD_HEIGHT };
    POINT ptSrc = { 0, 0 };
    POINT ptDst = { 0, 0 };
    RECT rc; GetWindowRect(g_hwndOSD, &rc);
    ptDst.x = rc.left;
    ptDst.y = rc.top;

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = (BYTE)g_currentAlpha;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(g_hwndOSD, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_TIMER:
        if (wParam == TIMER_ANIM) {
            if (g_animState == STATE_FADING_IN) {
                g_currentAlpha += FADE_SPEED;
                if (g_currentAlpha >= 255) {
                    g_currentAlpha = 255;
                    g_animState = STATE_VISIBLE;
                    KillTimer(hwnd, TIMER_ANIM);
                    SetTimer(hwnd, TIMER_STAY, STAY_TIME, NULL);
                }
            }
            else if (g_animState == STATE_FADING_OUT) {
                g_currentAlpha -= FADE_SPEED;
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
            SetTimer(hwnd, TIMER_ANIM, ANIM_DELAY, NULL);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_KEYUP) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;

        // Check if the key is one we're monitoring and if it's enabled
        bool shouldProcess = false;
        const wchar_t* keyName = nullptr;

        if (pKey->vkCode == VK_CAPITAL && ENABLE_CAPSLOCK) {
            shouldProcess = true;
            keyName = L"CapsLock";
        }
        else if (pKey->vkCode == VK_NUMLOCK && ENABLE_NUMLOCK) {
            shouldProcess = true;
            keyName = L"NumLock";
        }
        else if (pKey->vkCode == VK_SCROLL && ENABLE_SCROLLLOCK) {
            shouldProcess = true;
            keyName = L"ScrollLock";
        }

        if (shouldProcess && keyName != nullptr) {
            Sleep(50);

            bool isOn = (GetKeyState(pKey->vkCode) & 0x0001) != 0;
            g_text = std::wstring(keyName) + L": " + (isOn ? L"ON" : L"OFF");

            KillTimer(g_hwndOSD, TIMER_STAY);

            if (g_animState == STATE_HIDDEN || g_animState == STATE_FADING_OUT) {
                CenterOnActiveMonitor(g_hwndOSD);
                g_animState = STATE_FADING_IN;
                SetTimer(g_hwndOSD, TIMER_ANIM, ANIM_DELAY, NULL);
                ShowWindow(g_hwndOSD, SW_SHOWNOACTIVATE);
            }
            else if (g_animState == STATE_VISIBLE || g_animState == STATE_FADING_IN) {
                CenterOnActiveMonitor(g_hwndOSD);
                g_currentAlpha = 255;
                g_animState = STATE_VISIBLE;
                UpdateOSD();
                SetTimer(g_hwndOSD, TIMER_STAY, STAY_TIME, NULL);
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}
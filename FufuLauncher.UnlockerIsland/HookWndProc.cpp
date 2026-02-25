#include "HookWndProc.h"
#include "GamepadHotSwitch.h"
#include "Config.h"
#include "Scanner.h"
#include "EncryptedData.h"
#include <iostream>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

static HWND g_hUnityWindow = nullptr;
static bool g_subclassInstalled = false;
static UINT_PTR g_subclassId = 1;

LPVOID switchInputDeviceToTouchScreen = nullptr;
LPVOID switchInputDeviceToJoypad = nullptr;
LPVOID switchInputDeviceToKeyboard = nullptr;

LRESULT CALLBACK WindowSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (GamepadHotSwitch::GetInstance().IsEnabled())
        {
            GamepadHotSwitch::GetInstance().ProcessWindowMessage(uMsg, wParam, lParam);
        }
        break;
        
    case WM_GAMEPAD_ACTIVATED:
        HandleSwitchToGamepad();
        return 0;
        
    case WM_MOUSE_ACTIVATED:
        HandleSwitchToKeyboardMouse();
        return 0;
    }
    
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void HandleSwitchToGamepad()
{
    if (switchInputDeviceToJoypad)
    {
        typedef void(*SwitchInputDeviceToJoypadFn)(void*);
        SwitchInputDeviceToJoypadFn switchInput = (SwitchInputDeviceToJoypadFn)switchInputDeviceToJoypad;
        
        __try
        {
            if (Config::Get().debug_console)
                std::cout << "[HookWndProc] Switched to gamepad input" << '\n';
            switchInput(nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {

        }
    }
    else
    {
        if (Config::Get().debug_console)
            std::cout << "[HookWndProc] switchInputDeviceToJoypad function not available" << '\n';
    }
}

void HandleSwitchToKeyboardMouse()
{
    if (switchInputDeviceToKeyboard)
    {
        typedef void(*SwitchInputDeviceToKeyboardMouseFn)(void*);
        SwitchInputDeviceToKeyboardMouseFn switchInput = (SwitchInputDeviceToKeyboardMouseFn)switchInputDeviceToKeyboard;
        
        __try
        {
            if (Config::Get().debug_console)
                std::cout << "[HookWndProc] Switched to keyboard/mouse input" << '\n';
            switchInput(nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {

        }
    }
    else
    {
        if (Config::Get().debug_console)
            std::cout << "[HookWndProc] switchInputDeviceToTouchScreen function not available" << '\n';
    }
}
 
bool InstallWindowSubclass()
{
    if (!g_hUnityWindow || g_subclassInstalled)
    {
        return false;
    }
    
    if (SetWindowSubclass(g_hUnityWindow, WindowSubclassProc, g_subclassId, 0))
    {
        g_subclassInstalled = true;
        if (Config::Get().debug_console)
            std::cout << "[HookWndProc] Window subclass installed successfully" << '\n';
        return true;
    }
    
    DWORD error = GetLastError();
    if (Config::Get().debug_console)
        std::cout << "[HookWndProc] Failed to install window subclass: " << error << '\n';
    return false;
}

bool RemoveWindowSubclass()
{
    if (!g_hUnityWindow || !g_subclassInstalled)
    {
        return false;
    }
    
    if (::RemoveWindowSubclass(g_hUnityWindow, WindowSubclassProc, g_subclassId))
    {
        g_subclassInstalled = false;
        if (Config::Get().debug_console)
            std::cout << "[HookWndProc] Window subclass removed successfully" << '\n';
        return true;
    }
    
    DWORD error = GetLastError();
    if (Config::Get().debug_console)
        std::cout << "[HookWndProc] Failed to remove window subclass: " << error << '\n';
    return false;
}

void SetUnityMainWindow(HWND hWnd)
{
    if (g_hUnityWindow != hWnd)
    {
        if (g_subclassInstalled)
        {
            RemoveWindowSubclass();
        }
        
        g_hUnityWindow = hWnd;
        
        if (g_hUnityWindow)
        {
            InstallWindowSubclass();
        }
    }
}

HWND GetUnityMainWindow()
{
    return g_hUnityWindow;
}

void InitializeWndProcHooks()
{
    if (!switchInputDeviceToJoypad)
    {
        std::string pattern = XorString::decrypt(EncryptedPatterns::SwitchInputDeviceToJoypad);
        switchInputDeviceToJoypad = Scanner::ScanMainMod(pattern.c_str());
        if (Config::Get().debug_console)
        {
            if (switchInputDeviceToJoypad)
                std::cout << "[HookWndProc] Found SwitchInputDeviceToJoypad at: " << switchInputDeviceToJoypad << '\n';
            else
                std::cout << "[HookWndProc] Failed to find SwitchInputDeviceToJoypad" << '\n';
        }
    }
    
    if (!switchInputDeviceToKeyboard)
    {
        std::string pattern = XorString::decrypt(EncryptedPatterns::SwitchInputDeviceToKeyboard);
        switchInputDeviceToKeyboard = Scanner::ScanMainMod(pattern.c_str());
        if (Config::Get().debug_console)
        {
            if (switchInputDeviceToKeyboard)
                std::cout << "[HookWndProc] Found SwitchInputDeviceToKeyboard at: " << switchInputDeviceToKeyboard <<
                    '\n';
            else
                std::cout << "[HookWndProc] Failed to find SwitchInputDeviceToKeyboard" << '\n';
        }
    }
    
    HWND hWnd = FindUnityMainWindow();
    if (hWnd)
    {
        SetUnityMainWindow(hWnd);
    }
    else
    {
        if (Config::Get().debug_console)
            std::cout << "[HookWndProc] Unity main window not found during initialization" << '\n';
    }
}

HWND FindUnityMainWindow()
{
    HWND result = nullptr;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&result));
    return result;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    if (!IsWindowVisible(hWnd))
        return TRUE;
    
    wchar_t className[256];
    GetClassNameW(hWnd, className, 256);
    
    std::wstring classNameStr(className);
    if (classNameStr.find(L"UnityWndClass") != std::wstring::npos || 
        classNameStr.find(L"Unity") != std::wstring::npos)
    {
        wchar_t windowTitle[256];
        GetWindowTextW(hWnd, windowTitle, 256);
        if (wcslen(windowTitle) > 0)
        {
            HWND* pResult = reinterpret_cast<HWND*>(lParam);
            *pResult = hWnd;
            return FALSE;
        }
    }
    return TRUE;
}

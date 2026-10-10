/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "FreeCamera.h"

#include "../Camera/Camera.h"
#include "../Config/Config.h"

#include <atomic>
#include <iostream>
#include <Windows.h>

namespace FreeCamera
{
    namespace
    {
        std::atomic<bool> g_Ready{false};
        std::atomic<bool> g_Active{false};
        std::atomic<bool> g_Locked{false};
        volatile float g_Yaw = 0.0f, g_Pitch = 0.0f;
        Vector3 g_FreeCamPos = {0, 0, 0};
        Vector3 g_LastRealPos = {0, 0, 0};
        std::atomic<bool> g_NeedsInitialPosition{false};

        volatile LONG g_MouseDX = 0;
        volatile LONG g_MouseDY = 0;
        HWND g_GameWindow = nullptr;
        WNDPROC g_OldWndProc = nullptr;
        HHOOK g_KbHook = nullptr;

        bool IsFlightKey(DWORD key)
        {
            auto& config = Config::Get();
            return g_Active.load(std::memory_order_relaxed) &&
                !g_Locked.load(std::memory_order_relaxed) &&
                key != static_cast<DWORD>(config.free_cam_key) &&
                key != static_cast<DWORD>(config.free_cam_lock_key);
        }

        LRESULT CALLBACK KbProc(int nCode, WPARAM wParam, LPARAM lParam)
        {
            if (nCode >= 0 && lParam)
            {
                auto* keyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
                if (IsFlightKey(keyboard->vkCode)) return 1;
            }
            return CallNextHookEx(g_KbHook, nCode, wParam, lParam);
        }

        LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (g_Active.load(std::memory_order_relaxed) &&
                !g_Locked.load(std::memory_order_relaxed))
            {
                if (msg == WM_KEYDOWN || msg == WM_KEYUP ||
                    msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)
                {
                    if (IsFlightKey(static_cast<DWORD>(wParam))) return 0;
                }
                if (msg == WM_INPUT)
                {
                    UINT size = 0;
                    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
                                    RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
                    if (size > 0 && size <= 64)
                    {
                        BYTE buffer[64];
                        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
                                            RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size)
                        {
                            RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);
                            if (raw->header.dwType == RIM_TYPEMOUSE &&
                                !(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
                            {
                                InterlockedExchangeAdd(&g_MouseDX,
                                                       raw->data.mouse.lLastX);
                                InterlockedExchangeAdd(&g_MouseDY,
                                                       raw->data.mouse.lLastY);
                                return 0;
                            }
                            if (raw->header.dwType == RIM_TYPEKEYBOARD &&
                                IsFlightKey(raw->data.keyboard.VKey))
                            {
                                return 0;
                            }
                        }
                    }
                }
            }
            return g_OldWndProc
                       ? CallWindowProcW(g_OldWndProc, hwnd, msg, wParam, lParam)
                       : DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        void InitRawMouseInput(HWND hwnd)
        {
            g_GameWindow = hwnd;
            g_OldWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
                hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook)));

            RAWINPUTDEVICE devices[2] = {};
            devices[0].usUsagePage = 0x01;
            devices[0].usUsage = 0x02;
            devices[0].dwFlags = RIDEV_INPUTSINK;
            devices[0].hwndTarget = hwnd;
            devices[1].usUsagePage = 0x01;
            devices[1].usUsage = 0x06;
            devices[1].dwFlags = RIDEV_INPUTSINK;
            devices[1].hwndTarget = hwnd;
            RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
        }

        void ApplyNow()
        {
            if (!g_Active.load(std::memory_order_relaxed) ||
                !Camera::GetTransform())
                return;

            if (g_NeedsInitialPosition.load(std::memory_order_relaxed))
            {
                Vector3 currentPosition{};
                if (!Camera::GetPosition(currentPosition)) return;
                g_FreeCamPos = currentPosition;
                g_LastRealPos = currentPosition;
                g_NeedsInitialPosition.store(false, std::memory_order_relaxed);
            }

            Camera::Quaternion rotation = Camera::FromYawPitch(g_Yaw, g_Pitch);
            Camera::SetPosition(g_FreeCamPos);
            Camera::SetRotation(rotation);
        }

        void ToggleActive()
        {
            bool active = !g_Active.load(std::memory_order_relaxed);
            g_Active.store(active, std::memory_order_relaxed);
            if (active)
            {
                g_Locked.store(false, std::memory_order_relaxed);
                g_NeedsInitialPosition.store(true, std::memory_order_relaxed);
                InterlockedExchange(&g_MouseDX, 0);
                InterlockedExchange(&g_MouseDY, 0);
                ShowCursor(FALSE);
            }
            else
            {
                g_Locked.store(false, std::memory_order_relaxed);
                g_NeedsInitialPosition.store(false, std::memory_order_relaxed);
                ShowCursor(TRUE);
            }
        }

        void ToggleLock()
        {
            bool locked = !g_Locked.load(std::memory_order_relaxed);
            g_Locked.store(locked, std::memory_order_relaxed);
            InterlockedExchange(&g_MouseDX, 0);
            InterlockedExchange(&g_MouseDY, 0);
            ShowCursor(locked ? TRUE : FALSE);
        }

        DWORD WINAPI InputThread(LPVOID)
        {
            bool previousToggle = false;
            bool previousLock = false;
            LARGE_INTEGER frequency, previousTime, currentTime;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&previousTime);

            HWND hwnd = nullptr;
            while (!(hwnd = FindWindowA("UnityWndClass", nullptr))) Sleep(500);
            Sleep(15000);
            InitRawMouseInput(hwnd);

            while (true)
            {
                Sleep(10);
                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                auto& config = Config::Get();

                if (!g_Ready.load(std::memory_order_relaxed) ||
                    !config.enable_free_cam)
                {
                    if (g_Active.load(std::memory_order_relaxed)) ToggleActive();
                    previousToggle = false;
                    previousLock = false;
                    QueryPerformanceCounter(&previousTime);
                    continue;
                }

                bool toggle =
                    (GetAsyncKeyState(config.free_cam_key) & 0x8000) != 0;
                if (toggle && !previousToggle) ToggleActive();
                previousToggle = toggle;

                bool lockKey =
                    (GetAsyncKeyState(config.free_cam_lock_key) & 0x8000) != 0;
                if (lockKey && !previousLock &&
                    g_Active.load(std::memory_order_relaxed))
                    ToggleLock();
                previousLock = lockKey;

                QueryPerformanceCounter(&currentTime);
                float deltaSeconds = static_cast<float>(
                        currentTime.QuadPart - previousTime.QuadPart) /
                    static_cast<float>(frequency.QuadPart);
                previousTime = currentTime;
                if (deltaSeconds > 0.1f) deltaSeconds = 0.1f;

                if (!g_Active.load(std::memory_order_relaxed) ||
                    g_Locked.load(std::memory_order_relaxed))
                    continue;

                LONG deltaX = InterlockedExchange(&g_MouseDX, 0);
                LONG deltaY = InterlockedExchange(&g_MouseDY, 0);
                g_Yaw += static_cast<float>(deltaX) *
                    config.free_cam_mouse_sensitivity;
                g_Pitch += static_cast<float>(deltaY) *
                    config.free_cam_mouse_sensitivity;
                if (g_Pitch > 89.0f) g_Pitch = 89.0f;
                if (g_Pitch < -89.0f) g_Pitch = -89.0f;

                Camera::Quaternion rotation =
                    Camera::FromYawPitch(g_Yaw, g_Pitch);
                Vector3 forward =
                    Camera::RotateVector(rotation, {0, 0, 1});
                Vector3 right =
                    Camera::RotateVector(rotation, {1, 0, 0});

                float speed = config.free_cam_move_speed;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    speed *= config.free_cam_sprint_mult;
                }
                float step = speed * deltaSeconds;

                Vector3 position = g_FreeCamPos;
                if (GetAsyncKeyState('W') & 0x8000)
                {
                    position.x += forward.x * step;
                    position.y += forward.y * step;
                    position.z += forward.z * step;
                }
                if (GetAsyncKeyState('S') & 0x8000)
                {
                    position.x -= forward.x * step;
                    position.y -= forward.y * step;
                    position.z -= forward.z * step;
                }
                if (GetAsyncKeyState('D') & 0x8000)
                {
                    position.x += right.x * step;
                    position.y += right.y * step;
                    position.z += right.z * step;
                }
                if (GetAsyncKeyState('A') & 0x8000)
                {
                    position.x -= right.x * step;
                    position.y -= right.y * step;
                    position.z -= right.z * step;
                }
                if (GetAsyncKeyState(VK_SPACE) & 0x8000) position.y += step;
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) position.y -= step;
                g_FreeCamPos = position;
            }
            return 0;
        }
    }

    void Init()
    {
        std::cout << "[SCAN] Initializing FreeCamera..." << std::endl;
        if (!Camera::IsReady())
        {
            std::cout << "   -> [ERR] FreeCamera disabled: shared camera access is unavailable."
                << std::endl;
            return;
        }

        g_KbHook = SetWindowsHookExA(
            WH_KEYBOARD_LL, KbProc, GetModuleHandleA(nullptr), 0);
        g_Ready.store(true, std::memory_order_relaxed);
        std::cout << "   -> FreeCamera ready." << std::endl;

        CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);
        std::cout << "   -> Free-camera input window hook scheduled." << std::endl;
    }

    bool IsActive()
    {
        return g_Active.load(std::memory_order_relaxed);
    }

    void Tick()
    {
        if (!g_Ready.load(std::memory_order_relaxed) ||
            !g_Active.load(std::memory_order_relaxed))
            return;
        ApplyNow();
    }
}

/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "RainbowDamage.h"

#include <atomic>
#include <iostream>

namespace RainbowDamageFeature
{
    static Color g_palette[] = {
        {0.2f, 0.9f, 0.1f, 1.0f},
        {1.0f, 0.3f, 0.3f, 1.0f},
        {0.3f, 0.5f, 1.0f, 1.0f},
        {1.0f, 0.85f, 0.1f, 1.0f},
        {0.8f, 0.2f, 1.0f, 1.0f},
        {0.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 0.5f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
    };
    static constexpr int PALETTE_COUNT = sizeof(g_palette) / sizeof(Color);
    static volatile int g_colorIdx = 0;

    static constexpr int MaxExceptionCount = 3;
    static std::atomic<int> g_exceptionCount{0};
    static std::atomic<bool> g_forceFallback{false};

    FnGetColorList g_oGetColorA = nullptr;
    FnGetColorArr g_oGetColorB = nullptr;
    FnGetColorIdx g_oGetColor1 = nullptr;
    FnGetColorIdx g_oGetColor2 = nullptr;
    FnGetColorIdx g_oGetColor3 = nullptr;
    FnGetColorIdx g_oGetColor4 = nullptr;

    static void OnException(const char* stage)
    {
        int count = g_exceptionCount.fetch_add(1, std::memory_order_relaxed) + 1;
        std::cout << "[ERR] RainbowDamage exception in " << stage
            << " (count: " << count << "/" << MaxExceptionCount << ")" << std::endl;
        if (count >= MaxExceptionCount)
        {
            g_forceFallback.store(true, std::memory_order_relaxed);
            std::cout << "[WARN] RainbowDamage disabled due to repeated exceptions, falling back to original." <<
                std::endl;
        }
    }

    static Color GetTargetColor()
    {
        if (Config::Get().rainbow_damage_mode == 1)
        {
            int fixedIdx = Config::Get().rainbow_fixed_color_idx % PALETTE_COUNT;
            return g_palette[fixedIdx];
        }
        return g_palette[g_colorIdx];
    }

    void __fastcall HookGetColorA(Color* ret, void* self, void* list, int idx, void* method)
    {
        if (!Config::Get().enable_rainbow_damage)
        {
            g_oGetColorA(ret, self, list, idx, method);
            return;
        }
        __try
        {
            g_oGetColorA(ret, self, list, idx, method);
            if (!g_forceFallback.load(std::memory_order_relaxed))
            {
                *ret = GetTargetColor();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OnException("ColorA");
        }
    }

    void __fastcall HookGetColorB(Color* ret, void* self, void* arr, int idx, void* method)
    {
        if (!Config::Get().enable_rainbow_damage)
        {
            g_oGetColorB(ret, self, arr, idx, method);
            return;
        }
        __try
        {
            g_oGetColorB(ret, self, arr, idx, method);
            if (!g_forceFallback.load(std::memory_order_relaxed))
            {
                *ret = GetTargetColor();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OnException("ColorB");
        }
    }

    void __fastcall HookGetColor1(Color* ret, void* self, int idx, void* method)
    {
        if (!Config::Get().enable_rainbow_damage)
        {
            g_oGetColor1(ret, self, idx, method);
            return;
        }
        __try
        {
            g_oGetColor1(ret, self, idx, method);
            if (!g_forceFallback.load(std::memory_order_relaxed))
            {
                *ret = GetTargetColor();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OnException("Color1");
        }
    }

    void __fastcall HookGetColor2(Color* ret, void* self, int idx, void* method)
    {
        if (!Config::Get().enable_rainbow_damage)
        {
            g_oGetColor2(ret, self, idx, method);
            return;
        }
        __try
        {
            g_oGetColor2(ret, self, idx, method);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OnException("Color2");
        }
    }

    void __fastcall HookGetColor3(Color* ret, void* self, int idx, void* method)
    {
        if (!Config::Get().enable_rainbow_damage)
        {
            g_oGetColor3(ret, self, idx, method);
            return;
        }
        __try
        {
            g_oGetColor3(ret, self, idx, method);
            if (!g_forceFallback.load(std::memory_order_relaxed))
            {
                *ret = GetTargetColor();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OnException("Color3");
        }
    }

    void __fastcall HookGetColor4(Color* ret, void* self, int idx, void* method)
    {
        if (!Config::Get().enable_rainbow_damage)
        {
            g_oGetColor4(ret, self, idx, method);
            return;
        }
        __try
        {
            g_oGetColor4(ret, self, idx, method);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OnException("Color4");
        }
    }

    DWORD WINAPI ColorCycleThread(LPVOID)
    {
        while (true)
        {
            Sleep(2000);
            if (!Config::Get().enable_rainbow_damage) continue;
            if (Config::Get().rainbow_damage_mode == 0)
            {
                g_colorIdx = (g_colorIdx + 1) % PALETTE_COUNT;
            }
        }
        return 0;
    }
}

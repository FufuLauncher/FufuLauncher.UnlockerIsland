/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include "../Core/SharedState.h"
#include "../Config/Config.h"

namespace RainbowDamageFeature
{
    struct Color
    {
        float r, g, b, a;
    };

    typedef void (__fastcall *FnGetColorList)(Color* ret, void* self, void* list, int idx, void* method);
    typedef void (__fastcall *FnGetColorArr)(Color* ret, void* self, void* arr, int idx, void* method);
    typedef void (__fastcall *FnGetColorIdx)(Color* ret, void* self, int idx, void* method);

    extern FnGetColorList g_oGetColorA;
    extern FnGetColorArr g_oGetColorB;
    extern FnGetColorIdx g_oGetColor1;
    extern FnGetColorIdx g_oGetColor2;
    extern FnGetColorIdx g_oGetColor3;
    extern FnGetColorIdx g_oGetColor4;

    void __fastcall HookGetColorA(Color* ret, void* self, void* list, int idx, void* method);
    void __fastcall HookGetColorB(Color* ret, void* self, void* arr, int idx, void* method);
    void __fastcall HookGetColor1(Color* ret, void* self, int idx, void* method);
    void __fastcall HookGetColor2(Color* ret, void* self, int idx, void* method);
    void __fastcall HookGetColor3(Color* ret, void* self, int idx, void* method);
    void __fastcall HookGetColor4(Color* ret, void* self, int idx, void* method);
    DWORD WINAPI ColorCycleThread(LPVOID);
}

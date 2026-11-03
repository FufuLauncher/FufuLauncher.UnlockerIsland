/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <cwchar>
#include <iostream>
#include <intrin.h>
#include "DomainMap.h"
#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"
#include "../MinHook/MinHook.h"

namespace
{
    std::atomic<bool> g_Enabled{false}, g_Ready{false}, g_Faulted{false};
#if defined(_M_X64)
    using SourceJump = void (*)(void*, uint32_t, void*);
    using TypeCheck = bool (*)(int, void*);
    SourceJump g_OriginalSource = nullptr;
    TypeCheck g_OriginalType = nullptr;
    const unsigned char* const* g_Modules = nullptr;
    void* g_DailyCallers[2]{};
    thread_local bool g_SourceMapJump = false;

    template <typename T>
    T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const unsigned char*>(object) + offset);
    }

    void* Object(const void* object, size_t offset)
    {
        return object ? Read<void*>(object, offset) : nullptr;
    }

    bool StableDomain()
    {
        using namespace Patterns::DomainMap;
        const auto modules = *g_Modules;
        if (!modules) return false;
        const auto scene = Object(Object(modules, ModuleSceneSlot), SceneRootToScene);
        const auto script = Object(scene, SceneToScript);
        const auto transition = Object(Object(modules, ModuleTransitionSlot), TransitionRootToObject);
        const auto dungeon = Object(modules, ModuleDungeonSlot);
        if (!scene || !script || !transition || !dungeon) return false;
        const auto dungeonId = Read<uint32_t>(dungeon, DungeonId);
        return Read<int>(script, ScriptState) == 1 && Read<int>(scene, SceneState) == 4 &&
            Read<int>(transition, TransitionState) == 10 && dungeonId != 0 &&
            dungeonId == Read<uint32_t>(transition, TransitionDungeonId);
    }

    void HookSourceJump(void* instance, uint32_t sourceId, void* methodInfo)
    {
        const bool previous = g_SourceMapJump;
        g_SourceMapJump = g_Enabled.load() && g_Ready.load() && !g_Faulted.load() && sourceId != 0;
        __try { g_OriginalSource(instance, sourceId, methodInfo); }
        __finally { g_SourceMapJump = previous; }
    }

    bool HookTypeCheck(int type, void* methodInfo)
    {
        const auto caller = _ReturnAddress();
        const bool native = g_OriginalType(type, methodInfo);
        if (!native || type != 3 || !g_SourceMapJump || !g_Enabled.load() || !g_Ready.load() || g_Faulted.load() ||
            (caller != g_DailyCallers[0] && caller != g_DailyCallers[1]))
            return native;
        __try { if (StableDomain() && g_Enabled.load() && !g_Faulted.load()) return false; }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_Faulted.store(true);
            OutputDebugStringA("[DomainMap] Source state unavailable; native restriction retained.\n");
        }
        return native;
    }
#endif
}

void DomainMap::SetEnabled(bool enabled) { g_Enabled.store(enabled); }

void DomainMap::Init()
{
#if defined(_M_X64)
    using namespace Patterns::DomainMap;
    auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    const auto slash = std::wcsrchr(path, L'\\');
    if (!length || length >= MAX_PATH || _wcsicmp(slash ? slash + 1 : path, L"YuanShen.exe") != 0 ||
        dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 || nt->FileHeader.TimeDateStamp != TimeDateStamp ||
        nt->OptionalHeader.SizeOfImage != SizeOfImage || nt->OptionalHeader.CheckSum != CheckSum)
    {
        std::cout << "[DomainMap] Unsupported build; native source restrictions retained.\n";
        return;
    }
    const auto find = [](const char* pattern, void* within = nullptr, size_t size = 0)
    {
        return static_cast<unsigned char*>(Scanner::ScanUniqueMainMod(pattern, within, size));
    };
    auto source = find(SourceEntry);
    auto type = find(DungeonTypeCheck);
    auto guard = source
                     ? static_cast<unsigned char*>(Scanner::ResolveRelative(source + GuardCallInSource, 1, 5))
                     : nullptr;
    if (!source || !type || !guard || find(SourceGuard, guard, 0x100) != guard) return;
    for (unsigned i = 0; i < 2; ++i)
    {
        auto call = guard + (i == 0 ? DailyCallA : DailyCallB);
        if (find(DailyFightCall, call, 12) != call || Scanner::ResolveRelative(call + 5, 1, 5) != type) return;
        g_DailyCallers[i] = call + DailyCallReturnBias;
    }
    auto modules = static_cast<unsigned char*>(Scanner::ResolveRelative(type + TypeToModules, 3, 7));
    if (modules != Scanner::ResolveRelative(guard + GuardToModules, 3, 7) || modules < base ||
        modules + sizeof(void*) > base + SizeOfImage)
        return;
    g_Modules = reinterpret_cast<const unsigned char* const*>(modules);
    if (MH_CreateHook(source, reinterpret_cast<void*>(&HookSourceJump),
                      reinterpret_cast<void**>(&g_OriginalSource)) != MH_OK)
        return;
    if (MH_CreateHook(type, reinterpret_cast<void*>(&HookTypeCheck), reinterpret_cast<void**>(&g_OriginalType)) !=
        MH_OK)
    {
        MH_RemoveHook(source);
        return;
    }
    g_Ready.store(true);
    std::cout << "[DomainMap] Domain item-source navigation ready (default off).\n";
#endif
}

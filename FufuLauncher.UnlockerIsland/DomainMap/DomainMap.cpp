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
#include "../Core/Compatibility.h"
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
        using namespace Offsets::DomainMap;
        const bool os = Offsets::IsOS;
        const auto modules = *g_Modules;
        if (!modules) return false;
        const auto scene = Object(Object(modules, os ? ModuleSceneSlotOS : ModuleSceneSlot),
                                  os ? SceneRootToSceneOS : SceneRootToScene);
        const auto script = Object(scene, SceneToScript);
        const auto transition = Object(Object(modules, os ? ModuleTransitionSlotOS : ModuleTransitionSlot),
                                       os ? TransitionRootToObjectOS : TransitionRootToObject);
        const auto dungeon = Object(modules, ModuleDungeonSlot);
        if (!scene || !script || !transition || !dungeon) return false;
        const auto dungeonId = Read<uint32_t>(dungeon, os ? DungeonIdOS : DungeonId);
        return Read<int>(script, ScriptState) == 1 && Read<int>(scene, SceneState) == 4 &&
            Read<int>(transition, os ? TransitionStateOS : TransitionState) == 10 && dungeonId != 0 &&
            dungeonId == Read<uint32_t>(transition, os ? TransitionDungeonIdOS : TransitionDungeonId);
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
    using namespace Offsets::DomainMap;
    auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const auto imageSize = Compatibility::GetImageSize();
    if (!imageSize)
    {
        std::cout << "[DomainMap] Unsupported build; native source restrictions retained.\n";
        return;
    }
    const auto find = [](const char* pattern, void* within = nullptr, size_t size = 0)
    {
        return static_cast<unsigned char*>(Scanner::ScanUniqueMainMod(pattern, within, size));
    };
    auto source = find(SourceEntry);
    auto type = find(Offsets::IsOS ? DungeonTypeCheckOS : DungeonTypeCheck);
    auto guard = source
                     ? static_cast<unsigned char*>(Scanner::ResolveRelative(source + GuardCallInSource, 1, 5))
                     : nullptr;
    if (!source || !type || !guard ||
        find(Offsets::IsOS ? SourceGuardOS : SourceGuard, guard, GuardCheckSize) != guard) return;
    for (unsigned i = 0; i < 2; ++i)
    {
        auto call = guard + (i == 0 ? DailyCallA : DailyCallB);
        if (find(DailyFightCall, call, DailyCallCheckSize) != call || Scanner::ResolveRelative(call + 5, 1, 5) != type) return;
        g_DailyCallers[i] = call + DailyCallReturnBias;
    }
    auto modules = static_cast<unsigned char*>(Scanner::ResolveRelative(type + TypeToModules, 3, 7));
    if (modules != Scanner::ResolveRelative(guard + GuardToModules, 3, 7) || modules < base ||
        modules + sizeof(void*) > base + imageSize)
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

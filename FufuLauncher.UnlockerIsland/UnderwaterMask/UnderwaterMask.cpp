/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "UnderwaterMask.h"

#include "../Config/Config.h"
#include "../Core/Utils.h"
#include "../MinHook/MinHook.h"
#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"

#include <atomic>
#include <climits>
#include <cstdint>
#include <iostream>
#include <Psapi.h>

namespace UnderwaterMask
{
    namespace
    {
        using MaskFunction = std::int64_t(__fastcall*)(void*, double);
        using ClearFunction = void(__fastcall*)(void*);

        constexpr uintptr_t ClearSearchWindow = 0x8000;
        constexpr int MaxExceptionCount = 3;

        MaskFunction g_origPre = nullptr;
        MaskFunction g_origMain = nullptr;
        MaskFunction g_origPost = nullptr;
        ClearFunction g_clearMask = nullptr;

        std::atomic<int> g_exceptionCount{0};
        std::atomic<bool> g_forceFallback{false};

        bool IsDisabled()
        {
            if (g_forceFallback.load(std::memory_order_relaxed))
            {
                return false;
            }
            return Config::Get().disable_underwater_mask;
        }

        void OnException(const char* stage)
        {
            int count = g_exceptionCount.fetch_add(1, std::memory_order_relaxed) + 1;
            std::cout << "[ERR] UnderwaterMask exception in " << stage
                << " (count: " << count << "/" << MaxExceptionCount << ")" << std::endl;
            if (count >= MaxExceptionCount)
            {
                g_forceFallback.store(true, std::memory_order_relaxed);
                std::cout << "[WARN] UnderwaterMask disabled due to repeated exceptions, falling back to original." <<
                    std::endl;
            }
        }

        void InvokeClearMask(void* thisPtr)
        {
            if (!thisPtr || !IsValid(g_clearMask))
            {
                return;
            }

            __try
            {
                g_clearMask(thisPtr);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                OnException("ClearMask");
            }
        }

        std::int64_t InvokeOriginal(MaskFunction original, void* thisPtr, double deltaTime)
        {
            if (!original)
            {
                return 0;
            }
            __try
            {
                return original(thisPtr, deltaTime);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return 0;
            }
        }

        std::int64_t InvokeOrClear(MaskFunction original, void* thisPtr, double deltaTime, const char* stage)
        {
            if (!IsDisabled())
            {
                return InvokeOriginal(original, thisPtr, deltaTime);
            }

            __try
            {
                InvokeClearMask(thisPtr);
                return 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                OnException(stage);
                return InvokeOriginal(original, thisPtr, deltaTime);
            }
        }

        std::int64_t __fastcall HookPre(void* thisPtr, double deltaTime)
        {
            return InvokeOrClear(g_origPre, thisPtr, deltaTime, "Pre");
        }

        std::int64_t __fastcall HookMain(void* thisPtr, double deltaTime)
        {
            return InvokeOrClear(g_origMain, thisPtr, deltaTime, "Main");
        }

        std::int64_t __fastcall HookPost(void* thisPtr, double deltaTime)
        {
            return InvokeOrClear(g_origPost, thisPtr, deltaTime, "Post");
        }

        void* ScanDirect(const char* name, const char* pattern)
        {
            std::cout << "[SCAN] " << name << "..." << std::endl;

            void* address = Scanner::ScanMainMod(pattern);
            if (!address)
            {
                std::cout << "   -> [ERR] Pattern Not Found." << std::endl;
                return nullptr;
            }

            const auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
            std::cout << "   -> Found at: 0x"
                << std::hex
                << (reinterpret_cast<uintptr_t>(address) - moduleBase)
                << std::dec
                << std::endl;
            return address;
        }

        constexpr int kMaxSlots = 32;
        constexpr int32_t kNoDisp = INT_MIN;

        struct SlotList
        {
            uintptr_t fn;
            int32_t slots[kMaxSlots];
            int count;
        };

        bool HasSlot(const SlotList& list, int32_t offset)
        {
            for (int i = 0; i < list.count; ++i)
            {
                if (list.slots[i] == offset)
                {
                    return true;
                }
            }
            return false;
        }

        bool CollectClearedSlots(uintptr_t fn, uintptr_t limit, SlotList& out)
        {
            __try
            {
                const std::uint8_t* code = reinterpret_cast<const std::uint8_t*>(fn);
                std::int32_t lastLea[8];
                for (int i = 0; i < 8; ++i)
                {
                    lastLea[i] = kNoDisp;
                }

                std::size_t i = 0;
                while (i + 7 <= limit)
                {
                    //lea r64, [r64+disp32] == REX.W 8D /r with mod=10
                    if (code[i] == 0x48 && code[i + 1] == 0x8D && (code[i + 2] >> 6) == 2)
                    {
                        const int reg = (code[i + 2] >> 3) & 7;
                        lastLea[reg] = *reinterpret_cast<const std::int32_t*>(code + i + 3);
                        i += 7;
                        continue;
                    }

                    //mov dword ptr [reg(+disp32)], 0FFFFFFFFh == C7 /0, imm32 = -1
                    if (code[i] == 0xC7 && ((code[i + 1] >> 3) & 7) == 0)
                    {
                        const std::uint8_t modrm = code[i + 1];
                        const int mod = modrm >> 6;
                        const int rm = modrm & 7;
                        std::size_t k = i + 2;
                        std::int32_t disp = kNoDisp;

                        if (mod == 0)
                        {
                            if (rm == 4)
                            {
                                k += 1;
                            }
                            else if (rm == 5)
                            {
                                disp = *reinterpret_cast<const std::int32_t*>(code + k);
                                k += 4;
                            }
                        }
                        else if (mod == 1)
                        {
                            k += 1;
                        }
                        else if (mod == 2)
                        {
                            disp = *reinterpret_cast<const std::int32_t*>(code + k);
                            k += 4;
                        }
                        else
                        {
                            ++i;
                            continue;
                        }

                        if (k + 4 > limit)
                        {
                            break;
                        }
                        if (*reinterpret_cast<const std::uint32_t*>(code + k) == 0xFFFFFFFFu)
                        {
                            const std::int32_t offset = (disp != kNoDisp) ? disp : lastLea[rm];
                            if (offset != kNoDisp && !HasSlot(out, offset) && out.count < kMaxSlots)
                            {
                                out.slots[out.count++] = offset;
                            }
                            i = k + 4;
                            continue;
                        }
                    }

                    ++i;
                }
                return out.count > 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        int CollectClearCandidates(uintptr_t* out, int capacity)
        {
            uintptr_t base = 0;
            uintptr_t end = 0;
            HMODULE hMod = GetModuleHandle(nullptr);
            MODULEINFO mi = {};
            if (!hMod || !GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
            {
                return 0;
            }
            base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
            end = base + mi.SizeOfImage;

            int n = 0;
            uintptr_t cur = reinterpret_cast<uintptr_t>(Scanner::ScanMainMod(Patterns::UnderwaterMaskClear));
            while (cur && cur < end && n < capacity)
            {
                out[n++] = cur;
                cur = reinterpret_cast<uintptr_t>(Scanner::ScanRange(
                    reinterpret_cast<void*>(cur + 1),
                    static_cast<std::size_t>(end - (cur + 1)),
                    Patterns::UnderwaterMaskClear));
            }
            return n;
        }

        void* PickClearFunction(void* preMain)
        {
            uintptr_t candidates[8] = {};
            const int found = CollectClearCandidates(candidates, 8);
            if (found == 0)
            {
                return nullptr;
            }

            SlotList pre = {};
            const bool havePre = preMain &&
                CollectClearedSlots(reinterpret_cast<uintptr_t>(preMain), 0x2000, pre);

            uintptr_t best = 0;
            int bestSlots = kMaxSlots + 1;

            for (int i = 0; i < found; ++i)
            {
                SlotList cand = {};
                cand.fn = candidates[i];
                if (!CollectClearedSlots(candidates[i], 0x800, cand))
                {
                    continue;
                }

                if (havePre)
                {
                    bool subset = true;
                    for (int s = 0; s < cand.count && subset; ++s)
                    {
                        subset = HasSlot(pre, cand.slots[s]);
                    }
                    if (!subset)
                    {
                        continue;
                    }
                }

                if (cand.count < bestSlots)
                {
                    best = candidates[i];
                    bestSlots = cand.count;
                }
            }

            if (best)
            {
                const auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
                std::cout << "[SCAN] UnderwaterMask clear candidate selected"
                    << (havePre ? " by slot subset" : " by fewest slots")
                    << " (clears " << bestSlots << " slot(s)) at 0x"
                    << std::hex << (best - moduleBase) << std::dec << std::endl;
            }
            return reinterpret_cast<void*>(best);
        }

        void* FindClearFunction(void* anchorFunction)
        {
            if (!anchorFunction)
            {
                return Scanner::ScanMainMod(Patterns::UnderwaterMaskClear);
            }

            const uintptr_t anchor = reinterpret_cast<uintptr_t>(anchorFunction);
            if (void* local = Scanner::ScanRange(
                anchorFunction,
                ClearSearchWindow,
                Patterns::UnderwaterMaskClear))
            {
                return local;
            }

            const uintptr_t start = anchor > ClearSearchWindow
                                        ? anchor - ClearSearchWindow
                                        : 0;
            if (start != 0 && start < anchor)
            {
                if (void* local = Scanner::ScanRange(
                    reinterpret_cast<void*>(start),
                    static_cast<size_t>(anchor - start),
                    Patterns::UnderwaterMaskClear))
                {
                    return local;
                }
            }

            return Scanner::ScanMainMod(Patterns::UnderwaterMaskClear);
        }

        bool InstallOne(void* target, void* hook, MaskFunction* original, const char* name)
        {
            if (!target)
            {
                return true;
            }

            if (MH_CreateHook(target, hook, reinterpret_cast<void**>(original)) == MH_OK)
            {
                std::cout << "[SCAN] UnderwaterMask " << name << " Hook Ready." << std::endl;
                return true;
            }

            std::cout << "[ERR] UnderwaterMask " << name << " Hook Failed." << std::endl;
            return false;
        }
    }

    void Init()
    {
        if (!Config::Get().disable_underwater_mask)
        {
            return;
        }

        void* addrPre = ScanDirect("UnderwaterMaskPreMain", Patterns::UnderwaterMaskPreMain);
        void* addrMain = ScanDirect("UnderwaterMaskMain", Patterns::UnderwaterMaskMain);
        void* addrPost = ScanDirect("UnderwaterMaskPostMain", Patterns::UnderwaterMaskPostMain);

        if (!addrPre && !addrMain && !addrPost)
        {
            std::cout << "[ERR] UnderwaterMask patterns not found." << std::endl;
            return;
        }

        void* clearTarget = PickClearFunction(addrPre);
        if (!clearTarget)
        {
            std::cout << "[WARN] UnderwaterMask clear function not identified by slot layout,"
                " falling back to proximity search." << std::endl;
            void* clearAnchor = addrMain ? addrMain : (addrPre ? addrPre : addrPost);
            clearTarget = FindClearFunction(clearAnchor);
        }

        g_clearMask = reinterpret_cast<ClearFunction>(clearTarget);
        if (g_clearMask)
        {
            const auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
            std::cout << "[SCAN] UnderwaterMask clear function found at: 0x"
                << std::hex
                << (reinterpret_cast<uintptr_t>(g_clearMask) - moduleBase)
                << std::dec
                << std::endl;
        }
        else
        {
            std::cout << "[WARN] UnderwaterMask clear function not found; active cleanup unavailable." << std::endl;
        }

        bool ok = true;
        ok &= InstallOne(addrPre, reinterpret_cast<void*>(HookPre), &g_origPre, "Pre");
        ok &= InstallOne(addrMain, reinterpret_cast<void*>(HookMain), &g_origMain, "Main");
        ok &= InstallOne(addrPost, reinterpret_cast<void*>(HookPost), &g_origPost, "Post");

        if (!ok)
        {
            std::cout << "[ERR] UnderwaterMask hook initialization failed." << std::endl;
        }
    }
}

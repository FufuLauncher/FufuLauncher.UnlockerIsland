/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "Automation.h"
#include "../Scanner/Scanner.h"
#include "../Patterns/Patterns.h"
#include "../MinHook/MinHook.h"
#include <iostream>
#include <Psapi.h>

namespace HelperAddr
{
    uintptr_t InnerDispatcher = 0;
    uintptr_t CookHandler = 0;
    uintptr_t CookShowPage = 0;
    uintptr_t CookPatchEntity = 0;
    uintptr_t CookPatchPathB = 0;
    uintptr_t CookPatchBplSkip = 0;
    uintptr_t CookPatchNullChk1 = 0;
    uintptr_t CookPatchNullChk2 = 0;
    uintptr_t CookPatchNullTgt1 = 0;
    uintptr_t CookPatchNullTgt2 = 0;
    uintptr_t CookPatchFireWr = 0;
    uintptr_t ExpHandler = 0;
    uintptr_t ExpPatchAddr = 0;
}

Fn_CookShowPage g_oCookShowPage = nullptr;

static uint32_t g_CookFireState = 0;
static uint32_t g_CookFireParam = 0;
bool g_CookReady = false;
BYTE g_CookHandlerPrologue[8] = {0};
static BYTE g_CookSnapEntity[9] = {0};
static BYTE g_CookSnapBpl[1] = {0};
static BYTE g_CookSnapN1[6] = {0};
static BYTE g_CookSnapN2[6] = {0};
static volatile bool g_CookActive = false;
static volatile LONG g_CookLock = 0;
static volatile bool g_CookVehArmed = false;
static char g_CookEmptyStr[16] = {};

bool g_ExpReady = false;
BYTE g_ExpHandlerPrologue[8] = {0};
static BYTE g_ExpSnapPatch[8] = {};
static volatile LONG g_ExpLock = 0;
static volatile bool g_ExpVehArmed = false;
static uintptr_t g_ModBase = 0;
static uintptr_t g_ModEnd = 0;

volatile bool g_TrigCook = false;
volatile bool g_TrigExp = false;
DWORD g_LastCookTime = 0;
DWORD g_LastExpTime = 0;

static bool SEH_Memcmp(const void* p1, const void* p2, size_t sz)
{
    __try { return memcmp(p1, p2, sz) == 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static bool SEH_Memcpy(void* dest, const void* src, size_t sz)
{
    __try
    {
        memcpy(dest, src, sz);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static bool SEH_Read32(uintptr_t addr, int32_t& val)
{
    __try
    {
        val = *(int32_t*)addr;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static bool SEH_Read8(uintptr_t addr, uint8_t& val)
{
    __try
    {
        val = *(uint8_t*)addr;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

uintptr_t FindLocal(uintptr_t from, uintptr_t to, const char* pat)
{
    __try
    {
        int p[256], n = 0;
        for (const char* s = pat; *s;)
        {
            while (*s == ' ') s++;
            if (!*s) break;
            if (*s == '?')
            {
                p[n++] = -1;
                s++;
                if (*s == '?') s++;
            }
            else
            {
                char* e;
                p[n++] = (int)strtoul(s, &e, 16);
                s = e;
            }
        }
        for (uintptr_t i = from; i <= to - n; i++)
        {
            bool ok = true;
            for (int j = 0; j < n; j++) if (p[j] != -1 && *(BYTE*)(i + j) != p[j])
            {
                ok = false;
                break;
            }
            if (ok) return i;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return 0;
}

LONG CALLBACK CookVeh(EXCEPTION_POINTERS* ep)
{
    if (!ep || !ep->ExceptionRecord || !ep->ContextRecord) return EXCEPTION_CONTINUE_SEARCH;
    if (!g_CookVehArmed || ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    uintptr_t rip = (uintptr_t)ep->ContextRecord->Rip;

    if (rip >= g_ModBase && rip < g_ModEnd)
    {
        __try
        {
            BYTE* inst = (BYTE*)rip;
            if (inst[0] == 0x0F && inst[1] == 0xB6 && inst[2] == 0x17 &&
                ep->ContextRecord->Rdi > 0x7FFFFFFFFFFF0000ULL)
            {
                ep->ContextRecord->Rdi = (DWORD64)g_CookEmptyStr;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    if (rip == 0)
    {
        uintptr_t rsp = (uintptr_t)ep->ContextRecord->Rsp;
        __try
        {
            ep->ContextRecord->Rip = *(uintptr_t*)rsp;
            ep->ContextRecord->Rsp = rsp + 8;
            ep->ContextRecord->Rax = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    g_CookVehArmed = false;
    return EXCEPTION_CONTINUE_SEARCH;
}

LONG CALLBACK ExpVeh(EXCEPTION_POINTERS* ep)
{
    if (!ep || !ep->ExceptionRecord || !ep->ContextRecord) return EXCEPTION_CONTINUE_SEARCH;
    if (!g_ExpVehArmed || ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    uintptr_t rip = (uintptr_t)ep->ContextRecord->Rip;
    if (rip == 0)
    {
        uintptr_t rsp = (uintptr_t)ep->ContextRecord->Rsp;
        __try
        {
            ep->ContextRecord->Rip = *(uintptr_t*)rsp;
            ep->ContextRecord->Rsp = rsp + 8;
            ep->ContextRecord->Rax = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (rip >= g_ModBase && rip < g_ModEnd)
    {
        ep->ContextRecord->Rax = 0;
        ep->ContextRecord->Rip = rip + 4;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    g_ExpVehArmed = false;
    return EXCEPTION_CONTINUE_SEARCH;
}

void __fastcall hk_CookShowPage(__int64 page)
{
    if (g_CookActive && page)
    {
        g_CookActive = false;
        __try
        {
            uintptr_t v35 = *(uintptr_t*)(page + HelperField::CookCtxV35);
            if (v35)
            {
                uintptr_t v2 = *(uintptr_t*)(v35 + HelperField::CookCtxV2);
                if (v2)
                {
                    uint32_t oFS = g_CookFireState ? g_CookFireState : HelperField::CookFireStateDef;
                    uint32_t oFP = g_CookFireParam ? g_CookFireParam : HelperField::CookFireParamDef;
                    *(uint32_t*)(v2 + oFS) = HelperField::CookHookMagic1;
                    *(uint32_t*)(v2 + oFP) = HelperField::CookHookMagic1;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    __try
    {
        if (g_oCookShowPage)
        {
            g_oCookShowPage(page);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

static bool SEH_ResolveCookPatchesLogic()
{
    __try
    {
        uintptr_t h = HelperAddr::CookHandler;
        uintptr_t hEnd = h + 0x800;

        memcpy(g_CookHandlerPrologue, (void*)h, 8);

        HelperAddr::CookPatchPathB = FindLocal(h + 0x200, hEnd, Patterns::CookPathB);
        if (!HelperAddr::CookPatchPathB) return false;

        for (uintptr_t s = HelperAddr::CookPatchPathB - 5; s >= h + 0x100; s--)
        {
            uintptr_t m = FindLocal(s, s + 5, Patterns::CookEntityVal);
            if (m == s)
            {
                HelperAddr::CookPatchEntity = s;
                break;
            }
        }
        if (!HelperAddr::CookPatchEntity) return false;

        HelperAddr::CookPatchFireWr = FindLocal(HelperAddr::CookPatchPathB, hEnd, Patterns::CookFireWrite);
        if (!HelperAddr::CookPatchFireWr) return false;

        g_CookFireState = *(uint16_t*)(HelperAddr::CookPatchFireWr + 2);
        g_CookFireParam = *(uint16_t*)(HelperAddr::CookPatchFireWr + 8);

        for (uintptr_t s = HelperAddr::CookPatchFireWr - 1; s > HelperAddr::CookPatchFireWr - 0x30; s--)
        {
            uintptr_t m = FindLocal(s, s + 4, Patterns::CookBplSkip);
            if (m == s)
            {
                HelperAddr::CookPatchBplSkip = s + 3;
                break;
            }
        }
        if (!HelperAddr::CookPatchBplSkip) return false;

        uintptr_t nc = FindLocal(HelperAddr::CookPatchPathB, HelperAddr::CookPatchFireWr, Patterns::CookNullChk);
        if (nc)
        {
            HelperAddr::CookPatchNullChk1 = nc + 3;
            HelperAddr::CookPatchNullTgt1 = FindLocal(nc + 9, HelperAddr::CookPatchFireWr, Patterns::CookNullTgt1);
            uintptr_t nc2 = FindLocal(nc + 9, HelperAddr::CookPatchFireWr, Patterns::CookNullChk);
            if (nc2)
            {
                HelperAddr::CookPatchNullChk2 = nc2 + 3;
                HelperAddr::CookPatchNullTgt2 = FindLocal(nc2 + 9, HelperAddr::CookPatchFireWr, Patterns::CookNullTgt2);
            }
        }

        for (uintptr_t s = h + 0x300; s < h + 0x800; s++)
        {
            if (*(BYTE*)s == 0xE8 && *(BYTE*)(s + 5) == 0x40 && *(BYTE*)(s + 6) == 0xB6 && *(BYTE*)(s + 7) == 0x01)
            {
                int32_t rel = *(int32_t*)(s + 1);
                HelperAddr::CookShowPage = s + 5 + rel;
                break;
            }
        }

        memcpy(g_CookSnapEntity, (void*)HelperAddr::CookPatchEntity, 9);
        memcpy(g_CookSnapBpl, (void*)HelperAddr::CookPatchBplSkip, 1);
        if (HelperAddr::CookPatchNullChk1 && HelperAddr::CookPatchNullTgt1)
            memcpy(g_CookSnapN1, (void*)HelperAddr::CookPatchNullChk1, 6);
        if (HelperAddr::CookPatchNullChk2 && HelperAddr::CookPatchNullTgt2)
            memcpy(g_CookSnapN2, (void*)HelperAddr::CookPatchNullChk2, 6);

        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ResolveCookingPatches()
{
    if (!HelperAddr::CookHandler) return false;
    if (!SEH_ResolveCookPatchesLogic())
    {
        std::cout << "[COOK] Resolve failed or memory exception occurred" << std::endl;
        return false;
    }
    return true;
}

static bool SEH_ApplyCookPatches()
{
    __try
    {
        int32_t d = (int32_t)(HelperAddr::CookPatchPathB - (HelperAddr::CookPatchEntity + 5));
        BYTE jmp[9] = {0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90};
        memcpy(jmp + 1, &d, 4);
        memcpy((void*)HelperAddr::CookPatchEntity, jmp, 9);

        *(BYTE*)HelperAddr::CookPatchBplSkip = 0xEB;

        if (HelperAddr::CookPatchNullChk1 && HelperAddr::CookPatchNullTgt1)
        {
            int32_t d1 = (int32_t)(HelperAddr::CookPatchNullTgt1 - (HelperAddr::CookPatchNullChk1 + 6));
            BYTE p1[6] = {0x0F, 0x84, 0, 0, 0, 0};
            memcpy(p1 + 2, &d1, 4);
            memcpy((void*)HelperAddr::CookPatchNullChk1, p1, 6);
        }
        if (HelperAddr::CookPatchNullChk2 && HelperAddr::CookPatchNullTgt2)
        {
            int32_t d2 = (int32_t)(HelperAddr::CookPatchNullTgt2 - (HelperAddr::CookPatchNullChk2 + 6));
            BYTE p2[6] = {0x0F, 0x84, 0, 0, 0, 0};
            memcpy(p2 + 2, &d2, 4);
            memcpy((void*)HelperAddr::CookPatchNullChk2, p2, 6);
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool SEH_RestoreCookPatches()
{
    __try
    {
        memcpy((void*)HelperAddr::CookPatchEntity, g_CookSnapEntity, 9);
        memcpy((void*)HelperAddr::CookPatchBplSkip, g_CookSnapBpl, 1);
        if (HelperAddr::CookPatchNullChk1 && HelperAddr::CookPatchNullTgt1)
            memcpy((void*)HelperAddr::CookPatchNullChk1, g_CookSnapN1, 6);
        if (HelperAddr::CookPatchNullChk2 && HelperAddr::CookPatchNullTgt2)
            memcpy((void*)HelperAddr::CookPatchNullChk2, g_CookSnapN2, 6);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool SEH_CallCookHandler(Fn_CookHandler handler, __int64 dummy)
{
    __try
    {
        handler(dummy, dummy);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void DoCookingLogic()
{
    std::cout << "[Cook] Attempting to execute auto cook." << std::endl;

    if (!HelperAddr::CookHandler)
    {
        std::cout << "[Cook] Failed: CookHandler is null." << std::endl;
        return;
    }
    if (!g_CookReady)
    {
        std::cout << "[Cook] Failed: g_CookReady is false." << std::endl;
        return;
    }
    if (InterlockedCompareExchange(&g_CookLock, 1, 0) != 0)
    {
        std::cout << "[Cook] Failed: Currently executing (lock conflict)." << std::endl;
        return;
    }

    if (!SEH_Memcmp((void*)HelperAddr::CookHandler, g_CookHandlerPrologue, 8))
    {
        InterlockedExchange(&g_CookLock, 0);
        std::cout << "[Cook] Failed: Prologue changed or memory inaccessible." << std::endl;
        return;
    }

    std::cout << "[Cook] Memory check passed. Applying patches." << std::endl;

    uintptr_t lo = HelperAddr::CookPatchEntity;
    uintptr_t hi = HelperAddr::CookPatchFireWr + 19;
    DWORD prot = 0;
    bool patchSuccess = false;

    if (VirtualProtect((void*)lo, hi - lo, PAGE_EXECUTE_READWRITE, &prot))
    {
        if (SEH_ApplyCookPatches())
        {
            VirtualProtect((void*)lo, hi - lo, prot, &prot);
            FlushInstructionCache(GetCurrentProcess(), (void*)lo, hi - lo);
            patchSuccess = true;
        }
        else
        {
            VirtualProtect((void*)lo, hi - lo, prot, &prot);
            std::cout << "[Cook] Exception applying patches. Triggering rollback." << std::endl;
        }
    }

    if (patchSuccess)
    {
        std::cout << "[Cook] Patches applied. Calling handler." << std::endl;
        static BYTE dummy[4096] = {};
        g_CookActive = true;

        if (SEH_CallCookHandler((Fn_CookHandler)HelperAddr::CookHandler, (__int64)dummy))
        {
            std::cout << "[Cook] Handler executed successfully." << std::endl;
        }
        else
        {
            std::cout << "[Cook] Exception inside handler. Triggering rollback." << std::endl;
        }

        g_CookActive = false;
    }

    std::cout << "[Cook] Restoring memory." << std::endl;

    if (VirtualProtect((void*)lo, hi - lo, PAGE_EXECUTE_READWRITE, &prot))
    {
        if (!SEH_RestoreCookPatches())
        {
            std::cout << "[Cook] Exception during memory restoration!" << std::endl;
        }
        VirtualProtect((void*)lo, hi - lo, prot, &prot);
        FlushInstructionCache(GetCurrentProcess(), (void*)lo, hi - lo);
    }

    g_CookVehArmed = true;
    InterlockedExchange(&g_CookLock, 0);
    std::cout << "[Cook] Auto cook sequence completed/rolled back." << std::endl;
}

void InitCooking()
{
    if (!g_ModBase)
    {
        HMODULE hMod = GetModuleHandle(nullptr);
        MODULEINFO mi = {};
        if (GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
        {
            g_ModBase = (uintptr_t)mi.lpBaseOfDll;
            g_ModEnd = g_ModBase + mi.SizeOfImage;
        }
    }

    AddVectoredExceptionHandler(1, CookVeh);

    void* addr = Scanner::ScanMainMod(Patterns::CookHandler);
    if (!addr)
    {
        std::cout << "[COOK] CookHandler pattern not found" << std::endl;
        return;
    }
    HelperAddr::CookHandler = (uintptr_t)addr;
    std::cout << "[COOK] CookHandler found at 0x" << std::hex << HelperAddr::CookHandler << std::dec << std::endl;

    if (!ResolveCookingPatches()) return;
    g_CookReady = true;
    std::cout << "[COOK] Ready: fs=0x" << std::hex << g_CookFireState
        << " fp=0x" << g_CookFireParam << std::dec << std::endl;

    if (HelperAddr::CookShowPage)
    {
        if (MH_CreateHook((void*)HelperAddr::CookShowPage, (void*)hk_CookShowPage, (void**)&g_oCookShowPage) == MH_OK)
        {
            std::cout << "[COOK] ShowPage hook at 0x" << std::hex << HelperAddr::CookShowPage << std::dec << std::endl;
        }
    }
}

static bool SEH_ExpSitesLogic()
{
    __try
    {
        uintptr_t label3 = HelperAddr::ExpHandler + 0x27;
        if (*(BYTE*)label3 != 0xC7 || *(BYTE*)(label3 + 1) != 0x44)
        {
            return false;
        }

        uintptr_t testAddr = FindLocal(HelperAddr::ExpHandler + 0x40, HelperAddr::ExpHandler + 0x80,
                                       Patterns::ExpTestJz);
        if (!testAddr) return false;

        HelperAddr::ExpPatchAddr = label3;
        memcpy(g_ExpHandlerPrologue, (void*)HelperAddr::ExpHandler, 8);
        memcpy(g_ExpSnapPatch, (void*)HelperAddr::ExpPatchAddr, 8);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ResolveExpSites()
{
    if (!HelperAddr::ExpHandler) return false;
    if (!SEH_ExpSitesLogic())
    {
        std::cout << "[EXP] Validation or pattern failed" << std::endl;
        return false;
    }
    return true;
}

static bool SEH_ApplyExpPatch(int32_t jmpDisp)
{
    __try
    {
        BYTE patch[8] = {0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90};
        memcpy(patch + 1, &jmpDisp, 4);
        memcpy((void*)HelperAddr::ExpPatchAddr, patch, 8);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool SEH_CallExpHandler(Fn_ExpHandler handler, void* dummy)
{
    __try
    {
        handler(dummy, dummy);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void DoExpeditionLogic()
{
    std::cout << "[Expedition] Attempting to execute auto expedition." << std::endl;

    if (!HelperAddr::ExpHandler)
    {
        std::cout << "[Expedition] Failed: ExpHandler is null." << std::endl;
        return;
    }
    if (!g_ExpReady)
    {
        std::cout << "[Expedition] Failed: g_ExpReady is false." << std::endl;
        return;
    }
    if (InterlockedCompareExchange(&g_ExpLock, 1, 0) != 0)
    {
        std::cout << "[Expedition] Failed: Currently executing (lock conflict)." << std::endl;
        return;
    }

    if (!SEH_Memcmp((void*)HelperAddr::ExpHandler, g_ExpHandlerPrologue, 8))
    {
        InterlockedExchange(&g_ExpLock, 0);
        std::cout << "[Expedition] Failed: Prologue changed or memory inaccessible." << std::endl;
        return;
    }

    uintptr_t testAddr = FindLocal(HelperAddr::ExpHandler + 0x40, HelperAddr::ExpHandler + 0x80, Patterns::ExpTestJz);
    if (!testAddr)
    {
        InterlockedExchange(&g_ExpLock, 0);
        std::cout << "[Expedition] Failed: TestJz not found." << std::endl;
        return;
    }

    bool patchSuccess = false;
    DWORD prot = 0;
    int32_t jzDisp = 0;

    if (!SEH_Read32(testAddr + 4, jzDisp))
    {
        InterlockedExchange(&g_ExpLock, 0);
        std::cout << "[Expedition] Failed to read jzDisp." << std::endl;
        return;
    }

    uintptr_t elseTarget = (testAddr + 2) + 6 + jzDisp;
    int32_t jmpDisp = (int32_t)(elseTarget - (HelperAddr::ExpPatchAddr + 5));

    std::cout << "[Expedition] Applying patch to jump to success path." << std::endl;

    if (VirtualProtect((void*)HelperAddr::ExpPatchAddr, 8, PAGE_EXECUTE_READWRITE, &prot))
    {
        if (SEH_ApplyExpPatch(jmpDisp))
        {
            VirtualProtect((void*)HelperAddr::ExpPatchAddr, 8, prot, &prot);
            FlushInstructionCache(GetCurrentProcess(), (void*)HelperAddr::ExpPatchAddr, 8);
            patchSuccess = true;
        }
        else
        {
            VirtualProtect((void*)HelperAddr::ExpPatchAddr, 8, prot, &prot);
            std::cout << "[Expedition] Exception applying patches. Triggering rollback." << std::endl;
        }
    }

    if (patchSuccess)
    {
        static BYTE dummy[4096] = {};
        g_ExpVehArmed = true;

        if (SEH_CallExpHandler((Fn_ExpHandler)HelperAddr::ExpHandler, (void*)dummy))
        {
            std::cout << "[Expedition] Handler executed successfully." << std::endl;
        }
        else
        {
            std::cout << "[Expedition] Exception inside handler. Triggering rollback." << std::endl;
        }
    }

    if (VirtualProtect((void*)HelperAddr::ExpPatchAddr, 8, PAGE_EXECUTE_READWRITE, &prot))
    {
        if (!SEH_Memcpy((void*)HelperAddr::ExpPatchAddr, g_ExpSnapPatch, 8))
        {
            std::cout << "[Expedition] Exception during memory restoration!" << std::endl;
        }
        VirtualProtect((void*)HelperAddr::ExpPatchAddr, 8, prot, &prot);
        FlushInstructionCache(GetCurrentProcess(), (void*)HelperAddr::ExpPatchAddr, 8);
    }

    InterlockedExchange(&g_ExpLock, 0);
    std::cout << "[Expedition] Auto expedition sequence completed/rolled back." << std::endl;
}

// The hash constant appears twice inside the dispatcher: in its binary-search
// tree (followed by ja/jbe) and in the equality test that selects the case body
// (followed by jz/jnz). Only the latter identifies the expedition entry point.
static uintptr_t FindExpHashSite()
{
    uintptr_t cur = (uintptr_t)Scanner::ScanMainMod(Patterns::ExpHashCmp);
    while (cur && cur < g_ModEnd)
    {
        uint8_t op = 0, jcc = 0;
        if (SEH_Read8(cur + 6, op) && SEH_Read8(cur + 7, jcc) &&
            op == 0x0F && (jcc == 0x84 || jcc == 0x85))
        {
            return cur;
        }
        cur = (uintptr_t)Scanner::ScanRange((void*)(cur + 1), (size_t)(g_ModEnd - (cur + 1)), Patterns::ExpHashCmp);
    }
    return 0;
}

void InitExpedition()
{
    if (!g_ModBase)
    {
        HMODULE hMod = GetModuleHandle(nullptr);
        MODULEINFO mi = {};
        if (GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
        {
            g_ModBase = (uintptr_t)mi.lpBaseOfDll;
            g_ModEnd = g_ModBase + mi.SizeOfImage;
        }
    }

    AddVectoredExceptionHandler(1, ExpVeh);

    uintptr_t hashCmp = FindExpHashSite();
    if (!hashCmp)
    {
        std::cout << "[EXP] ExpHashCmp pattern not found" << std::endl;
        return;
    }
    std::cout << "[EXP] ExpHashCmp found at 0x" << std::hex << hashCmp << std::dec << std::endl;

    uintptr_t tailJmp = 0;
    uint8_t jccOp = 0;
    int32_t jzRel = 0;
    if (SEH_Read8(hashCmp + 7, jccOp) && SEH_Read32(hashCmp + 8, jzRel))
    {
        // jz (0F 84): the case body is the branch target;
        // jnz (0F 85): it falls through, since the mismatch is what branches away.
        uintptr_t caseBody = (jccOp == 0x84)
                                 ? hashCmp + 12 + (uintptr_t)jzRel
                                 : hashCmp + 12;
        tailJmp = FindLocal(caseBody, caseBody + 0x200, Patterns::ExpTailJmp);
    }
    if (!tailJmp)
    {
        std::cout << "[EXP] Tail jmp not found" << std::endl;
        return;
    }

    uintptr_t jmpInst = tailJmp + 2;
    int32_t rel = 0;

    if (!SEH_Read32(jmpInst + 1, rel))
    {
        std::cout << "[EXP] Exception reading handler rel" << std::endl;
        return;
    }

    HelperAddr::ExpHandler = jmpInst + 5 + rel;

    if (!HelperAddr::ExpHandler)
    {
        std::cout << "[EXP] Handler resolve failed" << std::endl;
        return;
    }
    std::cout << "[EXP] ExpHandler found at 0x" << std::hex << HelperAddr::ExpHandler << std::dec << std::endl;

    g_ExpReady = ResolveExpSites();
    if (g_ExpReady)
        std::cout << "[EXP] Ready: handler=0x" << std::hex << HelperAddr::ExpHandler
            << " patch=0x" << HelperAddr::ExpPatchAddr << std::dec << std::endl;
    else
        std::cout << "[EXP] ResolveExpSites failed" << std::endl;
}

void InitExpHandlerPrologueSafe()
{
    if (HelperAddr::ExpHandler && g_ExpReady)
    {
        if (!SEH_Memcpy(g_ExpHandlerPrologue, (void*)HelperAddr::ExpHandler, 8))
        {
            g_ExpReady = false;
            HelperAddr::ExpHandler = 0;
        }
    }
}

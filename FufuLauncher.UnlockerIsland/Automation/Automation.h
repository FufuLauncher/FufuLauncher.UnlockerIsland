/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include "../Core/SharedState.h"
#include <cstdint>

namespace HelperField
{
    constexpr uint32_t CookCtxV35 = 0x20;
    constexpr uint32_t CookCtxV2 = 0x10;
    constexpr uint32_t CookFireStateDef = 0x248;
    constexpr uint32_t CookFireParamDef = 0x250;
    constexpr uint32_t CookEntityRefDef = 0xA0;
    constexpr uint32_t CookHookMagic1 = 0x3F800000;
}

namespace HelperAddr
{
    extern uintptr_t InnerDispatcher;
    extern uintptr_t CookHandler;
    extern uintptr_t CookShowPage;
    extern uintptr_t CookPatchEntity;
    extern uintptr_t CookPatchPathB;
    extern uintptr_t CookPatchBplSkip;
    extern uintptr_t CookPatchNullChk1;
    extern uintptr_t CookPatchNullChk2;
    extern uintptr_t CookPatchNullTgt1;
    extern uintptr_t CookPatchNullTgt2;
    extern uintptr_t CookPatchFireWr;
    extern uintptr_t ExpHandler;
    extern uintptr_t ExpPatchAddr;
}

typedef void (__fastcall *Fn_CookShowPage)(__int64);
typedef __int64 (__fastcall *Fn_CookHandler)(__int64, __int64);
typedef bool (__fastcall *Fn_ExpHandler)(void*, void*);

extern Fn_CookShowPage g_oCookShowPage;
extern BYTE g_CookHandlerPrologue[8];
extern BYTE g_ExpHandlerPrologue[8];
extern bool g_CookReady;
extern bool g_ExpReady;
extern volatile bool g_TrigCook;
extern volatile bool g_TrigExp;
extern DWORD g_LastCookTime;
extern DWORD g_LastExpTime;

uintptr_t FindLocal(uintptr_t from, uintptr_t to, const char* pat);

LONG CALLBACK CookVeh(EXCEPTION_POINTERS* ep);
LONG CALLBACK ExpVeh(EXCEPTION_POINTERS* ep);

bool ResolveCookingPatches();
bool ResolveExpSites();

void __fastcall hk_CookShowPage(__int64 page);

void DoCookingLogic();
void DoExpeditionLogic();

void InitCooking();
void InitExpedition();
void InitExpHandlerPrologueSafe();

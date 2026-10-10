/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <list>

struct Il2CppString;

struct Vector3
{
    float x, y, z;
};

typedef int32_t (WINAPI *tGetFrameCount)();
typedef int32_t (WINAPI *tSetFrameCount)(int32_t);
typedef void (WINAPI *tSwitchInput)(void*);
typedef int32_t (WINAPI *tChangeFov)(void*, float);
typedef void (WINAPI *tSetupQuestBanner)(void*);
typedef void (WINAPI *tShowDamage)(void*, int, int, int, float, Il2CppString*, void*, void*, int, char, float);
typedef void (WINAPI *tCraftEntry)(void*);
typedef bool (WINAPI *tCraftPartner)(Il2CppString*, void*, void*, void*, void*);
typedef Il2CppString* (WINAPI *tFindString)(const char*);
typedef void* (WINAPI *tFindGameObject)(Il2CppString*);
typedef void (WINAPI *tSetActive)(void*, bool);
typedef void (WINAPI *tSetupPlayerProfilePage)(void*);
typedef void (WINAPI *tSetupResinList)(void*);
typedef bool (WINAPI *tEventCamera)(void*, void*);
typedef bool (WINAPI *tCheckCanEnter)();
typedef void (WINAPI *tOpenTeamPage)(bool);
typedef void (WINAPI *tOpenTeam)();
typedef __int64 (*tDisplayFog)(__int64, __int64);
typedef void* (WINAPI *tPlayerPerspective)(void*, float, void*);
typedef int32_t (WINAPI *tSetSyncCount)(bool);
typedef __int64 (WINAPI *tGameUpdate)(__int64, const char*);
typedef BOOL (WINAPI*tQueryPerformanceCounter)(LARGE_INTEGER*);
typedef ULONGLONG (WINAPI*tGetTickCount64)();
typedef bool (WINAPI *tGetActive)(void*);
typedef void (WINAPI *tAvatarPaimonAppear)(void*, void*, bool);
typedef void* (*tGetComponent)(void*, Il2CppString*);
typedef Il2CppString* (*tGetText)(void*);
typedef void (WINAPI *tVoidFunc)(void*);
typedef Il2CppString* (*tGetName)(void*);
typedef __int64 (*FnStringNew)(const char*);
typedef void (*FnShowDialog)(__int64, __int64, __int64, __int64, int);
typedef void (__fastcall *tButtonClicked)(void*);
typedef void (__fastcall *tClockPageBack)(void*, void*);

typedef __int64 (__fastcall *tUpdateInnerTarget)(void*, void*, double);

extern std::atomic<void*> o_GetFrameCount;
extern std::atomic<void*> o_SetFrameCount;
extern std::atomic<void*> o_ChangeFov;
extern std::atomic<void*> o_SetupQuestBanner;
extern std::atomic<void*> o_SetupPlayerProfilePage;
extern std::atomic<void*> o_SetupResinList;
extern std::atomic<void*> o_ShowDamage;
extern std::atomic<void*> o_CraftEntry;
extern std::atomic<void*> o_EventCamera;
extern std::atomic<void*> o_OpenTeam;
extern std::atomic<void*> o_DisplayFog;
extern std::atomic<void*> p_SwitchInput;
extern std::atomic<void*> p_FindString;
extern std::atomic<void*> p_CraftPartner;
extern std::atomic<void*> p_FindGameObject;
extern std::atomic<void*> o_SetActive;
extern std::atomic<void*> p_CheckCanEnter;
extern std::atomic<void*> p_OpenTeamPage;
extern std::atomic<void*> o_PlayerPerspective;
extern std::atomic<void*> o_SetSyncCount;
extern std::atomic<void*> o_GameUpdate;
extern std::atomic<void*> o_ClockPageOk;
extern std::atomic<void*> p_ClockPageClose;
extern std::atomic<void*> p_ClockPageFinish;
extern std::atomic<void*> p_ClockPageBack;
extern std::atomic<void*> p_CheckCanOpenMap;
extern std::atomic<void*> p_GetName;
extern std::atomic<void*> p_GetActive;
extern std::atomic<void*> p_AvatarPaimonAppear;
extern std::atomic<void*> p_StringNew;
extern std::atomic<void*> p_ShowDialog;
extern std::atomic<void*> o_UpdateInnerTarget;

extern std::atomic<bool> g_RequestReloadPopup;
extern std::atomic<bool> g_GameUpdateInit;
extern std::atomic<bool> g_RequestCraft;
extern std::atomic<bool> g_TouchScreenInit;

extern unsigned char originalCheckCanOpenMapBytes[5];

extern std::list<std::wstring> GrassPrefix;

struct SafeFogBuffer
{
    __declspec(align(16)) uint8_t data[404];
    uint8_t padding[16];
};

extern std::atomic<bool> g_ShouldShowDialog;
extern std::string g_DialogText;
extern std::mutex g_DialogMutex;
extern std::atomic<bool> g_StopDialogPolling;

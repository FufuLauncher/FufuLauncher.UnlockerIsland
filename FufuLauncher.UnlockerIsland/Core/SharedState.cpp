/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "SharedState.h"

std::atomic<void*> o_GetFrameCount{ nullptr };
std::atomic<void*> o_SetFrameCount{ nullptr };
std::atomic<void*> o_ChangeFov{ nullptr };
std::atomic<void*> o_SetupQuestBanner{ nullptr };
std::atomic<void*> o_ShowDamage{ nullptr };
std::atomic<void*> o_CraftEntry{ nullptr };
std::atomic<void*> o_EventCamera{ nullptr };
std::atomic<void*> o_OpenTeam{ nullptr };
std::atomic<void*> o_DisplayFog{ nullptr };
std::atomic<void*> p_SwitchInput{ nullptr };
std::atomic<void*> p_FindString{ nullptr };
std::atomic<void*> p_CraftPartner{ nullptr };
std::atomic<void*> p_FindGameObject{ nullptr };
std::atomic<void*> o_SetActive{ nullptr };
std::atomic<void*> p_CheckCanEnter{ nullptr };
std::atomic<void*> p_OpenTeamPage{ nullptr };
std::atomic<void*> o_PlayerPerspective{ nullptr };
std::atomic<void*> o_SetSyncCount{ nullptr };
std::atomic<void*> o_GameUpdate{ nullptr };
std::atomic<void*> o_ClockPageOk{ nullptr };
std::atomic<void*> p_ClockPageClose{ nullptr };
std::atomic<void*> p_ClockPageFinish{ nullptr };
std::atomic<void*> p_ClockPageBack{ nullptr };
std::atomic<void*> p_CheckCanOpenMap{ nullptr };
std::atomic<void*> p_GetName{ nullptr };
std::atomic<void*> o_send{ nullptr };
std::atomic<void*> o_sendto{ nullptr };
std::atomic<void*> p_GetActive{ nullptr };
std::atomic<void*> p_AvatarPaimonAppear{ nullptr };
std::atomic<void*> p_StringNew{ nullptr };
std::atomic<void*> p_ShowDialog{ nullptr };
std::atomic<void*> o_UpdateInnerTarget{ nullptr };
std::atomic<void*> o_ProfilePageRefresh{ nullptr };

std::atomic<bool> g_RequestReloadPopup{ false };
std::atomic<bool> g_GameUpdateInit{ false };
std::atomic<bool> g_ProfilePrivacyRuntimeReady{ false };
std::atomic<bool> g_RequestCraft{ false };
std::atomic<bool> g_TouchScreenInit{ false };

unsigned char originalCheckCanOpenMapBytes[5] = {0};
ID3D11DeviceContext* g_pd3dContext = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hGameWindow_ImGui = nullptr;
tResizeBuffers o_ResizeBuffers = nullptr;
tPresent1 o_Present1 = nullptr;

bool g_ShowCoordWindow = false;

std::list<std::wstring> GrassPrefix {
    L"Area_Ndkl_",
    L"Area_Nt_",
    L"Area_Fd_",
    L"Area_Xm_",
    L"Area_Ly_",
    L"Stages_M",
    L"BigWorld_",
};

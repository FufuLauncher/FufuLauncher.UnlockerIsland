/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include "../Core/SharedState.h"
#include <d3d11.h>

void UpdateOpenMap();
bool LoadTextureFromFile(const char* filename, ID3D11Device* device, ID3D11ShaderResourceView** out_srv, int* out_width,
                         int* out_height);
float GetProcessCpuUsage();
void* WINAPI hk_PlayerPerspective(void* a1, float a2, void* a3);
void LogOffset(const std::string& name, void* resultAddress, void* instructionAddress = nullptr);
bool WINAPI hk_EventCamera(void* a, void* b);
void WINAPI hk_CraftEntry(void* _this);
void WINAPI hk_OpenTeam();
void WINAPI hk_SetActive(void* pThis, bool active);
bool IsCameraPageActiveFromEvents();
auto hk_DisplayFog(__int64 a1, __int64 a2) -> __int64;
void InitExpHandlerPrologueSafe();

bool CheckWindowFocused(HWND window);

std::string GetOwnDllDir();
std::string GetInstructionInfo(uint8_t* addr);

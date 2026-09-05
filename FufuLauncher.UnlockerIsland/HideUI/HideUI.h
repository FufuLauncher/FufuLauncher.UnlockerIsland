/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include "../Core/SharedState.h"

void UpdateHideUID();
void UpdateHideMainUI();
bool UpdateHideProfileUID();
void UpdateHideProfileBirthday();
void UpdateProfilePrivacyUI();
bool IsProfilePrivacyUIActive();
void BeginProfilePrivacyUI();
void EndProfilePrivacyUI();
void NotifyProfileUIDBlocked();
void NotifyProfilePrivacyConfigReload();
void UpdatePendingProfilePrivacyUI();
void UpdateTitleWatermark();
void WINAPI hk_SetupQuestBanner(void* __this);
void WINAPI hk_ShowDamage(void* a, int b, int c, int d, float e, Il2CppString* f, void* g, void* h, int i, char j, float k);
__int64 __fastcall hk_ProfilePageRefresh(void* pThis, __int64 a2);

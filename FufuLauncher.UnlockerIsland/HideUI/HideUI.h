/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include "../Core/SharedState.h"

void UpdateHideUID();
// Event-driven profile privacy: invoked right after the game has set up /
// refreshed the player profile page, and after a config hot-reload. The
// page opens normally (origin runs first), then the UID/birthday objects
// are hidden when their config flags are on; UID is restored when off.
void ApplyProfilePrivacyState();
void UpdateTitleWatermark();
void WINAPI hk_SetupQuestBanner(void* __this);
void WINAPI hk_ShowDamage(void* a, int b, int c, int d, float e, Il2CppString* f, void* g, void* h, int i, char j, float k);
__int64 __fastcall hk_ProfilePageRefresh(void* pThis, __int64 a2);

/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include "../Core/SharedState.h"

void UpdateHideUID();
void UpdateHideMainUI();
// Event-driven profile privacy: invoked right after the game has set up the
// player profile page. Calls origin first so the page opens normally, then
// force-hides the UID/birthday objects when their config flags are on.
void ApplyProfilePrivacyState();
void UpdateTitleWatermark();
void WINAPI hk_SetupQuestBanner(void* __this);
void WINAPI hk_ShowDamage(void* a, int b, int c, int d, float e, Il2CppString* f, void* g, void* h, int i, char j, float k);

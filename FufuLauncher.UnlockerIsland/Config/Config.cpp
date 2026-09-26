/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "Config.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <Windows.h>
#include <iostream>

ModConfig g_Config;

std::string AnsiToUtf8(const std::string& str) {
    if (str.empty()) return "";
    
    int wLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wLen <= 0) return str;
    std::vector<wchar_t> wBuf(wLen);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wBuf.data(), wLen);
    
    int uLen = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, nullptr, 0, nullptr, nullptr);
    if (uLen <= 0) return str;
    std::vector<char> uBuf(uLen);
    WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, uBuf.data(), uLen, nullptr, nullptr);

    return std::string(uBuf.data());
}

namespace Config {
    ModConfig& Get() { return g_Config; }
    
    int ReadInt(LPCSTR section, int defaultVal, LPCSTR file) {
        return GetPrivateProfileIntA(section, "Value", defaultVal, file);
    }

    float ReadFloat(LPCSTR section, float defaultVal, LPCSTR file) {
        char buf[32];
        char defStr[32];
        snprintf(defStr, sizeof(defStr), "%f", defaultVal);
        GetPrivateProfileStringA(section, "Value", defStr, buf, 32, file);
        return (float)atof(buf);
    }

    void ReadString(LPCSTR section, LPCSTR defaultVal, char* outBuf, int size, LPCSTR file) {
        GetPrivateProfileStringA(section, "Value", defaultVal, outBuf, size, file);
    }
    
    void WriteFloat(LPCSTR section, float value, LPCSTR file) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", value);
        WritePrivateProfileStringA(section, "Value", buf, file);
    }
    
    void WriteInt(LPCSTR section, int value, LPCSTR file) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        WritePrivateProfileStringA(section, "Value", buf, file);
    }
    
    std::string GetConfigPath() {
        char path[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&g_Config, &hm);
        GetModuleFileNameA(hm, path, sizeof(path));
        
        std::string fullPath = path;
        std::string dirPath;
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dirPath = fullPath.substr(0, lastSlash);
        } else {
            dirPath = ".";
        }
        return dirPath + "\\config.ini";
    }
    
    void Load() {
        char path[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&g_Config, &hm);
        GetModuleFileNameA(hm, path, sizeof(path));
        
        std::string fullPath = path;
        std::string dirPath;
        
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dirPath = fullPath.substr(0, lastSlash);
        } else {
            dirPath = ".";
        }
        
        std::string cfgPath = dirPath + "\\config.ini"; 
        LPCSTR file = cfgPath.c_str();
        
        g_Config.debug_console = ReadInt("DebugConsole", 0, file);
        
        g_Config.enable_fps_override = ReadInt("FpsUnlock", 0, file);

        g_Config.enable_fps_clamp = ReadInt("PreventDetectionPopup", 1, file);
        
        g_Config.selected_fps = ReadInt("TargetFps", 60, file);
        
        if (g_Config.selected_fps > 100000) {
            g_Config.selected_fps = 100000;
        } else if (g_Config.selected_fps < 1) {
            g_Config.selected_fps = 60;
        }
        
        g_Config.enable_vsync_override = ReadInt("VSync", 1, file);
        
        g_Config.use_touch_screen = ReadInt("TouchScreen", 0, file);
        
        g_Config.enable_fov_override = ReadInt("FovUnlock", 0, file);
        
        g_Config.fov_value = ReadFloat("FovValue", 45.0f, file);

        g_Config.enable_camera_offset = ReadInt("EnableCameraOffset", 0, file);
        g_Config.camera_offset_key = ReadInt("CameraOffsetKey", VK_F6, file);
        g_Config.camera_offset_x = ReadFloat("CameraOffsetX", 0.0f, file);
        g_Config.camera_offset_z = ReadFloat("CameraOffsetZ", 0.0f, file);
        g_Config.camera_offset_y = ReadFloat("CameraOffsetY",
            ReadFloat("CameraHeightOffset", 0.0f, file), file);

        auto clampCameraOffset = [](float value) {
            if (value < -5.0f) return -5.0f;
            if (value > 5.0f) return 5.0f;
            return value;
        };
        g_Config.camera_offset_x = clampCameraOffset(g_Config.camera_offset_x);
        g_Config.camera_offset_z = clampCameraOffset(g_Config.camera_offset_z);
        g_Config.camera_offset_y = clampCameraOffset(g_Config.camera_offset_y);

        g_Config.camera_height_transition_speed =
            ReadFloat("CameraHeightTransitionSpeed", 8.0f, file);
        if (g_Config.camera_height_transition_speed < 0.1f) {
            g_Config.camera_height_transition_speed = 0.1f;
        } else if (g_Config.camera_height_transition_speed > 30.0f) {
            g_Config.camera_height_transition_speed = 30.0f;
        }

        g_Config.hide_quest_banner = ReadInt("HideQuestBanner", 0, file);
        
        g_Config.hide_uid = ReadInt("HideUID", 0, file);
        
        g_Config.disable_show_damage_text = ReadInt("DisableDamageText", 0, file);
        
        g_Config.disable_event_camera_move = ReadInt("DisableCameraMove", 0, file);
        
        g_Config.disable_fog = ReadInt("DisableFog", 0, file);
        
        g_Config.disable_character_fade = ReadInt("DisableCharFade", 0, file);
        
        g_Config.enable_redirect_craft_override = ReadInt("RedirectCraft", 0, file);
        
        g_Config.enable_remove_team_anim = ReadInt("RemoveTeamAnim", 0, file);
        
        g_Config.toggle_key = ReadInt("ToggleKey", VK_HOME, file);
        
        g_Config.craft_key = ReadInt("CraftKey", 0, file);
        
        g_Config.enable_custom_title = ReadInt("EnableCustomTitle", 0, file);
        
        char titleBuf[256] = {};
        ReadString("CustomTitleText", "FufuLauncher", titleBuf, sizeof(titleBuf), file);
        g_Config.custom_title_text = titleBuf;

        g_Config.dump_offsets = ReadInt("DumpOffsets", 0, file);
        

        g_Config.block_network = ReadInt("BlockNetwork", 0, file);
        
        g_Config.enable_network_toggle = ReadInt("EnableNetworkToggle", 0, file);
        
        g_Config.network_toggle_key = ReadInt("NetworkToggleKey", VK_F11, file);

        g_Config.enable_fov_limit_check = ReadInt("FovLimitCheck", 1, file);

        g_Config.hide_main_ui = ReadInt("HideMainUI", 0, file);

        g_Config.hide_profile_uid = ReadInt("HideProfileUID", 0, file);

        g_Config.hide_profile_birthday = ReadInt("HideProfileBirthday", 0, file);

        g_Config.use_system_cursor = ReadInt("UseSystemCursor", 0, file);

        g_Config.hide_grass = ReadInt("HideGrass", 0, file);

        g_Config.hide_grass_indiscriminate = ReadInt("HideGrassIndiscriminate", 0, file);

        g_Config.enable_clock_speedup = ReadInt("ClockSpeedup", 0, file);
        
        g_Config.enable_auto_cook = ReadInt("AutoCook", 0, file);
        g_Config.enable_auto_expedition = ReadInt("AutoExpedition", 0, file);
        
        g_Config.auto_cook_key = ReadInt("AutoCookKey", VK_F10, file);
        g_Config.auto_expedition_key = ReadInt("AutoExpeditionKey", VK_F9, file);

        g_Config.enable_rainbow_damage = ReadInt("EnableRainbowDamage", 0, file);
        g_Config.rainbow_damage_mode = ReadInt("RainbowDamageMode", 0, file);
        g_Config.rainbow_fixed_color_idx = ReadInt("RainbowFixedColorIdx", 0, file);

        g_Config.disable_underwater_mask = ReadInt("DisableUnderwaterMask", 0, file);

        g_Config.enable_low_render_scale = ReadInt("LowRenderScale", 0, file);
        g_Config.render_scale_value = ReadFloat("RenderScaleValue", 1.00f, file);

        if (g_Config.render_scale_value < 0.01f) {
            g_Config.render_scale_value = 0.01f;
        } else if (g_Config.render_scale_value > 3.00f) {
            g_Config.render_scale_value = 3.00f;
        }

        g_Config.enable_free_cam = ReadInt("EnableFreeCam", 0, file);
        g_Config.free_cam_key = ReadInt("FreeCamKey", VK_INSERT, file);
        g_Config.free_cam_lock_key = ReadInt("FreeCamLockKey", VK_DELETE, file);
        g_Config.free_cam_move_speed = ReadFloat("FreeCamMoveSpeed", 8.0f, file);
        g_Config.free_cam_sprint_mult = ReadFloat("FreeCamSprintMult", 3.0f, file);
        g_Config.free_cam_mouse_sensitivity = ReadFloat("FreeCamMouseSensitivity", 0.12f, file);

        g_Config.enable_paimon_follow = ReadInt("EnablePaimonFollow", 0, file);
    }
}

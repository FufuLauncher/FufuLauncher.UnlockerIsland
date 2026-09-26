/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#pragma once
#include <Windows.h>
#include <string>

struct ModConfig {
    bool debug_console = false;
    
    bool enable_vsync_override = true;
    
    bool enable_fps_override = false;

    bool enable_fps_clamp = true;
    
    int selected_fps = 60;
    
    bool enable_fov_override = false;
    
    float fov_value = 45.0f;

    bool enable_camera_offset = false;
    int camera_offset_key = VK_F6;
    float camera_offset_x = 0.0f;
    float camera_offset_z = 0.0f;
    float camera_offset_y = 0.0f;
    float camera_height_transition_speed = 8.0f;

    bool use_touch_screen = false;
    
    bool hide_quest_banner = false;
    
    bool hide_uid = false;
    
    bool disable_show_damage_text = false;
    
    bool disable_event_camera_move = false;
    
    bool disable_fog = false;
    
    bool disable_character_fade = false;
    
    bool enable_custom_title = false;
    
    std::string custom_title_text = "原神";

    bool enable_redirect_craft_override = false;
    
    bool enable_remove_team_anim = false;

    int toggle_key = VK_HOME;
    
    int craft_key = 0;

    bool dump_offsets = false;

    bool block_network = false;

    bool enable_network_toggle = false;
    
    int network_toggle_key = VK_F11;

    bool is_currently_blocking = false;

    bool enable_fov_limit_check = true;

    bool hide_main_ui = false;

    bool hide_profile_uid = false;

    bool hide_profile_birthday = false;

    bool use_system_cursor = false;

    bool hide_grass = false;

    bool hide_grass_indiscriminate = false;

    bool enable_clock_speedup = false;
    
    bool enable_auto_cook = false;
    bool enable_auto_expedition = false;
    
    int auto_cook_key = VK_F10;
    int auto_expedition_key = VK_F9;

    bool enable_rainbow_damage = false;
    int rainbow_damage_mode = 0;
    int rainbow_fixed_color_idx = 0;

    bool disable_underwater_mask = false;

    bool enable_low_render_scale = false;
    float render_scale_value = 1.00f;

    bool enable_free_cam = false;
    int free_cam_key = VK_INSERT;
    int free_cam_lock_key = VK_DELETE;
    float free_cam_move_speed = 8.0f;
    float free_cam_sprint_mult = 3.0f;
    float free_cam_mouse_sensitivity = 0.12f;

    bool enable_paimon_follow = false;
};

namespace Config {
    ModConfig& Get();
    void Load();
    void SaveOverlayPos(float x, float y);

    std::string GetConfigPath();
}

#pragma once
#include <Windows.h>
#include <string>

struct ModConfig {
    bool debug_console = false;
    
    bool enable_vsync_override = true;
    
    bool enable_fps_override = false;
    
    int selected_fps = 60;
    
    bool enable_fov_override = false;
    
    float fov_value = 45.0f;
    
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

    bool display_paimon = false;

    bool enable_free_cam = false;
    
    int free_cam_key = VK_F5;
    
    int free_cam_reset_key = VK_F7;

    bool enable_free_cam_movement_fix = true;

    bool enable_gamepad_hot_switch = false;

    bool hide_grass = false;

    bool  ResinItem000106;
    bool  ResinItem000201;
    bool  ResinItem107009;
    bool  ResinItem107012;
    bool  ResinItem220007;
};

namespace Config {
    ModConfig& Get();
    void Load();
    void SaveOverlayPos(float x, float y);
}

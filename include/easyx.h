#pragma once

#include "common/structure.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <graphics.h>

// EasyX显示布局集中管理，避免界面代码到处散落写死的坐标
namespace ui_layout
{
    const int window_width = 2600;
    const int window_height = 1500;
    const int map_width = 2050;
    const int panel_left = map_width;
    const int panel_right = window_width;

    const int network_left = 35;
    const int network_right = 2020;
    const int network_top = 35;
    const int network_bottom = 1460;

    const int logical_left = 40;
    const int logical_right = 890;
    const int logical_top = 60;
    const int logical_bottom = 610;

    // 总览校园枢纽，与校内底图坐标无关
    const int campus_logical_x = 175;
    const int campus_logical_y = 270;

    const int jiading_offset_x = 575;
    const int jiading_offset_y = 260;

    const int panel_content_left = 2090;
    const int panel_content_right = 2560;
}

bool enable_high_dpi_rendering();
void lock_easyx_window_size();
void run_easyx_interface(vertex vertices[], int vertex_number,
    const transit_line lines[], int line_number,
    const jiading_map_node map_nodes[], int map_node_number,
    const IMAGE& jiading_background, ui_state& state);

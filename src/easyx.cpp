#define _CRT_SECURE_NO_WARNINGS
#include "../include/easyx.h"
#include "../include/csv_data.h"
#include "../include/dijkstra.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <cstring>

using namespace std;

/***************************************************************************
  函数名称：get_entity_station_name
  功    能：取得用于经停统计和换乘显示的实体站名
  输入参数：const vertex vertices[]：顶点数组
  int vertex_id：物理顶点编号
  返 回 值：去掉地铁线路后缀后的实体站名
  说    明：只去除形如“（11号线）”的地铁线路后缀，公交站名中的普通括号内容保留
***************************************************************************/
string get_entity_station_name(const vertex vertices[], int vertex_id)
{
    if (vertex_id < 0 || vertex_id >= physical_vertex_number)
        return "未知站点";

    string name = vertices[vertex_id].name;
    size_t left_bracket = name.rfind("（");
    size_t line_suffix = string::npos;

    if (left_bracket != string::npos)
        line_suffix = name.find("号线）", left_bracket);

    if (left_bracket != string::npos && line_suffix != string::npos
        && line_suffix + string("号线）").size() == name.size())
        name = name.substr(0, left_bracket);

    return name;
}

/***************************************************************************
  函数名称：same_entity_station
  功    能：判断两个物理节点是否代表同一个实体站点
  输入参数：const vertex vertices[]：顶点数组
  int first_vertex：第一个物理节点编号
  int second_vertex：第二个物理节点编号
  返 回 值：实体站名相同返回true，否则返回false
  说    明：用于合并同一地铁换乘站在不同线路上的两个图节点
***************************************************************************/
bool same_entity_station(const vertex vertices[],
    int first_vertex, int second_vertex)
{
    if (first_vertex < 0 || first_vertex >= physical_vertex_number
        || second_vertex < 0 || second_vertex >= physical_vertex_number)
        return false;

    return get_entity_station_name(vertices, first_vertex)
        == get_entity_station_name(vertices, second_vertex);
}

/***************************************************************************
  函数名称：map_to_physical_vertex
  功    能：把公交虚拟状态节点映射回对应的可见物理节点
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  int vertex_id：待映射顶点编号
  返 回 值：成功返回物理节点编号，失败返回-1
  说    明：前92个节点直接返回自身；虚拟节点根据名称和总览坐标寻找对应物理节点
***************************************************************************/
int map_to_physical_vertex(const vertex vertices[], int vertex_number,
    int vertex_id)
{
    if (!is_valid_vertex_id(vertex_id, vertex_number))
        return -1;

    if (vertex_id < physical_vertex_number)
        return vertex_id;

    int physical_limit = physical_vertex_number;
    if (vertex_number < physical_limit)
        physical_limit = vertex_number;

    for (int i = 0; i < physical_limit; i++) {
        if (vertices[vertex_id].name == vertices[i].name
            && vertices[vertex_id].x == vertices[i].x
            && vertices[vertex_id].y == vertices[i].y)
            return i;
    }

    return -1;
}

/***************************************************************************
  函数名称：build_route_segments
  功    能：把Dijkstra原始path压缩成用户可读的路线阶段，并统计经停站数
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  const int path[]：终点到起点的倒序路径数组
  int path_vertex_number：路径顶点数量
  const edge_node* previous_edge[]：Dijkstra实际采用的前驱边
  bool allow_bike：是否允许骑行
  route_segment segments[]：输出路线阶段数组
  int& segment_number：输出路线阶段数量
  int& stop_number：输出全程经停站数
  返 回 值：成功返回true，路径映射或前驱边异常返回false
  说    明：同type和line_id连续合并；步行和骑行分别合并；公交状态边累计成本但不增加站间数
***************************************************************************/
bool build_route_segments(const vertex vertices[], int vertex_number,
    const int path[], int path_vertex_number,
    const edge_node* previous_edge[], bool allow_bike,
    route_segment segments[], int& segment_number,
    int& stop_number)
{
    segment_number = 0;
    stop_number = 0;

    if (path_vertex_number < 2 || path_vertex_number > vertex_number)
        return false;

    int entity_path_number = 0;
    string last_entity_name = "";

    for (int i = path_vertex_number - 1; i >= 0; i--) {
        int physical_vertex =
            map_to_physical_vertex(vertices, vertex_number, path[i]);

        if (physical_vertex < 0)
            return false;

        string entity_name =
            get_entity_station_name(vertices, physical_vertex);

        if (entity_path_number == 0 || entity_name != last_entity_name) {
            entity_path_number++;
            last_entity_name = entity_name;
        }
    }

    if (entity_path_number > 2)
        stop_number = entity_path_number - 2;

    for (int i = path_vertex_number - 1; i > 0; i--) {
        int raw_from_vertex = path[i];
        int raw_to_vertex = path[i - 1];
        const edge_node* current_edge = previous_edge[raw_to_vertex];

        if (!current_edge)
            return false;

        int physical_from_vertex =
            map_to_physical_vertex(vertices, vertex_number,
                raw_from_vertex);
        int physical_to_vertex =
            map_to_physical_vertex(vertices, vertex_number,
                raw_to_vertex);

        if (physical_from_vertex < 0 || physical_to_vertex < 0)
            return false;

        int effective_time =
            get_effective_time_cost(*current_edge, allow_bike);
        bool use_bike =
            current_edge->type == edge_type::TRANSFER
            && effective_time < current_edge->time_cost;

        bool can_merge = false;
        if (segment_number > 0) {
            route_segment& last_segment = segments[segment_number - 1];

            can_merge =
                last_segment.type == current_edge->type
                && last_segment.line_id == current_edge->line_id
                && last_segment.end_vertex == physical_from_vertex;

            if (current_edge->type == edge_type::TRANSFER
                && last_segment.use_bike != use_bike)
                can_merge = false;
        }

        if (can_merge) {
            route_segment& last_segment = segments[segment_number - 1];
            last_segment.end_vertex = physical_to_vertex;
            last_segment.time_cost += effective_time;
            last_segment.fare_cost += current_edge->fare_cost;
            if (!same_entity_station(vertices,
                physical_from_vertex, physical_to_vertex))
                last_segment.station_edge_number++;
        }
        else {
            if (segment_number >= max_vertices)
                return false;

            route_segment& new_segment = segments[segment_number];
            new_segment = route_segment{};
            new_segment.start_vertex = physical_from_vertex;
            new_segment.end_vertex = physical_to_vertex;
            new_segment.type = current_edge->type;
            new_segment.line_id = current_edge->line_id;
            new_segment.use_bike = use_bike;
            new_segment.time_cost = effective_time;
            new_segment.fare_cost = current_edge->fare_cost;
            if (!same_entity_station(vertices,
                physical_from_vertex, physical_to_vertex))
                new_segment.station_edge_number = 1;
            segment_number++;
        }
    }

    return segment_number > 0;
}

/***************************************************************************
  函数名称：is_public_transport_segment
  功    能：判断路线阶段是否属于公共交通阶段
  输入参数：const route_segment& segment：路线阶段
  返 回 值：地铁或公交返回true，步行/骑行返回false
  说    明：换乘次数只在前后两段公共交通之间产生
***************************************************************************/
bool is_public_transport_segment(const route_segment& segment)
{
    return segment.type == edge_type::METRO
        || segment.type == edge_type::BUS;
}

/***************************************************************************
  函数名称：get_transfer_count
  功    能：根据压缩后的路线阶段统计真正的换乘次数
  输入参数：const ui_state& state：当前路线状态
  返 回 值：换乘次数
  说    明：开头或结尾的步行不算换乘；只有后续再次进入公共交通时才增加一次
***************************************************************************/
int get_transfer_count(const ui_state& state)
{
    int transfer_count = 0;
    int previous_public_segment = -1;

    for (int i = 0; i < state.segment_number; i++) {
        if (!is_public_transport_segment(state.segments[i]))
            continue;

        if (previous_public_segment >= 0)
            transfer_count++;

        previous_public_segment = i;
    }

    return transfer_count;
}

/***************************************************************************
  函数名称：get_transfer_summary
  功    能：生成右栏换乘站或步行接驳位置摘要
  输入参数：const vertex vertices[]：顶点数组
  const ui_state& state：当前路线状态
  返 回 值：多个换乘位置用顿号连接的字符串
  说    明：同一实体站换线只显示一个站名；步行接驳显示“起点->终点”
***************************************************************************/
string get_transfer_summary(const vertex vertices[], const ui_state& state)
{
    string summary;
    int previous_public_segment = -1;

    for (int i = 0; i < state.segment_number; i++) {
        if (!is_public_transport_segment(state.segments[i]))
            continue;

        if (previous_public_segment >= 0) {
            int from_vertex =
                state.segments[previous_public_segment].end_vertex;
            int to_vertex = state.segments[i].start_vertex;

            string from_name =
                get_entity_station_name(vertices, from_vertex);
            string to_name =
                get_entity_station_name(vertices, to_vertex);

            if (!summary.empty())
                summary += "、";

            if (same_entity_station(vertices, from_vertex, to_vertex))
                summary += from_name;
            else
                summary += from_name + "->" + to_name;
        }

        previous_public_segment = i;
    }

    return summary;
}

/***************************************************************************
  函数名称：enable_high_dpi_rendering
  功    能：在创建EasyX窗口前启用高DPI感知
  输入参数：无
  返 回 值：成功设置DPI模式返回true，否则返回false
  说    明：优先使用Per-Monitor V2，避免Windows对固定像素画布做二次模糊缩放
***************************************************************************/
bool enable_high_dpi_rendering()
{
    HMODULE user32_module = GetModuleHandleA("user32.dll");
    if (!user32_module)
        return SetProcessDPIAware() != FALSE;

    using set_process_dpi_context_function = BOOL(WINAPI*)(HANDLE);
    using set_thread_dpi_context_function = HANDLE(WINAPI*)(HANDLE);

    set_process_dpi_context_function set_process_context =
        reinterpret_cast<set_process_dpi_context_function>(
            GetProcAddress(user32_module, "SetProcessDpiAwarenessContext"));
    set_thread_dpi_context_function set_thread_context =
        reinterpret_cast<set_thread_dpi_context_function>(
            GetProcAddress(user32_module, "SetThreadDpiAwarenessContext"));

    HANDLE per_monitor_v2_context =
        reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4));

    bool enabled = false;
    if (set_process_context)
        enabled = set_process_context(per_monitor_v2_context) != FALSE;
    if (set_thread_context)
        enabled = set_thread_context(per_monitor_v2_context) != nullptr || enabled;

    if (enabled)
        return true;

    return SetProcessDPIAware() != FALSE;
}

struct screen_point {
    int x = 0;
    int y = 0;
};

struct station_label_rectangle {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

struct station_label_text {
    string first_line = "";
    string second_line = "";
    int width = 0;
    int height = 0;
    int line_number = 0;
};

/***************************************************************************
  函数名称：lock_easyx_window_size
  功    能：固定窗口尺寸并放在适合一镜到底录屏的位置
  输入参数：无
  返 回 值：无
  说    明：1500乘800可在1600乘900屏幕中保留标题栏和任务栏，便于系统时间持续可见
***************************************************************************/
void lock_easyx_window_size()
{
    HWND easyx_window = GetHWnd();
    if (!easyx_window)
        return;

    LONG_PTR window_style = GetWindowLongPtr(easyx_window, GWL_STYLE);
    window_style &= ~static_cast<LONG_PTR>(WS_THICKFRAME | WS_MAXIMIZEBOX);
    SetWindowLongPtr(easyx_window, GWL_STYLE, window_style);

    SetWindowPos(easyx_window, nullptr,
        18, 8, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

/***************************************************************************
  函数名称：set_ui_font
  功    能：设置右侧界面使用的清晰中文字体
  输入参数：int font_height：字体高度；int font_weight：字体粗细
  返 回 值：无
  说    明：使用Microsoft YaHei UI和ClearType，字符集固定为GB2312
***************************************************************************/
void set_ui_font(int font_height, int font_weight = FW_NORMAL)
{
    LOGFONT text_font = {};
    text_font.lfHeight = -font_height;
    text_font.lfWeight = font_weight;
    text_font.lfCharSet = GB2312_CHARSET;
    text_font.lfOutPrecision = OUT_TT_PRECIS;
    text_font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    text_font.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    text_font.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    std::strncpy(text_font.lfFaceName,
        "Microsoft YaHei UI", LF_FACESIZE - 1);
    text_font.lfFaceName[LF_FACESIZE - 1] = '\0';
    settextstyle(&text_font);
}

/***************************************************************************
  函数名称：set_map_font
  功    能：设置总览线路图站名字体
  输入参数：int font_height：字体高度；int font_weight：字体粗细
  返 回 值：无
  说    明：小字号地图标签使用SimSun以获得更清楚的中文笔画
***************************************************************************/
void set_map_font(int font_height, int font_weight = FW_NORMAL)
{
    LOGFONT text_font = {};
    text_font.lfHeight = -font_height;
    text_font.lfWeight = font_weight;
    text_font.lfCharSet = GB2312_CHARSET;
    text_font.lfOutPrecision = OUT_TT_PRECIS;
    text_font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    text_font.lfQuality = CLEARTYPE_QUALITY;
    text_font.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
    std::strncpy(text_font.lfFaceName,
        "SimSun", LF_FACESIZE - 1);
    text_font.lfFaceName[LF_FACESIZE - 1] = '\0';
    settextstyle(&text_font);
}

/***************************************************************************
  函数名称：scale_total_map_x
  功    能：把CSV逻辑x坐标转换为新版总览图屏幕坐标
  输入参数：int logical_x：CSV逻辑坐标
  返 回 值：EasyX屏幕x坐标
  说    明：数据坐标与窗口坐标解耦，以后调整录屏窗口不必重写CSV
***************************************************************************/
int scale_total_map_x(int logical_x)
{
    if (logical_x < ui_layout::logical_left)
        logical_x = ui_layout::logical_left;
    if (logical_x > ui_layout::logical_right)
        logical_x = ui_layout::logical_right;

    return ui_layout::network_left
        + (logical_x - ui_layout::logical_left)
        * (ui_layout::network_right - ui_layout::network_left)
        / (ui_layout::logical_right - ui_layout::logical_left);
}

/***************************************************************************
  函数名称：scale_total_map_y
  功    能：把CSV逻辑y坐标转换为新版总览图屏幕坐标
  输入参数：int logical_y：CSV逻辑坐标
  返 回 值：EasyX屏幕y坐标
  说    明：网络主体占据上方区域，底部给嘉定校内完整站名留空间
***************************************************************************/
int scale_total_map_y(int logical_y)
{
    if (logical_y < ui_layout::logical_top)
        logical_y = ui_layout::logical_top;
    if (logical_y > ui_layout::logical_bottom)
        logical_y = ui_layout::logical_bottom;

    return ui_layout::network_top
        + (logical_y - ui_layout::logical_top)
        * (ui_layout::network_bottom - ui_layout::network_top)
        / (ui_layout::logical_bottom - ui_layout::logical_top);
}

/***************************************************************************
  函数名称：is_jiading_internal_vertex
  功    能：判断物理节点是否属于嘉定校区内部
  输入参数：int vertex_id：物理节点编号
  返 回 值：56到63号中除59外的校内节点返回true，否则返回false
  说    明：总览将校内节点收束为校园枢纽；59号校外公交站单独显示
***************************************************************************/
bool is_jiading_internal_vertex(int vertex_id)
{
    // 59号是绿苑路曹安公路公交站，不属于校内节点
    return vertex_id >= 56 && vertex_id <= 63 && vertex_id != 59;
}

// 嘉定校区—封浜共线走廊的校园端。59号单独显示，
// 其余校内节点在总览图中仍收束到嘉定校区枢纽。
bool is_jiading_corridor_terminal(int vertex_id)
{
    return vertex_id == 59 || is_jiading_internal_vertex(vertex_id);
}

/***************************************************************************
  函数名称：get_total_map_point
  功    能：取得真实或虚拟节点在总览图中的统一屏幕坐标
  输入参数：const vertex vertices[]：顶点数组；int vertex_number：顶点数量；int vertex_id：节点编号
  返 回 值：screen_point
  说    明：公交虚拟节点先映射物理节点，嘉定校内节点统一收束到校园枢纽
***************************************************************************/
screen_point get_total_map_point(const vertex vertices[],
    int vertex_number, int vertex_id)
{
    int physical_vertex = vertex_id;
    if (vertex_id >= physical_vertex_number) {
        int mapped = map_to_physical_vertex(vertices, vertex_number, vertex_id);
        if (mapped >= 0)
            physical_vertex = mapped;
    }

    int logical_x = vertices[vertex_id].x;
    int logical_y = vertices[vertex_id].y;

    if (physical_vertex >= 0
        && physical_vertex < physical_vertex_number
        && is_jiading_internal_vertex(physical_vertex)) {
        logical_x = ui_layout::campus_logical_x;
        logical_y = ui_layout::campus_logical_y;
    }

    // 总览使用示意坐标：参考用户提供的GCJ-02相对方位，密集站适度展开。
    // 132路在校园西北侧折返，112路沿校园西南侧向东；公交节点避开11号线。
    // 封浜公交站—嘉定校区的北安跨、DZ1、822共用312国道/曹安公路同一走廊。
    // 虚拟节点按同一physical_vertex定位，底图、选线和点击共用这组坐标。
    switch (physical_vertex) {
        case 29: logical_x = 115; logical_y = 220; break; // 昌吉东路（11号线）
        case 30: logical_x = 45; logical_y = 279; break; // 上海汽车城（11号线）
        case 31: logical_x -= 5; break; // 豫园（14号线）：向左靠近10号线豫园
        case 59: logical_x = ui_layout::campus_logical_x - 5; logical_y = ui_layout::campus_logical_y + 5; break; // 绿苑路曹安公路（保持原绿苑路位置）
        case 64: logical_x = 118; logical_y = 225; break; // 公交昌吉东路站
        case 65: logical_x = 102; logical_y = 239; break; // 百安公路双浦路
        case 66: logical_x = 88; logical_y = 253; break; // 百安公路于塘南路
        case 67: logical_x = 102; logical_y = 266; break; // 于塘南路望融路
        case 68: logical_x = 139; logical_y = 270; break; // 昌吉东路安虹北路
        case 69: logical_x = ui_layout::campus_logical_x + 2; logical_y = ui_layout::campus_logical_y - 4; break; // 昌吉东路绿苑路：仅保留点，紧贴嘉定校区北侧
        case 70: logical_x = 57; logical_y = 279; break; // 曹安公路安谐路
        case 71: logical_x = 78; logical_y = 286; break; // 曹安公路于田路
        case 72: logical_x = 109; logical_y = 292; break; // 曹安公路安虹路
        case 73: logical_x = 143; logical_y = 294; break; // 曹安公路二十三号桥
        case 84: logical_x = 196; logical_y = 279; break; // 曹安公路嘉松北路
        case 83: logical_x = 210; logical_y = 285; break; // 曹安公路新黄公路
        case 82: logical_x = 231; logical_y = 294; break; // 曹安公路许家东街村
        case 81: logical_x = 245; logical_y = 300; break; // 曹安公路星塔路
        case 80: logical_x = 259; logical_y = 306; break; // 曹安公路联群路
        case 79: logical_x = 273; logical_y = 312; break; // 曹安公路联西路
        case 78: logical_x = 287; logical_y = 318; break; // 曹安公路宝园七路
        case 77: logical_x = 308; logical_y = 327; break; // 曹安公路宝园五路
        case 76: logical_x = 322; logical_y = 333; break; // 曹安公路曹丰路
        case 75: logical_x = 343; logical_y = 342; break; // 曹安公路翔江路
        case 74: logical_x = 371; logical_y = 354; break; // 封浜公交站
        case 47: logical_x = 375; logical_y = 349; break; // 封浜（14号线）
        case 46: logical_x = 401; logical_y = 355; break; // 乐秀路（14号线）
        case 45: logical_x = 429; logical_y = 362; break; // 临洮路（14号线）
        default: break;
    }

    screen_point point;
    point.x = scale_total_map_x(logical_x);
    point.y = scale_total_map_y(logical_y);
    return point;
}

// 一条算法边可共用道路折点；折点只负责显示，不代表新增停站。
// DZ1、822、北安跨在线路图中共用312国道/曹安公路这一条走廊；
// 822显示沿途站点，DZ1和北安跨虽然算法上是直达边，绘图仍沿同一组道路点。
// 132路直接使用重新整理后的站点示意坐标，避免局部折返形成视觉假环。
int get_total_map_edge_points(const vertex vertices[], int vertex_number,
    int from, int to, const edge_node& edge, screen_point points[])
{
    points[0] = get_total_map_point(vertices, vertex_number, from);
    points[1] = get_total_map_point(vertices, vertex_number, to);
    int a = map_to_physical_vertex(vertices, vertex_number, from);
    int b = map_to_physical_vertex(vertices, vertex_number, to);

    // DZ1(4)和北安跨(6)的嘉定校区—封浜段与822完全共线。
    if (edge.type == edge_type::BUS
        && (edge.line_id == 4 || edge.line_id == 6)
        && ((a == 74 && is_jiading_corridor_terminal(b))
            || (b == 74 && is_jiading_corridor_terminal(a)))) {
        int count = 0;
        if (is_jiading_corridor_terminal(a)) {
            points[count++] = get_total_map_point(vertices, vertex_number, from);
            for (int station = 84; station >= 74; --station)
                points[count++] = get_total_map_point(vertices, vertex_number, station);
        }
        else {
            for (int station = 74; station <= 84; ++station)
                points[count++] = get_total_map_point(vertices, vertex_number, station);
            points[count++] = get_total_map_point(vertices, vertex_number, to);
        }
        return count;
    }

    // 上海汽车城短驳车与112路在校园—安谐路段共用曹安公路走廊。
    if (edge.type == edge_type::BUS && edge.line_id == 9
        && ((a == 70 && is_jiading_corridor_terminal(b))
            || (b == 70 && is_jiading_corridor_terminal(a)))) {
        points[4] = points[1];
        for (int i = 0; i < 3; ++i)
            points[i + 1] = get_total_map_point(vertices, vertex_number,
                a == 70 ? 71 + i : 73 - i);
        return 5;
    }

    return 2;
}

/***************************************************************************
  函数名称：set_network_edge_style
  功    能：按照线路类型设置总览图基础运营线路样式
  输入参数：const edge_node& edge：待绘制边
  返 回 值：无
  说    明：地铁最醒目，普通公交次之，教师班车使用较淡虚线
***************************************************************************/
void set_network_edge_style(const edge_node& edge)
{
    if (edge.type == edge_type::BUS) {
        if (edge.line_id == 10 || edge.line_id == 11) {
            setlinecolor(RGB(151, 177, 161));
            setlinestyle(PS_DASH, 2);
        }
        else {
            setlinecolor(RGB(63, 143, 181));
            setlinestyle(PS_SOLID, 3);
        }
        return;
    }

    switch (edge.line_id) {
        case 0:
            setlinecolor(RGB(111, 75, 126));
            break;
        case 1:
            setlinecolor(RGB(124, 74, 43));
            break;
        case 2:
            setlinecolor(RGB(128, 119, 20));
            break;
        case 3:
            setlinecolor(RGB(184, 45, 58));
            break;
        default:
            setlinecolor(RGB(70, 100, 150));
            break;
    }
    setlinestyle(PS_SOLID, 6);
}

/***************************************************************************
  函数名称：draw_base_network
  功    能：绘制总览图中长期可见的地铁和公交运营线路
  输入参数：const vertex vertices[]：顶点数组；int vertex_number：顶点数量
  返 回 值：无
  说    明：TRANSFER只属于算法可达关系，不在底图常驻绘制，从根源减少蜘蛛网线条
***************************************************************************/
void draw_base_network(const vertex vertices[], int vertex_number)
{
    for (int i = 0; i < vertex_number; i++) {
        const edge_node* current_edge = vertices[i].first_edge;

        while (current_edge) {
            if (i < current_edge->to
                && current_edge->type != edge_type::TRANSFER) {
                int from_physical = map_to_physical_vertex(vertices, vertex_number, i);
                int to_physical = map_to_physical_vertex(
                    vertices, vertex_number, current_edge->to);

                // 嘉定校区—封浜只有一条312国道/曹安公路物理走廊。
                // 底图由822路(line 5)绘制一次；DZ1(line 4)和北安跨(line 6)
                // 在这一段不重复落笔，但选中路线时仍会沿同一几何高亮。
                bool shared_fengbang_corridor_duplicate =
                    current_edge->type == edge_type::BUS
                    && (current_edge->line_id == 4 || current_edge->line_id == 6)
                    && ((from_physical == 74 && is_jiading_corridor_terminal(to_physical))
                        || (to_physical == 74 && is_jiading_corridor_terminal(from_physical)));

                if (!shared_fengbang_corridor_duplicate) {
                    screen_point from_point = get_total_map_point(vertices, vertex_number, i);
                    screen_point to_point = get_total_map_point(
                        vertices, vertex_number, current_edge->to);

                    if (from_point.x != to_point.x || from_point.y != to_point.y) {
                        set_network_edge_style(*current_edge);
                        screen_point points[16];
                        int count = get_total_map_edge_points(vertices, vertex_number,
                            i, current_edge->to, *current_edge, points);
                        for (int p = 1; p < count; ++p)
                            line(points[p - 1].x, points[p - 1].y, points[p].x, points[p].y);
                    }
                }
            }
            current_edge = current_edge->next;
        }
    }

    setlinestyle(PS_SOLID, 1);
}

/***************************************************************************
  函数名称：draw_route_highlight
  功    能：在总览图基础线路上绘制黄色推荐路线
  输入参数：const vertex vertices[]：顶点数组；int vertex_number：顶点数量；const ui_state& state：界面状态
  返 回 值：无
  说    明：只有最终路线真正采用TRANSFER时才显示对应步行或骑行连接
***************************************************************************/
void draw_route_highlight(const vertex vertices[], int vertex_number,
    const ui_state& state)
{
    if (!state.route_ready || state.path_vertex_number < 2)
        return;

    for (int i = state.path_vertex_number - 1; i > 0; i--) {
        int from_vertex = state.path[i];
        int to_vertex = state.path[i - 1];
        const edge_node* route_edge = state.previous_edge[to_vertex];
        if (!route_edge)
            continue;

        screen_point from_point = get_total_map_point(vertices, vertex_number, from_vertex);
        screen_point to_point = get_total_map_point(vertices, vertex_number, to_vertex);

        if (from_point.x == to_point.x && from_point.y == to_point.y)
            continue;

        setlinecolor(RGB(255, 201, 35));
        if (route_edge->type == edge_type::TRANSFER)
            setlinestyle(PS_DASH, 5);
        else
            setlinestyle(PS_SOLID, 10);

        screen_point points[16];
        int count = get_total_map_edge_points(vertices, vertex_number,
            from_vertex, to_vertex, *route_edge, points);
        for (int p = 1; p < count; ++p)
            line(points[p - 1].x, points[p - 1].y, points[p].x, points[p].y);
    }

    setlinestyle(PS_SOLID, 1);
}

/***************************************************************************
  函数名称：draw_total_map_vertices
  功    能：绘制总览图中的实体站点圆圈
  输入参数：const vertex vertices[]：顶点数组；int vertex_number：顶点数量
  返 回 值：无
  说    明：同实体换乘站只画一个圆圈，嘉定校内节点收束成一个校园枢纽，59号校外公交站单独显示
***************************************************************************/
void draw_total_map_vertices(const vertex vertices[], int vertex_number)
{
    int physical_limit = vertex_number;
    if (physical_limit > physical_vertex_number)
        physical_limit = physical_vertex_number;

    string drawn_names[physical_vertex_number];
    int drawn_name_number = 0;

    for (int i = 0; i < physical_limit; i++) {
        if (is_jiading_internal_vertex(i))
            continue;

        string entity_name = get_entity_station_name(vertices, i);
        bool already_drawn = false;

        for (int j = 0; j < drawn_name_number; j++) {
            if (drawn_names[j] == entity_name) {
                already_drawn = true;
                break;
            }
        }

        // 14号线豫园(31)在总览图中与10号线豫园略微错开，
        // 因此即使实体站名相同，也需要单独补画一个站点圆圈。
        if (already_drawn && i != 31)
            continue;

        if (!already_drawn)
            drawn_names[drawn_name_number++] = entity_name;
        screen_point point = get_total_map_point(vertices, vertex_number, i);

        setfillcolor(RGB(255, 255, 255));
        if (vertices[i].type == station_type::METRO)
            setlinecolor(RGB(47, 117, 196));
        else
            setlinecolor(RGB(216, 119, 39));

        fillcircle(point.x, point.y, 7);
    }

    screen_point campus_point;
    campus_point.x = scale_total_map_x(ui_layout::campus_logical_x);
    campus_point.y = scale_total_map_y(ui_layout::campus_logical_y);
    setfillcolor(RGB(255, 255, 255));
    setlinecolor(RGB(216, 119, 39));
    fillcircle(campus_point.x, campus_point.y, 10);
}

/***************************************************************************
  函数名称：build_station_label_text
  功    能：把较长站名压缩为最多两行显示
  输入参数：const string& text：完整站名；int max_width：单行最大像素宽度
  返 回 值：station_label_text
  说    明：右侧控制面板仍需动态处理文本；总览图标签不再调用此函数计算位置
***************************************************************************/
station_label_text build_station_label_text(const string& text, int max_width)
{
    station_label_text result;

    if (textwidth(text.c_str()) <= max_width) {
        result.first_line = text;
        result.width = textwidth(text.c_str());
        result.height = 22;
        result.line_number = 1;
        return result;
    }

    string first_line;
    string second_line;
    size_t position = 0;
    bool use_second_line = false;

    while (position < text.size()) {
        unsigned char first_byte = static_cast<unsigned char>(text[position]);
        size_t char_length =
            (first_byte >= 0x81 && first_byte <= 0xFE && position + 1 < text.size()) ? 2 : 1;
        string one_character = text.substr(position, char_length);

        if (!use_second_line) {
            string candidate = first_line + one_character;
            if (!first_line.empty() && textwidth(candidate.c_str()) > max_width)
                use_second_line = true;
            else
                first_line = candidate;
        }

        if (use_second_line)
            second_line += one_character;

        position += char_length;
    }

    result.first_line = first_line;
    result.second_line = second_line;
    result.width = textwidth(first_line.c_str());
    int second_width = textwidth(second_line.c_str());
    if (second_width > result.width)
        result.width = second_width;
    result.height = 44;
    result.line_number = 2;
    return result;
}

// 总览图标签已经由最终确认版一次性烘焙到 data/map_labels.csv。
// 运行时只读取固定矩形、固定换行和固定引线，不再执行碰撞检测、候选搜索或人工二次微调。
struct map_label_item {
    int draw_order = -1;
    int vertex_id = -1; // -1代表收束后的嘉定校区标签
    string entity_name;
    screen_point anchor;
    station_label_text text;
    station_label_rectangle rectangle;
    bool leader = false;
    screen_point leader_start;
    screen_point leader_end;
};

struct map_label_layout {
    vector<map_label_item> items;
    bool loaded = false;
};

map_label_layout total_map_labels;

/***************************************************************************
  函数名称：split_fixed_map_label_csv_row
  功    能：解析 map_labels.csv 的一行，兼容 Excel 写出的双引号字段
  输入参数：原始行文本、输出字段数组
  返 回 值：格式合法返回true
  说    明：CP936/GBK 中文字节不会与英文逗号、双引号冲突，可按字节安全解析
***************************************************************************/
bool split_fixed_map_label_csv_row(const string& line_text, vector<string>& fields)
{
    fields.clear();
    string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line_text.size(); ++i) {
        char ch = line_text[i];
        if (in_quotes) {
            if (ch == '"') {
                if (i + 1 < line_text.size() && line_text[i + 1] == '"') {
                    field += '"';
                    ++i;
                }
                else {
                    in_quotes = false;
                }
            }
            else {
                field += ch;
            }
        }
        else {
            if (ch == ',') {
                fields.push_back(field);
                field.clear();
            }
            else if (ch == '"') {
                if (!field.empty())
                    return false;
                in_quotes = true;
            }
            else {
                field += ch;
            }
        }
    }

    if (in_quotes)
        return false;

    fields.push_back(field);
    return true;
}

/***************************************************************************
  函数名称：parse_fixed_map_label_integer
  功    能：严格解析固定标签CSV中的整数
  输入参数：文本、输出整数
  返 回 值：完整字段为合法int时返回true
***************************************************************************/
bool parse_fixed_map_label_integer(const string& text, int& value)
{
    try {
        size_t used = 0;
        int parsed = stoi(text, &used);
        if (used != text.size())
            return false;
        value = parsed;
        return true;
    }
    catch (const invalid_argument&) {
        return false;
    }
    catch (const out_of_range&) {
        return false;
    }
}

/***************************************************************************
  函数名称：load_fixed_total_map_labels
  功    能：读取已经人工确认并烘焙完成的总览图标签快照
  输入参数：CSV路径
  返 回 值：成功返回true
  说    明：label_left/top 等均为2600x1500界面中的最终屏幕像素；运行时不再重新布局
***************************************************************************/
bool load_fixed_total_map_labels(const string& file_path)
{
    ifstream input(file_path, ios::binary);
    if (!input.is_open()) {
        cout << "总览图固定标签CSV无法打开: " << file_path << endl;
        return false;
    }

    string header;
    if (!getline(input, header)) {
        cout << "总览图固定标签CSV为空" << endl;
        return false;
    }
    remove_trailing_carriage_return(header);
    if (header.substr(0, 3) == "\xEF\xBB\xBF")
        header.erase(0, 3);

    const string expected_header =
        "draw_order,vertex_id,entity_name,anchor_x,anchor_y,"
        "label_left,label_top,label_right,label_bottom,text_width,text_height,"
        "line_number,first_line,second_line,leader,leader_start_x,leader_start_y,"
        "leader_end_x,leader_end_y";
    if (header != expected_header) {
        cout << "总览图固定标签CSV表头不正确，请检查 data/map_labels.csv" << endl;
        return false;
    }

    vector<map_label_item> loaded_items;
    string data_line;
    int source_line_number = 1;
    while (getline(input, data_line)) {
        ++source_line_number;
        remove_trailing_carriage_return(data_line);
        if (data_line.empty())
            continue;

        vector<string> fields;
        if (!split_fixed_map_label_csv_row(data_line, fields) || fields.size() != 19) {
            cout << "总览图固定标签CSV第" << source_line_number
                << "行字段数量或引号格式错误" << endl;
            return false;
        }

        map_label_item item;
        int leader_value = 0;
        bool numeric_ok =
            parse_fixed_map_label_integer(fields[0], item.draw_order)
            && parse_fixed_map_label_integer(fields[1], item.vertex_id)
            && parse_fixed_map_label_integer(fields[3], item.anchor.x)
            && parse_fixed_map_label_integer(fields[4], item.anchor.y)
            && parse_fixed_map_label_integer(fields[5], item.rectangle.left)
            && parse_fixed_map_label_integer(fields[6], item.rectangle.top)
            && parse_fixed_map_label_integer(fields[7], item.rectangle.right)
            && parse_fixed_map_label_integer(fields[8], item.rectangle.bottom)
            && parse_fixed_map_label_integer(fields[9], item.text.width)
            && parse_fixed_map_label_integer(fields[10], item.text.height)
            && parse_fixed_map_label_integer(fields[11], item.text.line_number)
            && parse_fixed_map_label_integer(fields[14], leader_value)
            && parse_fixed_map_label_integer(fields[15], item.leader_start.x)
            && parse_fixed_map_label_integer(fields[16], item.leader_start.y)
            && parse_fixed_map_label_integer(fields[17], item.leader_end.x)
            && parse_fixed_map_label_integer(fields[18], item.leader_end.y);
        if (!numeric_ok) {
            cout << "总览图固定标签CSV第" << source_line_number
                << "行存在非法整数" << endl;
            return false;
        }

        item.entity_name = fields[2];
        item.text.first_line = fields[12];
        item.text.second_line = fields[13];
        item.leader = leader_value != 0;

        if (item.draw_order < 0
            || item.text.line_number < 1 || item.text.line_number > 2
            || item.rectangle.left > item.rectangle.right
            || item.rectangle.top > item.rectangle.bottom
            || item.text.width < 0 || item.text.height < 0) {
            cout << "总览图固定标签CSV第" << source_line_number
                << "行数据范围不合法" << endl;
            return false;
        }

        loaded_items.push_back(item);
    }
    input.close();

    if (loaded_items.empty()) {
        cout << "总览图固定标签CSV没有标签数据" << endl;
        return false;
    }

    stable_sort(loaded_items.begin(), loaded_items.end(),
        [](const map_label_item& first, const map_label_item& second) {
            return first.draw_order < second.draw_order;
        });

    for (size_t i = 0; i < loaded_items.size(); ++i) {
        if (loaded_items[i].draw_order != static_cast<int>(i)) {
            cout << "总览图固定标签CSV的draw_order必须从0连续编号" << endl;
            return false;
        }
    }

    total_map_labels.items.swap(loaded_items);
    total_map_labels.loaded = true;
    return true;
}

/***************************************************************************
  函数名称：prepare_total_map_labels
  功    能：保证固定标签快照已经加载
  输入参数：保留原接口参数，避免影响其它EasyX绘制层
  返 回 值：无
  说    明：这里不再读取站点位置进行任何标签计算，视觉结果完全由map_labels.csv决定
***************************************************************************/
void prepare_total_map_labels(const vertex vertices[], int vertex_number)
{
    (void)vertices;
    (void)vertex_number;
    if (total_map_labels.loaded)
        return;

    if (!load_fixed_total_map_labels("data/map_labels.csv"))
        throw runtime_error("Fixed map labels cannot be loaded.");
}

void draw_station_label(const station_label_rectangle& box,
    const station_label_text& label)
{
    setfillcolor(RGB(247, 250, 252));
    solidrectangle(box.left - 2, box.top - 1, box.right + 2, box.bottom + 1);
    set_map_font(20);
    setbkmode(TRANSPARENT);
    settextcolor(RGB(28, 36, 48));
    outtextxy(box.left, box.top, label.first_line.c_str());
    if (label.line_number == 2)
        outtextxy(box.left, box.top + 22, label.second_line.c_str());
}

// 引线端点已经随标签一起烘焙；不再按距离、穿越关系或站号进行运行时判断。
void draw_total_map_label_leader(size_t index)
{
    if (index >= total_map_labels.items.size())
        return;

    const map_label_item& item = total_map_labels.items[index];
    if (!item.leader)
        return;

    setlinecolor(RGB(150, 163, 172));
    setlinestyle(PS_SOLID, 1);
    line(item.leader_start.x, item.leader_start.y,
        item.leader_end.x, item.leader_end.y);
}

void draw_all_station_names(const vertex vertices[], int vertex_number)
{
    prepare_total_map_labels(vertices, vertex_number);

    // 保持烘焙版完全相同的绘制层次：先全部引线，再按draw_order画全部标签底色和文字。
    for (size_t i = 0; i < total_map_labels.items.size(); ++i)
        draw_total_map_label_leader(i);
    for (size_t i = 0; i < total_map_labels.items.size(); ++i)
        draw_station_label(total_map_labels.items[i].rectangle, total_map_labels.items[i].text);
}

// 重点站和悬停站继续复用固定标签矩形，只增加原有边框，不改变标签坐标。
void emphasize_total_map_label(const vertex vertices[], int vertex_id, COLORREF color)
{
    string entity = is_jiading_internal_vertex(vertex_id)
        ? "嘉定校区" : get_entity_station_name(vertices, vertex_id);
    for (size_t i = 0; i < total_map_labels.items.size(); ++i) {
        const map_label_item& item = total_map_labels.items[i];
        if (item.entity_name != entity)
            continue;

        const station_label_rectangle& box = item.rectangle;
        setlinecolor(color);
        setlinestyle(PS_SOLID, 1);
        rectangle(box.left - 3, box.top - 2, box.right + 3, box.bottom + 2);
        break;
    }
}

/***************************************************************************
  函数名称：find_clicked_vertex
  功    能：根据总览图屏幕坐标寻找被点击的站点
  输入参数：顶点数组、顶点数量、鼠标坐标
  返 回 值：普通站返回顶点编号，点击嘉定校园枢纽返回-2，未命中返回-1
  说    明：总览使用缩放后的屏幕坐标，CSV坐标不再与窗口尺寸直接绑定
***************************************************************************/
int find_clicked_vertex(const vertex vertices[], int vertex_number,
    int mouse_x, int mouse_y)
{
    const int click_radius = 10;

    screen_point campus_point;
    campus_point.x = scale_total_map_x(ui_layout::campus_logical_x);
    campus_point.y = scale_total_map_y(ui_layout::campus_logical_y);
    int campus_dx = mouse_x - campus_point.x;
    int campus_dy = mouse_y - campus_point.y;
    if (campus_dx * campus_dx + campus_dy * campus_dy <= 13 * 13)
        return -2;

    int physical_limit = vertex_number;
    if (physical_limit > physical_vertex_number)
        physical_limit = physical_vertex_number;

    for (int i = 0; i < physical_limit; i++) {
        if (is_jiading_internal_vertex(i))
            continue;

        screen_point point = get_total_map_point(vertices, vertex_number, i);
        int dx = mouse_x - point.x;
        int dy = mouse_y - point.y;
        if (dx * dx + dy * dy <= click_radius * click_radius)
            return i;
    }

    return -1;
}

/***************************************************************************
  函数名称：find_jiading_map_node
  功    能：根据顶点编号寻找嘉定局部图节点
  输入参数：总图顶点数组、局部图节点数组、节点数量、顶点编号
  返 回 值：找到返回地址，否则返回nullptr
  说    明：公交虚拟节点通过名称和逻辑坐标映射到对应物理节点
***************************************************************************/
const jiading_map_node* find_jiading_map_node(
    const vertex vertices[], const jiading_map_node map_nodes[],
    int map_node_number, int vertex_id)
{
    if (vertex_id < 0 || vertex_id >= max_vertices)
        return nullptr;

    for (int i = 0; i < map_node_number; i++) {
        if (map_nodes[i].vertex_id == vertex_id)
            return &map_nodes[i];
    }

    if (vertex_id >= physical_vertex_number) {
        for (int i = 0; i < map_node_number; i++) {
            int physical_id = map_nodes[i].vertex_id;
            if (vertices[vertex_id].name == vertices[physical_id].name
                && vertices[vertex_id].x == vertices[physical_id].x
                && vertices[vertex_id].y == vertices[physical_id].y)
                return &map_nodes[i];
        }
    }

    return nullptr;
}

/***************************************************************************
  函数名称：get_jiading_screen_point
  功    能：把嘉定局部图坐标平移到新版大窗口中的居中位置
  输入参数：const jiading_map_node& node：局部图节点
  返 回 值：screen_point
  说    明：900乘700官方底图保持原像素，不做模糊拉伸
***************************************************************************/
screen_point get_jiading_screen_point(const jiading_map_node& node)
{
    screen_point point;
    point.x = node.x + ui_layout::jiading_offset_x;
    point.y = node.y + ui_layout::jiading_offset_y;
    return point;
}

/***************************************************************************
  函数名称：find_clicked_jiading_vertex
  功    能：寻找嘉定局部图中被点击的物理节点
  输入参数：局部图节点数组、节点数量、鼠标坐标
  返 回 值：命中返回物理顶点编号，否则返回-1
  说    明：点击判断使用平移后的新版屏幕坐标
***************************************************************************/
int find_clicked_jiading_vertex(const jiading_map_node map_nodes[],
    int map_node_number, int mouse_x, int mouse_y)
{
    const int click_radius = 10;

    for (int i = 0; i < map_node_number; i++) {
        screen_point point = get_jiading_screen_point(map_nodes[i]);
        int dx = mouse_x - point.x;
        int dy = mouse_y - point.y;
        if (dx * dx + dy * dy <= click_radius * click_radius)
            return map_nodes[i].vertex_id;
    }

    return -1;
}

/***************************************************************************
  函数名称：draw_transfer_star
  功    能：在指定位置绘制换乘红色星号
  输入参数：int x, int y：屏幕坐标
  返 回 值：无
  说    明：使用ASCII星号保证GB2312兼容
***************************************************************************/
void draw_transfer_star(int x, int y)
{
    setbkmode(TRANSPARENT);
    set_ui_font(23, FW_BOLD);
    settextcolor(RGB(220, 40, 40));
    outtextxy(x - 6, y - 18, "*");
}

/***************************************************************************
  函数名称：draw_selected_vertices
  功    能：在总览图上突出显示当前起点和终点
  输入参数：顶点数组、顶点数量和ui_state
  返 回 值：无
  说    明：嘉定校内节点在总览统一落到校园枢纽，进入局部图后显示精确位置
***************************************************************************/
void draw_selected_vertices(const vertex vertices[], int vertex_number,
    const ui_state& state)
{
    if (is_valid_vertex_id(state.start_vertex, vertex_number)) {
        screen_point point = get_total_map_point(vertices, vertex_number, state.start_vertex);
        setlinecolor(RGB(20, 110, 60));
        setfillcolor(RGB(47, 191, 113));
        fillcircle(point.x, point.y, 9);
    }

    if (is_valid_vertex_id(state.end_vertex, vertex_number)) {
        screen_point point = get_total_map_point(vertices, vertex_number, state.end_vertex);
        setlinecolor(RGB(140, 35, 35));
        setfillcolor(RGB(239, 90, 90));
        fillcircle(point.x, point.y, 9);
    }
}

/***************************************************************************
  函数名称：draw_global_transfer_marks
  功    能：在总览图中标出当前推荐路线的换乘位置
  输入参数：顶点数组、顶点数量和ui_state
  返 回 值：无
  说    明：不再长期绘制所有TRANSFER边，只显示最终路线真正发生的换乘
***************************************************************************/
void draw_global_transfer_marks(const vertex vertices[], int vertex_number,
    const ui_state& state)
{
    int previous_public_segment = -1;

    for (int i = 0; i < state.segment_number; i++) {
        if (!is_public_transport_segment(state.segments[i]))
            continue;

        if (previous_public_segment >= 0) {
            int first_vertex = state.segments[previous_public_segment].end_vertex;
            int second_vertex = state.segments[i].start_vertex;

            if (same_entity_station(vertices, first_vertex, second_vertex)) {
                screen_point point = get_total_map_point(vertices, vertex_number, first_vertex);
                draw_transfer_star(point.x, point.y);
            }
            else {
                screen_point first_point = get_total_map_point(vertices, vertex_number, first_vertex);
                screen_point second_point = get_total_map_point(vertices, vertex_number, second_vertex);
                draw_transfer_star(first_point.x, first_point.y);
                draw_transfer_star(second_point.x, second_point.y);
            }
        }

        previous_public_segment = i;
    }
}

/***************************************************************************
  函数名称：draw_station_name_box
  功    能：绘制起终点、换乘点或悬停站点的醒目白底名称框
  输入参数：const string& text：站名；int center_x, center_y：节点位置
  返 回 值：无
  说    明：只用于重点信息，不给所有普通站名加边框
***************************************************************************/
void draw_station_name_box(const string& text, int center_x, int center_y)
{
    setbkmode(TRANSPARENT);
    set_ui_font(15, FW_BOLD);

    int text_width_value = textwidth(text.c_str());
    int text_height_value = textheight(text.c_str());
    int left = center_x + 10;
    int top = center_y - text_height_value - 10;

    if (left + text_width_value + 10 > ui_layout::map_width - 6)
        left = center_x - text_width_value - 14;
    if (left < 4)
        left = 4;
    if (top < 4)
        top = center_y + 10;
    if (top + text_height_value + 8 > ui_layout::window_height - 4)
        top = ui_layout::window_height - text_height_value - 12;

    int right = left + text_width_value + 10;
    int bottom = top + text_height_value + 8;
    setfillcolor(RGB(255, 255, 255));
    solidrectangle(left, top, right, bottom);
    setlinecolor(RGB(126, 140, 154));
    rectangle(left, top, right, bottom);
    settextcolor(RGB(25, 35, 48));
    outtextxy(left + 5, top + 4, text.c_str());
}

/***************************************************************************
  函数名称：draw_global_route_labels
  功    能：在总览图中加强起点、终点和换乘站名称
  输入参数：顶点数组、顶点数量和ui_state
  返 回 值：无
  说    明：普通站名始终存在，本函数只负责路线重点的二次强调
***************************************************************************/
void draw_global_route_labels(const vertex vertices[], int vertex_number,
    const ui_state& state)
{
    bool marked[physical_vertex_number] = { false };

    if (state.start_vertex >= 0 && state.start_vertex < physical_vertex_number)
        marked[state.start_vertex] = true;
    if (state.end_vertex >= 0 && state.end_vertex < physical_vertex_number)
        marked[state.end_vertex] = true;

    if (state.route_ready) {
        int previous_public_segment = -1;
        for (int i = 0; i < state.segment_number; i++) {
            if (!is_public_transport_segment(state.segments[i]))
                continue;
            if (previous_public_segment >= 0) {
                int first_vertex = state.segments[previous_public_segment].end_vertex;
                int second_vertex = state.segments[i].start_vertex;
                if (first_vertex >= 0 && first_vertex < physical_vertex_number)
                    marked[first_vertex] = true;
                if (second_vertex >= 0 && second_vertex < physical_vertex_number)
                    marked[second_vertex] = true;
            }
            previous_public_segment = i;
        }
    }

    string drawn_names[physical_vertex_number];
    int drawn_number = 0;

    for (int i = 0; i < physical_vertex_number; i++) {
        if (!marked[i])
            continue;

        string entity_name = get_entity_station_name(vertices, i);
        bool already_drawn = false;
        for (int j = 0; j < drawn_number; j++) {
            if (drawn_names[j] == entity_name) {
                already_drawn = true;
                break;
            }
        }
        if (already_drawn)
            continue;

        drawn_names[drawn_number++] = entity_name;
        emphasize_total_map_label(vertices, i, RGB(90, 113, 135));
    }
}

/***************************************************************************
  函数名称：draw_jiading_route_highlight
  功    能：在嘉定局部图中绘制推荐路线
  输入参数：总图顶点、局部节点、节点数量和ui_state
  返 回 值：无
  说    明：使用官方底图原始坐标加统一偏移，不拉伸图片
***************************************************************************/
void draw_jiading_route_highlight(const vertex vertices[],
    const jiading_map_node map_nodes[], int map_node_number,
    const ui_state& state)
{
    if (!state.route_ready || state.path_vertex_number < 2)
        return;

    setlinecolor(RGB(255, 201, 35));
    setlinestyle(PS_SOLID, 7);

    for (int i = state.path_vertex_number - 1; i > 0; i--) {
        const jiading_map_node* from_node = find_jiading_map_node(
            vertices, map_nodes, map_node_number, state.path[i]);
        const jiading_map_node* to_node = find_jiading_map_node(
            vertices, map_nodes, map_node_number, state.path[i - 1]);
        if (!from_node || !to_node)
            continue;

        screen_point from_point = get_jiading_screen_point(*from_node);
        screen_point to_point = get_jiading_screen_point(*to_node);
        if (from_point.x == to_point.x && from_point.y == to_point.y)
            continue;

        line(from_point.x, from_point.y, to_point.x, to_point.y);
    }

    setlinestyle(PS_SOLID, 1);
}

/***************************************************************************
  函数名称：draw_jiading_transfer_marks
  功    能：在嘉定局部图中绘制当前路线换乘标记
  输入参数：总图顶点、局部节点、节点数量和ui_state
  返 回 值：无
  说    明：只显示实际路线产生的换乘
***************************************************************************/
void draw_jiading_transfer_marks(const vertex vertices[],
    const jiading_map_node map_nodes[], int map_node_number,
    const ui_state& state)
{
    bool marked[physical_vertex_number] = { false };
    int previous_public_segment = -1;

    for (int i = 0; i < state.segment_number; i++) {
        if (!is_public_transport_segment(state.segments[i]))
            continue;
        if (previous_public_segment >= 0) {
            int first_vertex = state.segments[previous_public_segment].end_vertex;
            int second_vertex = state.segments[i].start_vertex;
            if (first_vertex >= 0 && first_vertex < physical_vertex_number)
                marked[first_vertex] = true;
            if (second_vertex >= 0 && second_vertex < physical_vertex_number)
                marked[second_vertex] = true;
        }
        previous_public_segment = i;
    }

    for (int i = 0; i < physical_vertex_number; i++) {
        if (!marked[i])
            continue;
        const jiading_map_node* node = find_jiading_map_node(
            vertices, map_nodes, map_node_number, i);
        if (!node)
            continue;
        screen_point point = get_jiading_screen_point(*node);
        draw_transfer_star(point.x, point.y);
    }
}

/***************************************************************************
  函数名称：draw_jiading_campus_map
  功    能：在新版大窗口中绘制嘉定校区官方局部图
  输入参数：总图顶点、局部节点、节点数量、背景图和ui_state
  返 回 值：无
  说    明：900乘700官方图保持原像素居中放置，右侧控制区始终不变
***************************************************************************/
void draw_jiading_campus_map(const vertex vertices[],
    const jiading_map_node map_nodes[], int map_node_number,
    const IMAGE& background, const ui_state& state)
{
    putimage(ui_layout::jiading_offset_x, ui_layout::jiading_offset_y, &background);

    if (state.route_ready)
        draw_jiading_route_highlight(vertices, map_nodes, map_node_number, state);

    setfillcolor(RGB(255, 255, 255));
    setlinecolor(RGB(220, 122, 36));
    for (int i = 0; i < map_node_number; i++) {
        screen_point point = get_jiading_screen_point(map_nodes[i]);
        fillcircle(point.x, point.y, 6);
    }

    if (state.route_ready)
        draw_jiading_transfer_marks(vertices, map_nodes, map_node_number, state);

    set_ui_font(15);
    setbkmode(TRANSPARENT);
    for (int i = 0; i < map_node_number; i++) {
        int left = map_nodes[i].label_x + ui_layout::jiading_offset_x;
        int top = map_nodes[i].label_y + ui_layout::jiading_offset_y;
        const string& label_text = map_nodes[i].short_name;
        int width = textwidth(label_text.c_str());
        int height = textheight(label_text.c_str());

        setfillcolor(RGB(255, 255, 255));
        solidrectangle(left, top, left + width + 8, top + height + 6);
        setlinecolor(RGB(155, 165, 175));
        rectangle(left, top, left + width + 8, top + height + 6);
        settextcolor(RGB(35, 45, 60));
        outtextxy(left + 4, top + 3, label_text.c_str());
    }

    const jiading_map_node* start_node = find_jiading_map_node(
        vertices, map_nodes, map_node_number, state.start_vertex);
    if (start_node) {
        screen_point point = get_jiading_screen_point(*start_node);
        setlinecolor(RGB(20, 110, 60));
        setfillcolor(RGB(47, 191, 113));
        fillcircle(point.x, point.y, 9);
    }

    const jiading_map_node* end_node = find_jiading_map_node(
        vertices, map_nodes, map_node_number, state.end_vertex);
    if (end_node) {
        screen_point point = get_jiading_screen_point(*end_node);
        setlinecolor(RGB(140, 35, 35));
        setfillcolor(RGB(239, 90, 90));
        fillcircle(point.x, point.y, 9);
    }
}

/***************************************************************************
  函数名称：draw_hovered_station
  功    能：绘制鼠标悬停站点的完整名称
  输入参数：总图顶点、顶点数量、嘉定局部节点、节点数量和ui_state
  返 回 值：无
  说    明：总览和嘉定局部图分别使用自己的屏幕坐标
***************************************************************************/
void draw_hovered_station(const vertex vertices[], int vertex_number,
    const jiading_map_node map_nodes[], int map_node_number,
    const ui_state& state)
{
    if (state.hovered_vertex < 0 || state.hovered_vertex >= physical_vertex_number)
        return;

    if (state.is_jiading_campus) {
        const jiading_map_node* node = find_jiading_map_node(
            vertices, map_nodes, map_node_number, state.hovered_vertex);
        if (!node)
            return;
        screen_point point = get_jiading_screen_point(*node);
        draw_station_name_box(vertices[state.hovered_vertex].name, point.x, point.y);
    }
    else {
        emphasize_total_map_label(vertices, state.hovered_vertex, RGB(30, 105, 170));
    }
}

/***************************************************************************
  函数名称：is_point_in_rectangle
  功    能：判断指定点是否位于矩形区域
  输入参数：点坐标和矩形四条边
  返 回 值：位于矩形内返回true
  说    明：用于右侧控件统一命中判断
***************************************************************************/
bool is_point_in_rectangle(int point_x, int point_y,
    int left, int top, int right, int bottom)
{
    return point_x >= left && point_x <= right
        && point_y >= top && point_y <= bottom;
}

/***************************************************************************
  函数名称：draw_button
  功    能：绘制统一风格的右侧按钮
  输入参数：矩形位置、文字、是否选中、是否主按钮
  返 回 值：无
  说    明：统一按钮外观，减少旧EasyX代码重复设置颜色和字体
***************************************************************************/
void draw_button(int left, int top, int right, int bottom,
    const string& text, bool selected = false, bool primary = false)
{
    if (primary)
        setfillcolor(RGB(42, 117, 177));
    else if (selected)
        setfillcolor(RGB(214, 234, 249));
    else
        setfillcolor(RGB(242, 246, 250));

    solidrectangle(left, top, right, bottom);
    setlinecolor(RGB(143, 158, 174));
    rectangle(left, top, right, bottom);

    set_ui_font(15, selected || primary ? FW_BOLD : FW_NORMAL);
    settextcolor(primary ? RGB(255, 255, 255) : RGB(35, 45, 60));
    int width = textwidth(text.c_str());
    int height = textheight(text.c_str());
    outtextxy((left + right - width) / 2, (top + bottom - height) / 2,
        text.c_str());
}

/***************************************************************************
  函数名称：draw_checkbox
  功    能：绘制右侧复选框
  输入参数：int x, y：左上角；bool checked：是否勾选
  返 回 值：无
  说    明：只保留允许骑行一个复选框，站名在总览中始终完整展示
***************************************************************************/
void draw_checkbox(int x, int y, bool checked)
{
    setfillcolor(RGB(255, 255, 255));
    solidrectangle(x, y, x + 20, y + 20);
    setlinecolor(RGB(132, 148, 164));
    rectangle(x, y, x + 20, y + 20);

    if (checked) {
        setfillcolor(RGB(47, 191, 113));
        solidrectangle(x + 4, y + 4, x + 16, y + 16);
    }
}

/***************************************************************************
  函数名称：draw_control_panel
  功    能：绘制右侧控制区域
  输入参数：顶点数组、顶点数量和ui_state
  返 回 值：无
  说    明：所有控件使用统一布局常量，放大窗口后仍适合一镜到底录屏
***************************************************************************/
void draw_control_panel(const vertex vertices[], int vertex_number,
    const ui_state& state)
{
    setfillcolor(RGB(250, 252, 255));
    solidrectangle(ui_layout::panel_left, 0,
        ui_layout::panel_right, ui_layout::window_height);
    setlinecolor(RGB(205, 214, 224));
    line(ui_layout::panel_left, 0, ui_layout::panel_left, ui_layout::window_height);

    const int left = ui_layout::panel_content_left;
    const int right = ui_layout::panel_content_right;

    set_ui_font(21, FW_BOLD);
    settextcolor(RGB(24, 52, 76));
    outtextxy(left, 22, "城市多模态交通导航");

    set_ui_font(16, FW_BOLD);
    settextcolor(RGB(35, 45, 60));
    outtextxy(left, 67, "起点:");
    string start_text = "未选择";
    if (is_valid_vertex_id(state.start_vertex, vertex_number))
        start_text = get_entity_station_name(vertices, state.start_vertex);
    station_label_text start_label = build_station_label_text(start_text, 255);
    outtextxy(left + 58, 67, start_label.first_line.c_str());
    if (start_label.line_number == 2)
        outtextxy(left + 58, 83, start_label.second_line.c_str());

    outtextxy(left, 96, "终点:");
    string end_text = "未选择";
    if (is_valid_vertex_id(state.end_vertex, vertex_number))
        end_text = get_entity_station_name(vertices, state.end_vertex);
    station_label_text end_label = build_station_label_text(end_text, 255);
    outtextxy(left + 58, 96, end_label.first_line.c_str());
    if (end_label.line_number == 2)
        outtextxy(left + 58, 112, end_label.second_line.c_str());

    draw_button(left, 128, right, 158,
        state.is_jiading_campus ? "返回全市总览" : "嘉定校区局部图");

    set_ui_font(16, FW_BOLD);
    outtextxy(left, 185, "出发时间");
    draw_button(left, 214, left + 64, 252, "-5");

    set_ui_font(18, FW_BOLD);
    stringstream time_stream;
    time_stream << setw(2) << setfill('0') << state.start_hour
        << ":" << setw(2) << state.start_minute;
    string time_text = time_stream.str();
    int time_text_width = textwidth(time_text.c_str());
    int time_text_x = (left + right - time_text_width) / 2;
    outtextxy(time_text_x, 222, time_text.c_str());
    draw_button(right - 64, 214, right, 252, "+5");

    set_ui_font(16, FW_BOLD);
    outtextxy(left, 282, "路线策略");
    draw_button(left, 312, left + 158, 352,
        "时间最短", state.k == 0);
    draw_button(right - 158, 312, right, 352,
        "经济优先", state.k == 8);

    set_ui_font(16, FW_BOLD);
    outtextxy(left, 382, "允许骑行");
    draw_checkbox(left + 105, 380, state.allow_bike);

    set_ui_font(13);
    settextcolor(RGB(90, 105, 120));

    draw_button(left, 444, left + 158, 486,
        "开始规划", false, true);
    draw_button(right - 158, 444, right, 486, "重置");

    set_ui_font(14);
    settextcolor(RGB(55, 70, 84));
    string status_text = "状态: " + state.message;
    station_label_text status_label = build_station_label_text(status_text, 326);
    outtextxy(left, 505, status_label.first_line.c_str());
    if (status_label.line_number == 2)
        outtextxy(left, 520, status_label.second_line.c_str());
}

/***************************************************************************
  函数名称：truncate_gb2312_text_to_width
  功    能：把GB2312文本压缩到指定像素宽度并保留省略号
  输入参数：const string& text：原文本；int max_width：最大像素宽度
  返 回 值：不超过指定宽度的文本
  说    明：只用于右侧紧凑路线阶段，地图站名不会截断
***************************************************************************/
string truncate_gb2312_text_to_width(const string& text, int max_width)
{
    if (textwidth(text.c_str()) <= max_width)
        return text;

    string result;
    size_t position = 0;
    while (position < text.size()) {
        unsigned char first_byte = static_cast<unsigned char>(text[position]);
        size_t char_length =
            (first_byte >= 0xA1 && position + 1 < text.size()) ? 2 : 1;
        string candidate = result + text.substr(position, char_length) + "...";
        if (textwidth(candidate.c_str()) > max_width)
            break;
        result += text.substr(position, char_length);
        position += char_length;
    }

    return result + "...";
}

/***************************************************************************
  函数名称：build_route_endpoint_text
  功    能：生成起点到终点的紧凑路线端点文本
  输入参数：起点名称、终点名称、最大像素宽度
  返 回 值：紧凑文本
  说    明：两端平均分配宽度，避免长站名把另一端完全挤掉
***************************************************************************/
string build_route_endpoint_text(const string& start_name,
    const string& end_name, int max_width)
{
    string arrow = " -> ";
    int arrow_width = textwidth(arrow.c_str());
    int one_side_width = (max_width - arrow_width) / 2;

    return truncate_gb2312_text_to_width(start_name, one_side_width)
        + arrow
        + truncate_gb2312_text_to_width(end_name, one_side_width);
}

/***************************************************************************
  函数名称：draw_route_result
  功    能：在右侧下半区绘制路线统计和分页导航
  输入参数：顶点数组、线路数组、线路数量和ui_state
  返 回 值：无
  说    明：一页显示四个路线阶段，右栏始终与总图同时存在，适合一镜到底录屏
***************************************************************************/
void draw_route_result(const vertex vertices[], const transit_line lines[],
    int line_number, const ui_state& state)
{
    if (!state.route_ready)
        return;

    const int left = ui_layout::panel_content_left;
    const int right = ui_layout::panel_content_right;
    const int text_width_limit = right - left;

    setfillcolor(RGB(245, 248, 252));
    solidrectangle(left - 8, 532, right + 2, 792);

    set_ui_font(15, FW_BOLD);
    settextcolor(RGB(35, 45, 60));
    stringstream result_stream;
    result_stream << "时间:" << state.total_time_cost << "分  "
        << fixed << setprecision(2)
        << "费用:" << state.total_fare_cost << "元";
    outtextxy(left, 540, result_stream.str().c_str());

    result_stream.str("");
    result_stream.clear();
    result_stream << "到达:"
        << setw(2) << setfill('0') << state.arrival_hour
        << ":" << setw(2) << state.arrival_minute;
    if (state.days_passed > 0)
        result_stream << "+" << state.days_passed << "天";
    result_stream << "  经停:" << state.stop_number << "站";
    outtextxy(left, 561, result_stream.str().c_str());

    int transfer_count = get_transfer_count(state);
    result_stream.str("");
    result_stream.clear();
    result_stream << "换乘:" << transfer_count << "次";
    if (transfer_count > 0)
        result_stream << "  " << get_transfer_summary(vertices, state);
    set_ui_font(14);
    string transfer_text = truncate_gb2312_text_to_width(
        result_stream.str(), text_width_limit);
    outtextxy(left, 582, transfer_text.c_str());

    set_ui_font(15, FW_BOLD);
    outtextxy(left, 606, "详细路线");

    const int segments_per_page = 4;
    int total_pages = (state.segment_number + segments_per_page - 1)
        / segments_per_page;
    if (total_pages < 1)
        total_pages = 1;

    int first_segment = state.guide_page * segments_per_page;
    int last_segment = first_segment + segments_per_page;
    if (last_segment > state.segment_number)
        last_segment = state.segment_number;

    int y = 628;
    for (int i = first_segment; i < last_segment; i++) {
        const route_segment& segment = state.segments[i];
        result_stream.str("");
        result_stream.clear();
        result_stream << (i + 1) << ". ";

        if (segment.type == edge_type::TRANSFER)
            result_stream << (segment.use_bike ? "骑行" : "步行");
        else if (segment.line_id >= 0 && segment.line_id < line_number)
            result_stream << "乘" << lines[segment.line_id].name;
        else
            result_stream << "公共交通";

        result_stream << " " << segment.time_cost << "分";
        if (is_public_transport_segment(segment)) {
            int intermediate_stop_number = segment.station_edge_number - 1;
            if (intermediate_stop_number > 0)
                result_stream << " 经" << intermediate_stop_number << "站";
        }

        set_ui_font(14);
        string first_line = truncate_gb2312_text_to_width(
            result_stream.str(), text_width_limit);
        outtextxy(left, y, first_line.c_str());

        set_ui_font(13);
        string endpoints = build_route_endpoint_text(
            get_entity_station_name(vertices, segment.start_vertex),
            get_entity_station_name(vertices, segment.end_vertex),
            text_width_limit - 10);
        outtextxy(left + 8, y + 15, endpoints.c_str());
        y += 32;
    }

    set_ui_font(13);
    stringstream page_stream;
    page_stream << (state.guide_page + 1) << " / " << total_pages;
    string page_text = page_stream.str();

    if (state.guide_page > 0)
        draw_button(left, 764, left + 42, 790, "<");
    if (state.guide_page + 1 < total_pages)
        draw_button(right - 42, 764, right, 790, ">");

    settextcolor(RGB(70, 82, 96));
    outtextxy((left + right - textwidth(page_text.c_str())) / 2,
        770, page_text.c_str());
}

/***************************************************************************
  函数名称：draw_easyx_interface
  功    能：按新版分层结构完整刷新EasyX界面
  输入参数：顶点、线路、嘉定局部图、背景图和ui_state
  返 回 值：无
  说    明：运营线、推荐路线、站点、全部站名和重点标记分层绘制，明确区分算法边与展示边
***************************************************************************/
void draw_easyx_interface(const vertex vertices[], int vertex_number,
    const transit_line lines[], int line_number,
    const jiading_map_node map_nodes[], int map_node_number,
    const IMAGE& jiading_background, const ui_state& state)
{
    setbkcolor(RGB(247, 250, 252));
    cleardevice();

    if (state.is_jiading_campus) {
        draw_jiading_campus_map(vertices, map_nodes,
            map_node_number, jiading_background, state);
    }
    else {
        draw_base_network(vertices, vertex_number);
        if (state.route_ready)
            draw_route_highlight(vertices, vertex_number, state);

        draw_total_map_vertices(vertices, vertex_number);
        draw_all_station_names(vertices, vertex_number);
        if (state.route_ready)
            draw_global_transfer_marks(vertices, vertex_number, state);

        draw_selected_vertices(vertices, vertex_number, state);
        draw_global_route_labels(vertices, vertex_number, state);
    }

    draw_hovered_station(vertices, vertex_number,
        map_nodes, map_node_number, state);
    draw_control_panel(vertices, vertex_number, state);

    if (state.route_ready)
        draw_route_result(vertices, lines, line_number, state);

    FlushBatchDraw();
}

/***************************************************************************
  函数名称：invalidate_route_result
  功    能：让当前规划结果及其派生显示状态全部失效
  输入参数：ui_state& state：需要修改的界面状态
  返 回 值：无
  说    明：修改起终点、时间、策略或骑行选项后统一调用，避免残留旧路线和旧页码
***************************************************************************/
void invalidate_route_result(ui_state& state)
{
    state.route_ready = false;
    state.path_vertex_number = 0;
    state.segment_number = 0;
    state.guide_page = 0;
    state.stop_number = 0;
    state.total_time_cost = 0;
    state.total_fare_cost = 0;
    state.arrival_hour = 0;
    state.arrival_minute = 0;
    state.days_passed = 0;
}

/***************************************************************************
  函数名称：reset_ui_state
  功    能：把EasyX界面状态恢复到程序启动时的默认状态
  输入参数：ui_state& state：需要重置的界面状态
  返 回 值：无
  说    明：数组旧数据无需逐项清零，只要对应数量归零且route_ready为false即可视为无效
***************************************************************************/
void reset_ui_state(ui_state& state)
{
    state.start_vertex = -1;
    state.end_vertex = -1;

    state.k = 0;
    state.allow_bike = false;
    state.show_all_names = true;
    state.is_jiading_campus = false;

    state.start_hour = 8;
    state.start_minute = 30;
    state.hovered_vertex = -1;

    invalidate_route_result(state);

    state.message = "请选择起点";
}

/***************************************************************************
  函数名称：calculate_route_for_ui
  功    能：根据当前ui_state中的起终点和策略调用已有算法完成一次路线规划
  输入参数：vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  ui_state& state：当前EasyX界面状态，规划结果写回其中
  返 回 值：路线规划成功返回true，失败返回false
  说    明：Dijkstra和原有统计保持不变；build_paths之后增加route_segment解释层供EasyX使用
***************************************************************************/
bool calculate_route_for_ui(vertex vertices[], int vertex_number,
    ui_state& state)
{
    invalidate_route_result(state);

    if (!is_valid_vertex_id(state.start_vertex, vertex_number)
        || !is_valid_vertex_id(state.end_vertex, vertex_number)) {

        state.message = "请先选择起点和终点";
        return false;
    }

    if (state.start_vertex == state.end_vertex) {
        state.message = "起点和终点不能相同";
        return false;
    }

    if (!dijkstra(vertices, vertex_number,
        state.start_vertex, state.k,
        state.distance, state.previous_vertex,
        state.previous_edge,
        state.allow_bike)) {
        state.message = "Dijkstra计算失败";
        return false;
    }

    if (!build_paths(state.previous_vertex, vertex_number,
        state.start_vertex, state.end_vertex,
        state.path, state.path_vertex_number)) {

        state.message = "当前起终点之间没有可用路线";
        return false;
    }

    if (!calculate_path_statistics(
        state.path, state.path_vertex_number,
        state.previous_edge,
        state.total_time_cost,
        state.total_fare_cost,
        state.allow_bike)) {

        state.message = "路线统计失败";
        return false;
    }

    if (!build_route_segments(
        vertices, vertex_number,
        state.path, state.path_vertex_number,
        state.previous_edge, state.allow_bike,
        state.segments, state.segment_number,
        state.stop_number)) {

        state.message = "路线阶段生成失败";
        return false;
    }

    if (!calculate_arrival_time(
        state.start_hour,
        state.start_minute,
        state.total_time_cost,
        state.arrival_hour,
        state.arrival_minute,
        state.days_passed)) {

        state.message = "到达时间计算失败";
        return false;
    }

    state.guide_page = 0;
    state.route_ready = true;
    state.message = "路线规划完成";

    return true;
}

/***************************************************************************
  函数名称：handle_left_click
  功    能：处理新版EasyX界面的鼠标左键点击
  输入参数：鼠标坐标、顶点数组、嘉定局部节点和ui_state
  返 回 值：无
  说    明：全部控件坐标统一引用ui_layout，点击总览嘉定校园枢纽会进入同窗口局部图
***************************************************************************/
void handle_left_click(int mouse_x, int mouse_y,
    vertex vertices[], int vertex_number,
    const jiading_map_node map_nodes[], int map_node_number,
    ui_state& state)
{
    if (mouse_x < ui_layout::map_width) {
        int clicked_vertex = -1;
        if (state.is_jiading_campus)
            clicked_vertex = find_clicked_jiading_vertex(
                map_nodes, map_node_number, mouse_x, mouse_y);
        else
            clicked_vertex = find_clicked_vertex(
                vertices, vertex_number, mouse_x, mouse_y);

        if (clicked_vertex == -2) {
            state.is_jiading_campus = true;
            state.hovered_vertex = -1;
            state.message = "请在嘉定校区局部图选择具体站点";
            return;
        }

        if (clicked_vertex == -1)
            return;

        if (state.start_vertex == -1) {
            state.start_vertex = clicked_vertex;
            state.end_vertex = -1;
            invalidate_route_result(state);
            state.message = "请选择终点";
        }
        else {
            if (clicked_vertex == state.start_vertex) {
                state.message = "终点不能与起点相同";
                return;
            }
            state.end_vertex = clicked_vertex;
            invalidate_route_result(state);
            state.message = "起终点已选择，请开始规划";
        }
        return;
    }

    const int left = ui_layout::panel_content_left;
    const int right = ui_layout::panel_content_right;

    if (is_point_in_rectangle(mouse_x, mouse_y, left, 128, right, 158)) {
        state.is_jiading_campus = !state.is_jiading_campus;
        state.hovered_vertex = -1;
        return;
    }

    if (state.route_ready && state.segment_number > 0) {
        const int segments_per_page = 4;
        int total_pages = (state.segment_number + segments_per_page - 1)
            / segments_per_page;

        if (is_point_in_rectangle(mouse_x, mouse_y,
            left, 764, left + 42, 790)) {
            if (state.guide_page > 0)
                state.guide_page--;
            return;
        }

        if (is_point_in_rectangle(mouse_x, mouse_y,
            right - 42, 764, right, 790)) {
            if (state.guide_page + 1 < total_pages)
                state.guide_page++;
            return;
        }
    }

    if (is_point_in_rectangle(mouse_x, mouse_y,
        left, 214, left + 64, 252)) {
        int total_minutes = state.start_hour * 60 + state.start_minute - 5;
        if (total_minutes < 0)
            total_minutes += 24 * 60;
        state.start_hour = total_minutes / 60;
        state.start_minute = total_minutes % 60;
        invalidate_route_result(state);
        state.message = "出发时间已修改，请重新规划";
    }
    else if (is_point_in_rectangle(mouse_x, mouse_y,
        right - 64, 214, right, 252)) {
        int total_minutes = state.start_hour * 60 + state.start_minute;
        total_minutes = (total_minutes + 5) % (24 * 60);
        state.start_hour = total_minutes / 60;
        state.start_minute = total_minutes % 60;
        invalidate_route_result(state);
        state.message = "出发时间已修改，请重新规划";
    }
    else if (is_point_in_rectangle(mouse_x, mouse_y,
        left, 312, left + 158, 352)) {
        state.k = 0;
        invalidate_route_result(state);
        state.message = "已选择时间最短策略";
    }
    else if (is_point_in_rectangle(mouse_x, mouse_y,
        right - 158, 312, right, 352)) {
        state.k = 8;
        invalidate_route_result(state);
        state.message = "已选择经济优先策略";
    }
    else if (is_point_in_rectangle(mouse_x, mouse_y,
        left + 105, 380, left + 125, 400)) {
        state.allow_bike = !state.allow_bike;
        invalidate_route_result(state);
        state.message = state.allow_bike
            ? "已允许骑行接驳" : "已关闭骑行接驳";
    }
    else if (is_point_in_rectangle(mouse_x, mouse_y,
        left, 444, left + 158, 486)) {
        calculate_route_for_ui(vertices, vertex_number, state);
    }
    else if (is_point_in_rectangle(mouse_x, mouse_y,
        right - 158, 444, right, 486)) {
        reset_ui_state(state);
    }
}

/***************************************************************************
  函数名称：handle_mouse_move
  功    能：更新新版界面当前悬停站点
  输入参数：鼠标坐标、顶点数组、嘉定局部节点和ui_state
  返 回 值：悬停对象变化返回true，否则返回false
  说    明：只在鼠标进入左侧地图区域时做站点命中判断
***************************************************************************/
bool handle_mouse_move(int mouse_x, int mouse_y,
    const vertex vertices[], int vertex_number,
    const jiading_map_node map_nodes[], int map_node_number,
    ui_state& state)
{
    int hovered_vertex = -1;

    if (mouse_x < ui_layout::map_width) {
        if (state.is_jiading_campus)
            hovered_vertex = find_clicked_jiading_vertex(
                map_nodes, map_node_number, mouse_x, mouse_y);
        else {
            hovered_vertex = find_clicked_vertex(
                vertices, vertex_number, mouse_x, mouse_y);
            if (hovered_vertex == -2)
                hovered_vertex = -1;
        }
    }

    if (hovered_vertex == state.hovered_vertex)
        return false;

    state.hovered_vertex = hovered_vertex;
    return true;
}

/***************************************************************************
  函数名称：run_easyx_interface
  功    能：运行新版EasyX主界面消息循环
  输入参数：顶点、线路、嘉定局部节点、背景图和ui_state
  返 回 值：无
  说    明：整个演示始终在同一个1500乘800窗口完成，不需要切换应用或重新摆窗口
***************************************************************************/
void run_easyx_interface(vertex vertices[], int vertex_number,
    const transit_line lines[], int line_number,
    const jiading_map_node map_nodes[], int map_node_number,
    const IMAGE& jiading_background, ui_state& state)
{
    BeginBatchDraw();
    draw_easyx_interface(vertices, vertex_number,
        lines, line_number, map_nodes, map_node_number,
        jiading_background, state);

    while (true) {
        ExMessage message = getmessage(EX_MOUSE | EX_KEY | EX_WINDOW);

        if (message.message == WM_CLOSE)
            break;
        if (message.message == WM_KEYDOWN && message.vkcode == VK_ESCAPE)
            break;

        if (message.message == WM_MOUSEMOVE) {
            if (handle_mouse_move(message.x, message.y,
                vertices, vertex_number, map_nodes, map_node_number, state)) {
                draw_easyx_interface(vertices, vertex_number,
                    lines, line_number, map_nodes, map_node_number,
                    jiading_background, state);
            }
            continue;
        }

        if (message.message == WM_LBUTTONDOWN) {
            handle_left_click(message.x, message.y,
                vertices, vertex_number, map_nodes, map_node_number, state);
            draw_easyx_interface(vertices, vertex_number,
                lines, line_number, map_nodes, map_node_number,
                jiading_background, state);
        }
    }

    EndBatchDraw();
}

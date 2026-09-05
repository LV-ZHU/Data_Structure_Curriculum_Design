from pathlib import Path

path = Path("main.cpp")
text = path.read_bytes().decode("gb2312")
text = text.replace("\r\n", "\n").replace("\r", "\n")

COMMENT = "/***************************************************************************"

def block_start(signature: str) -> int:
    position = text.index(signature)
    comment_position = text.rfind(COMMENT, 0, position)
    if comment_position < 0:
        raise RuntimeError("找不到函数注释块：" + signature)
    return comment_position

ui_start = block_start("bool enable_high_dpi_rendering()")
ui_end = block_start("void invalidate_route_result(")

ui_code = r'''/***************************************************************************
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

// EasyX显示布局集中管理，避免界面代码到处散落硬编码坐标
namespace ui_layout
{
    const int window_width = 1500;
    const int window_height = 800;
    const int map_width = 1120;
    const int panel_left = map_width;
    const int panel_right = window_width;

    const int network_left = 20;
    const int network_right = 1100;
    const int network_top = 20;
    const int network_bottom = 625;

    const int logical_left = 40;
    const int logical_right = 890;
    const int logical_top = 60;
    const int logical_bottom = 610;

    const int campus_list_left = 18;
    const int campus_list_top = 642;
    const int campus_list_right = 1102;
    const int campus_list_bottom = 787;

    const int jiading_offset_x = 110;
    const int jiading_offset_y = 50;

    const int panel_content_left = 1142;
    const int panel_content_right = 1478;
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
        28, 8, 0, 0,
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
  返 回 值：56到63号校内节点返回true，否则返回false
  说    明：总览将校内八个节点收束为校园枢纽，完整名称在底部清单同时展示
***************************************************************************/
bool is_jiading_internal_vertex(int vertex_id)
{
    return vertex_id >= 56 && vertex_id <= 63;
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
        logical_x = 150;
        logical_y = 235;
    }

    screen_point point;
    point.x = scale_total_map_x(logical_x);
    point.y = scale_total_map_y(logical_y);
    return point;
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
            setlinestyle(PS_DASH, 1);
        }
        else {
            setlinecolor(RGB(63, 143, 181));
            setlinestyle(PS_SOLID, 2);
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
    setlinestyle(PS_SOLID, 4);
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
                screen_point from_point = get_total_map_point(vertices, vertex_number, i);
                screen_point to_point = get_total_map_point(
                    vertices, vertex_number, current_edge->to);

                if (from_point.x != to_point.x || from_point.y != to_point.y) {
                    set_network_edge_style(*current_edge);
                    line(from_point.x, from_point.y, to_point.x, to_point.y);
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
            setlinestyle(PS_DASH, 3);
        else
            setlinestyle(PS_SOLID, 7);

        line(from_point.x, from_point.y, to_point.x, to_point.y);
    }

    setlinestyle(PS_SOLID, 1);
}

/***************************************************************************
  函数名称：draw_total_map_vertices
  功    能：绘制总览图中的实体站点圆圈
  输入参数：const vertex vertices[]：顶点数组；int vertex_number：顶点数量
  返 回 值：无
  说    明：同实体换乘站只画一个圆圈，嘉定校内八个节点收束成一个校园枢纽
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

        if (already_drawn)
            continue;

        drawn_names[drawn_name_number++] = entity_name;
        screen_point point = get_total_map_point(vertices, vertex_number, i);

        setfillcolor(RGB(255, 255, 255));
        if (vertices[i].type == station_type::METRO)
            setlinecolor(RGB(47, 117, 196));
        else
            setlinecolor(RGB(216, 119, 39));

        fillcircle(point.x, point.y, 5);
    }

    screen_point campus_point;
    campus_point.x = scale_total_map_x(150);
    campus_point.y = scale_total_map_y(235);
    setfillcolor(RGB(255, 255, 255));
    setlinecolor(RGB(216, 119, 39));
    fillcircle(campus_point.x, campus_point.y, 7);
}

/***************************************************************************
  函数名称：build_station_label_text
  功    能：把较长站名压缩为最多两行显示
  输入参数：const string& text：完整站名；int max_width：单行最大像素宽度
  返 回 值：station_label_text
  说    明：按GB2312双字节边界拆分，不截断汉字，完整名称始终保留
***************************************************************************/
station_label_text build_station_label_text(const string& text, int max_width)
{
    station_label_text result;

    if (textwidth(text.c_str()) <= max_width) {
        result.first_line = text;
        result.width = textwidth(text.c_str());
        result.height = 15;
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
            (first_byte >= 0xA1 && position + 1 < text.size()) ? 2 : 1;
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
    result.height = 30;
    result.line_number = 2;
    return result;
}

/***************************************************************************
  函数名称：get_label_overlap_area
  功    能：计算两个站名矩形留出安全间距后的重叠面积
  输入参数：两个station_label_rectangle
  返 回 值：重叠面积
  说    明：标签之间预留2像素空隙，避免文字视觉上粘成一团
***************************************************************************/
int get_label_overlap_area(const station_label_rectangle& first,
    const station_label_rectangle& second)
{
    const int safe_gap = 2;
    int overlap_left = (first.left - safe_gap) > (second.left - safe_gap)
        ? (first.left - safe_gap) : (second.left - safe_gap);
    int overlap_top = (first.top - safe_gap) > (second.top - safe_gap)
        ? (first.top - safe_gap) : (second.top - safe_gap);
    int overlap_right = (first.right + safe_gap) < (second.right + safe_gap)
        ? (first.right + safe_gap) : (second.right + safe_gap);
    int overlap_bottom = (first.bottom + safe_gap) < (second.bottom + safe_gap)
        ? (first.bottom + safe_gap) : (second.bottom + safe_gap);

    if (overlap_left >= overlap_right || overlap_top >= overlap_bottom)
        return 0;

    return (overlap_right - overlap_left) * (overlap_bottom - overlap_top);
}

/***************************************************************************
  函数名称：build_station_label_candidate
  功    能：根据站点、方向和距离生成标签候选矩形
  输入参数：站点坐标、标签宽高、方向编号和距离
  返 回 值：候选矩形
  说    明：只在站点附近安排文字，不再使用远距离引导线
***************************************************************************/
station_label_rectangle build_station_label_candidate(
    int center_x, int center_y, int label_width, int label_height,
    int direction, int distance)
{
    station_label_rectangle candidate;

    switch (direction) {
        case 0:
            candidate.left = center_x - label_width / 2;
            candidate.top = center_y - distance - label_height;
            break;
        case 1:
            candidate.left = center_x - label_width / 2;
            candidate.top = center_y + distance;
            break;
        case 2:
            candidate.left = center_x - distance - label_width;
            candidate.top = center_y - label_height / 2;
            break;
        case 3:
            candidate.left = center_x + distance;
            candidate.top = center_y - label_height / 2;
            break;
        case 4:
            candidate.left = center_x + distance;
            candidate.top = center_y - distance - label_height;
            break;
        case 5:
            candidate.left = center_x - distance - label_width;
            candidate.top = center_y - distance - label_height;
            break;
        case 6:
            candidate.left = center_x + distance;
            candidate.top = center_y + distance;
            break;
        default:
            candidate.left = center_x - distance - label_width;
            candidate.top = center_y + distance;
            break;
    }

    candidate.right = candidate.left + label_width;
    candidate.bottom = candidate.top + label_height;
    return candidate;
}

/***************************************************************************
  函数名称：score_station_label_candidate
  功    能：给一个站名候选位置评分
  输入参数：候选矩形、已放置标签、全部节点和当前站点
  返 回 值：分数越低越适合
  说    明：越界和文字重叠重罚，距离轻罚，不产生任何标签引导线
***************************************************************************/
int score_station_label_candidate(
    const station_label_rectangle& candidate,
    const station_label_rectangle placed_rectangles[], int placed_number,
    const vertex vertices[], int vertex_number, int current_vertex, int distance)
{
    if (candidate.left < 4 || candidate.top < 4
        || candidate.right > ui_layout::map_width - 4
        || candidate.bottom > ui_layout::campus_list_top - 8)
        return INT_MAX / 4;

    int score = distance * 3;

    for (int i = 0; i < placed_number; i++) {
        int overlap_area = get_label_overlap_area(candidate, placed_rectangles[i]);
        if (overlap_area > 0)
            score += 20000 + overlap_area * 25;
    }

    int physical_limit = vertex_number;
    if (physical_limit > physical_vertex_number)
        physical_limit = physical_vertex_number;

    for (int i = 0; i < physical_limit; i++) {
        if (i == current_vertex || is_jiading_internal_vertex(i))
            continue;

        screen_point point = get_total_map_point(vertices, vertex_number, i);
        if (point.x >= candidate.left - 4 && point.x <= candidate.right + 4
            && point.y >= candidate.top - 4 && point.y <= candidate.bottom + 4)
            score += 5000;
    }

    return score;
}

/***************************************************************************
  函数名称：choose_station_label_rectangle
  功    能：在站点附近多个候选位置中选择最清楚的位置
  输入参数：站点坐标、标签宽高、已有标签和全部节点
  返 回 值：评分最低的候选矩形
  说    明：最大偏移78像素，不再把站名拉到很远并连大量灰线
***************************************************************************/
station_label_rectangle choose_station_label_rectangle(
    int center_x, int center_y, int label_width, int label_height,
    const station_label_rectangle placed_rectangles[], int placed_number,
    const vertex vertices[], int vertex_number, int current_vertex)
{
    const int distances[] = { 8, 15, 24, 34, 46, 60, 78 };
    const int distance_number = sizeof(distances) / sizeof(distances[0]);

    station_label_rectangle best_candidate;
    int best_score = INT_MAX;

    for (int distance_index = 0; distance_index < distance_number; distance_index++) {
        for (int direction = 0; direction < 8; direction++) {
            station_label_rectangle candidate = build_station_label_candidate(
                center_x, center_y, label_width, label_height,
                direction, distances[distance_index]);

            int score = score_station_label_candidate(
                candidate, placed_rectangles, placed_number,
                vertices, vertex_number, current_vertex, distances[distance_index]);

            if (score < best_score) {
                best_score = score;
                best_candidate = candidate;
            }
            if (score < 80)
                return candidate;
        }
    }

    return best_candidate;
}

/***************************************************************************
  函数名称：draw_station_label
  功    能：绘制普通总览站名标签
  输入参数：标签矩形和文字信息
  返 回 值：无
  说    明：使用与背景一致的无边框底色压住线路，不画任何引导线
***************************************************************************/
void draw_station_label(const station_label_rectangle& rectangle,
    const station_label_text& label)
{
    setfillcolor(RGB(247, 250, 252));
    solidrectangle(rectangle.left - 2, rectangle.top - 1,
        rectangle.right + 2, rectangle.bottom + 1);

    set_map_font(13);
    setbkmode(TRANSPARENT);
    settextcolor(RGB(28, 36, 48));
    outtextxy(rectangle.left, rectangle.top, label.first_line.c_str());

    if (label.line_number == 2)
        outtextxy(rectangle.left, rectangle.top + 15, label.second_line.c_str());
}

/***************************************************************************
  函数名称：get_station_label_density
  功    能：统计站点周围的实体站密度
  输入参数：顶点数组、顶点数量、当前站点编号
  返 回 值：附近站点数量
  说    明：密集站先安排文字，减少后放置标签无处可放的情况
***************************************************************************/
int get_station_label_density(const vertex vertices[], int vertex_number, int vertex_id)
{
    int density = 0;
    screen_point center = get_total_map_point(vertices, vertex_number, vertex_id);

    int physical_limit = vertex_number;
    if (physical_limit > physical_vertex_number)
        physical_limit = physical_vertex_number;

    for (int i = 0; i < physical_limit; i++) {
        if (i == vertex_id || is_jiading_internal_vertex(i))
            continue;

        screen_point other = get_total_map_point(vertices, vertex_number, i);
        int dx = other.x - center.x;
        int dy = other.y - center.y;
        if (dx * dx + dy * dy <= 95 * 95)
            density++;
    }

    return density;
}

/***************************************************************************
  函数名称：draw_all_station_names
  功    能：在总览图永久显示全部非嘉定校内实体站名
  输入参数：const vertex vertices[]：顶点数组；int vertex_number：顶点数量
  返 回 值：无
  说    明：同实体换乘站只写一次，长站名最多两行，全程不绘制引导线
***************************************************************************/
void draw_all_station_names(const vertex vertices[], int vertex_number)
{
    int physical_limit = vertex_number;
    if (physical_limit > physical_vertex_number)
        physical_limit = physical_vertex_number;

    int label_vertices[physical_vertex_number];
    int label_number = 0;
    string used_names[physical_vertex_number];
    int used_name_number = 0;

    for (int i = 0; i < physical_limit; i++) {
        if (is_jiading_internal_vertex(i))
            continue;

        string entity_name = get_entity_station_name(vertices, i);
        bool already_used = false;
        for (int j = 0; j < used_name_number; j++) {
            if (used_names[j] == entity_name) {
                already_used = true;
                break;
            }
        }
        if (already_used)
            continue;

        used_names[used_name_number++] = entity_name;
        label_vertices[label_number++] = i;
    }

    for (int i = 1; i < label_number; i++) {
        int current_vertex = label_vertices[i];
        int current_density = get_station_label_density(
            vertices, vertex_number, current_vertex);
        int j = i - 1;

        while (j >= 0
            && get_station_label_density(vertices, vertex_number, label_vertices[j])
                < current_density) {
            label_vertices[j + 1] = label_vertices[j];
            j--;
        }
        label_vertices[j + 1] = current_vertex;
    }

    station_label_rectangle placed_rectangles[physical_vertex_number];
    int placed_number = 0;
    set_map_font(13);

    for (int i = 0; i < label_number; i++) {
        int vertex_id = label_vertices[i];
        string station_name = get_entity_station_name(vertices, vertex_id);
        station_label_text label = build_station_label_text(station_name, 112);
        screen_point point = get_total_map_point(vertices, vertex_number, vertex_id);

        station_label_rectangle chosen = choose_station_label_rectangle(
            point.x, point.y, label.width, label.height,
            placed_rectangles, placed_number,
            vertices, vertex_number, vertex_id);

        draw_station_label(chosen, label);
        placed_rectangles[placed_number++] = chosen;
    }

    string campus_text = "嘉定校区";
    station_label_text campus_label = build_station_label_text(campus_text, 112);
    screen_point campus_point;
    campus_point.x = scale_total_map_x(150);
    campus_point.y = scale_total_map_y(235);

    station_label_rectangle campus_rectangle = choose_station_label_rectangle(
        campus_point.x, campus_point.y,
        campus_label.width, campus_label.height,
        placed_rectangles, placed_number,
        vertices, vertex_number, -1);
    draw_station_label(campus_rectangle, campus_label);
}

/***************************************************************************
  函数名称：draw_jiading_station_list
  功    能：在总览图底部列出嘉定校区八个内部节点的完整名称
  输入参数：const vertex vertices[]：顶点数组
  返 回 值：无
  说    明：保证一镜到底时所有站名同时可见，具体选点仍在同一窗口内进入嘉定局部图
***************************************************************************/
void draw_jiading_station_list(const vertex vertices[])
{
    setfillcolor(RGB(255, 255, 255));
    solidrectangle(ui_layout::campus_list_left, ui_layout::campus_list_top,
        ui_layout::campus_list_right, ui_layout::campus_list_bottom);
    setlinecolor(RGB(205, 214, 224));
    rectangle(ui_layout::campus_list_left, ui_layout::campus_list_top,
        ui_layout::campus_list_right, ui_layout::campus_list_bottom);

    set_ui_font(14, FW_BOLD);
    settextcolor(RGB(42, 55, 70));
    outtextxy(30, 650, "嘉定校区内部节点");

    const int column_width = 260;
    const int start_x = 30;
    const int start_y = 676;
    set_map_font(13);

    for (int i = 56; i <= 63; i++) {
        int index = i - 56;
        int column = index % 4;
        int row = index / 4;
        int x = start_x + column * column_width;
        int y = start_y + row * 50;

        setfillcolor(RGB(220, 122, 36));
        solidcircle(x + 5, y + 8, 3);

        station_label_text label = build_station_label_text(vertices[i].name, 225);
        settextcolor(RGB(32, 42, 54));
        outtextxy(x + 14, y, label.first_line.c_str());
        if (label.line_number == 2)
            outtextxy(x + 14, y + 15, label.second_line.c_str());
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
    campus_point.x = scale_total_map_x(150);
    campus_point.y = scale_total_map_y(235);
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
        screen_point point = get_total_map_point(vertices, vertex_number, i);
        draw_station_name_box(entity_name, point.x, point.y);
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
        screen_point point = get_total_map_point(
            vertices, vertex_number, state.hovered_vertex);
        draw_station_name_box(
            get_entity_station_name(vertices, state.hovered_vertex), point.x, point.y);
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
    outtextxy(left + 135, 222, time_text.c_str());
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
    outtextxy(left, 412, "总览始终展示全部站名，校内站点见左下角");

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
        draw_jiading_station_list(vertices);

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

'''

text = text[:ui_start] + ui_code + text[ui_end:]

input_start = block_start("void handle_left_click(")
main_start = block_start("int main()")

input_code = r'''/***************************************************************************
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

'''

text = text[:input_start] + input_code + text[main_start:]

old_init = "initgraph(1200, 700, EX_SHOWCONSOLE);"
if old_init not in text:
    raise RuntimeError("找不到旧initgraph尺寸")
text = text.replace(old_init,
    "initgraph(ui_layout::window_width, ui_layout::window_height, EX_SHOWCONSOLE);", 1)

# 旧的显示全部站名状态字段保留在ui_state中，不再作为显示开关使用，不影响算法数据。

encoded = text.replace("\n", "\r\n").encode("gb2312")
if b"\n" in encoded.replace(b"\r\n", b""):
    raise RuntimeError("main.cpp存在非CRLF换行")

decoded = encoded.decode("gb2312")
if decoded.count("void draw_schematic_connection(") != 0:
    raise RuntimeError("旧draw_schematic_connection重复代码未清除")
if decoded.count("void draw_base_network(") != 1:
    raise RuntimeError("新版基础网络绘制函数数量异常")
if "current_edge->type != edge_type::TRANSFER" not in decoded:
    raise RuntimeError("TRANSFER分层显示规则没有生效")
if "draw_jiading_station_list(vertices);" not in decoded:
    raise RuntimeError("嘉定全部站名清单没有生效")
if "ui_layout::window_width" not in decoded:
    raise RuntimeError("新版窗口尺寸没有生效")

path.write_bytes(encoded)
print("EasyX重构完成")
print("main.cpp保持GB2312 + CRLF")
print("窗口1500x800，地图1120x800")

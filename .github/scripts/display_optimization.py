from pathlib import Path
import re

path = Path('main.cpp')
text = path.read_bytes().decode('gb2312')
text = text.replace('\r\n', '\n').replace('\r', '\n')

COMMENT = '/***************************************************************************'

def block_start(signature: str) -> int:
    pos = text.index(signature)
    start = text.rfind(COMMENT, 0, pos)
    if start < 0:
        raise RuntimeError('找不到函数注释块: ' + signature)
    return start

# 1. 删除总览底部重复列出的嘉定校区8个节点
campus_start = block_start('void draw_jiading_station_list(')
campus_end = block_start('int find_clicked_vertex(')
text = text[:campus_start] + text[campus_end:]

call_pattern = re.compile(r'^\s*draw_jiading_station_list\(vertices\);\s*\n', re.M)
text, call_count = call_pattern.subn('', text)
if call_count != 1:
    raise RuntimeError(f'draw_jiading_station_list调用数量异常: {call_count}')

# 2. 删除右栏解释性提示
hint_pattern = re.compile(r'^.*总览始终展示全部站名，校内站点见左下角.*\n', re.M)
text, hint_count = hint_pattern.subn('', text)
if hint_count != 1:
    raise RuntimeError(f'右栏提示数量异常: {hint_count}')

# 3. 释放底部空间给总览网络，并清掉已经不再使用的campus_list布局常量
text = text.replace('    const int network_bottom = 1255;\n', '    const int network_bottom = 1460;\n', 1)
for line in [
    '    const int campus_list_left = 24;\n',
    '    const int campus_list_top = 1280;\n',
    '    const int campus_list_right = 2026;\n',
    '    const int campus_list_bottom = 1482;\n',
]:
    if line not in text:
        raise RuntimeError('找不到待删除布局常量: ' + line.strip())
    text = text.replace(line, '', 1)

old_boundary = '        || candidate.bottom > ui_layout::campus_list_top - 8)\n'
new_boundary = '        || candidate.bottom > ui_layout::network_bottom - 8)\n'
if old_boundary not in text:
    raise RuntimeError('找不到站名纵向边界判断')
text = text.replace(old_boundary, new_boundary, 1)

# 4. 新增“标签不得压住运营线路”的几何判断
score_comment = block_start('int score_station_label_candidate(')
geometry_code = r'''/***************************************************************************
  函数名称：cross_product_for_label
  功    能：计算线段相交判断使用的二维叉积
  输入参数：三个屏幕坐标点
  返 回 值：叉积结果
  说    明：仅用于总览图站名避让，不参与任何路径算法
***************************************************************************/
long long cross_product_for_label(
    const screen_point& first,
    const screen_point& second,
    int x, int y)
{
    return static_cast<long long>(second.x - first.x) * (y - first.y)
        - static_cast<long long>(second.y - first.y) * (x - first.x);
}

/***************************************************************************
  函数名称：is_point_on_label_segment
  功    能：判断一点是否位于给定线段的包围范围内
  输入参数：线段两个端点与待判断点坐标
  返 回 值：在范围内返回true
  说    明：配合叉积处理共线情况
***************************************************************************/
bool is_point_on_label_segment(
    const screen_point& first,
    const screen_point& second,
    int x, int y)
{
    int min_x = first.x < second.x ? first.x : second.x;
    int max_x = first.x > second.x ? first.x : second.x;
    int min_y = first.y < second.y ? first.y : second.y;
    int max_y = first.y > second.y ? first.y : second.y;
    return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
}

/***************************************************************************
  函数名称：does_segment_intersect_label_side
  功    能：判断一条运营线路与标签矩形的一条边是否相交
  输入参数：运营线路端点、矩形边两个端点
  返 回 值：相交返回true
  说    明：用于给站名候选位置增加避线惩罚
***************************************************************************/
bool does_segment_intersect_label_side(
    const screen_point& line_first,
    const screen_point& line_second,
    const screen_point& side_first,
    const screen_point& side_second)
{
    long long first_cross = cross_product_for_label(
        line_first, line_second, side_first.x, side_first.y);
    long long second_cross = cross_product_for_label(
        line_first, line_second, side_second.x, side_second.y);
    long long third_cross = cross_product_for_label(
        side_first, side_second, line_first.x, line_first.y);
    long long fourth_cross = cross_product_for_label(
        side_first, side_second, line_second.x, line_second.y);

    if (((first_cross > 0 && second_cross < 0)
        || (first_cross < 0 && second_cross > 0))
        && ((third_cross > 0 && fourth_cross < 0)
        || (third_cross < 0 && fourth_cross > 0)))
        return true;

    if (first_cross == 0 && is_point_on_label_segment(
        line_first, line_second, side_first.x, side_first.y))
        return true;
    if (second_cross == 0 && is_point_on_label_segment(
        line_first, line_second, side_second.x, side_second.y))
        return true;
    if (third_cross == 0 && is_point_on_label_segment(
        side_first, side_second, line_first.x, line_first.y))
        return true;
    if (fourth_cross == 0 && is_point_on_label_segment(
        side_first, side_second, line_second.x, line_second.y))
        return true;

    return false;
}

/***************************************************************************
  函数名称：does_network_edge_cross_label
  功    能：判断一条总览运营线路是否穿过站名标签区域
  输入参数：线路两个端点、候选标签矩形
  返 回 值：穿过或进入标签区域返回true
  说    明：标签区域额外扩张4像素，避免文字虽然没压线但视觉上贴得太近
***************************************************************************/
bool does_network_edge_cross_label(
    const screen_point& first,
    const screen_point& second,
    const station_label_rectangle& rectangle)
{
    station_label_rectangle expanded = rectangle;
    expanded.left -= 4;
    expanded.top -= 4;
    expanded.right += 4;
    expanded.bottom += 4;

    auto inside = [&](const screen_point& point) {
        return point.x >= expanded.left && point.x <= expanded.right
            && point.y >= expanded.top && point.y <= expanded.bottom;
    };
    if (inside(first) || inside(second))
        return true;

    screen_point top_left{ expanded.left, expanded.top };
    screen_point top_right{ expanded.right, expanded.top };
    screen_point bottom_left{ expanded.left, expanded.bottom };
    screen_point bottom_right{ expanded.right, expanded.bottom };

    return does_segment_intersect_label_side(first, second, top_left, top_right)
        || does_segment_intersect_label_side(first, second, top_right, bottom_right)
        || does_segment_intersect_label_side(first, second, bottom_right, bottom_left)
        || does_segment_intersect_label_side(first, second, bottom_left, top_left);
}

'''
text = text[:score_comment] + geometry_code + text[score_comment:]

# 5. 在候选位置评分里加入运营线路穿越惩罚
score_start = text.index('int score_station_label_candidate(')
return_pos = text.index('    return score;\n', score_start)
edge_penalty = r'''
    for (int i = 0; i < vertex_number; i++) {
        const edge_node* current_edge = vertices[i].first_edge;
        while (current_edge) {
            if (i < current_edge->to
                && current_edge->type != edge_type::TRANSFER) {
                screen_point first_point = get_total_map_point(
                    vertices, vertex_number, i);
                screen_point second_point = get_total_map_point(
                    vertices, vertex_number, current_edge->to);

                if (does_network_edge_cross_label(
                    first_point, second_point, candidate))
                    score += 12000;
            }
            current_edge = current_edge->next;
        }
    }

'''
text = text[:return_pos] + edge_penalty + text[return_pos:]

# 6. 最终结构检查
for forbidden in [
    'void draw_jiading_station_list(',
    'draw_jiading_station_list(vertices);',
    '总览始终展示全部站名，校内站点见左下角',
    'ui_layout::campus_list_top',
]:
    if forbidden in text:
        raise RuntimeError('残留内容: ' + forbidden)

required = [
    'const int window_width = 2600;',
    'const int window_height = 1500;',
    'const int network_bottom = 1460;',
    'string get_map_display_name(',
    'does_network_edge_cross_label(',
    'draw_all_station_names(',
    'draw_jiading_campus_map(',
    'draw_route_result(',
]
for marker in required:
    if marker not in text:
        raise RuntimeError('缺少关键结构: ' + marker)

raw = text.replace('\n', '\r\n').encode('gb2312')
path.write_bytes(raw)
print('显示优化完成')
print('main.cpp保持GB2312 + CRLF')
print('删除底部校内节点清单与右栏冗余提示')
print('站名布局新增运营线路避让')

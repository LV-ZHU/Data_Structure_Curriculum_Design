from pathlib import Path

path = Path('main.cpp')
text = path.read_bytes().decode('gb2312')
text = text.replace('\r\n', '\n').replace('\r', '\n')

replacements = {
'''    const int window_width = 1500;\n    const int window_height = 800;\n    const int map_width = 1120;''': '''    const int window_width = 2600;\n    const int window_height = 1500;\n    const int map_width = 2050;''',
'''    const int network_left = 20;\n    const int network_right = 1100;\n    const int network_top = 20;\n    const int network_bottom = 625;''': '''    const int network_left = 35;\n    const int network_right = 2020;\n    const int network_top = 35;\n    const int network_bottom = 1255;''',
'''    const int campus_list_left = 18;\n    const int campus_list_top = 642;\n    const int campus_list_right = 1102;\n    const int campus_list_bottom = 787;''': '''    const int campus_list_left = 24;\n    const int campus_list_top = 1280;\n    const int campus_list_right = 2026;\n    const int campus_list_bottom = 1482;''',
'''    const int jiading_offset_x = 110;\n    const int jiading_offset_y = 50;''': '''    const int jiading_offset_x = 575;\n    const int jiading_offset_y = 260;''',
'''    const int panel_content_left = 1142;\n    const int panel_content_right = 1478;''': '''    const int panel_content_left = 2090;\n    const int panel_content_right = 2560;''',
'''    SetWindowPos(easyx_window, nullptr,\n        28, 8, 0, 0,''': '''    SetWindowPos(easyx_window, nullptr,\n        18, 8, 0, 0,''',
'''    result.height = 15;''': '''    result.height = 22;''',
'''    result.height = 30;''': '''    result.height = 44;''',
'''    const int safe_gap = 2;''': '''    const int safe_gap = 6;''',
'''    const int distances[] = { 8, 15, 24, 34, 46, 60, 78 };''': '''    const int distances[] = { 12, 22, 34, 48, 64, 82, 104, 128 };''',
'''    set_map_font(13);''': '''    set_map_font(20);''',
'''        outtextxy(rectangle.left, rectangle.top + 15, label.second_line.c_str());''': '''        outtextxy(rectangle.left, rectangle.top + 22, label.second_line.c_str());''',
'''        if (dx * dx + dy * dy <= 95 * 95)''': '''        if (dx * dx + dy * dy <= 170 * 170)''',
'''        string station_name = get_entity_station_name(vertices, vertex_id);\n        station_label_text label = build_station_label_text(station_name, 112);''': '''        string station_name = get_map_display_name(vertices, vertex_id);\n        station_label_text label = build_station_label_text(station_name, 180);''',
'''    station_label_text campus_label = build_station_label_text(campus_text, 112);''': '''    station_label_text campus_label = build_station_label_text(campus_text, 180);''',
'''    set_ui_font(14, FW_BOLD);\n    settextcolor(RGB(42, 55, 70));\n    outtextxy(30, 650, "嘉定校区内部节点");\n\n    const int column_width = 260;\n    const int start_x = 30;\n    const int start_y = 676;\n    set_map_font(13);''': '''    set_ui_font(20, FW_BOLD);\n    settextcolor(RGB(42, 55, 70));\n    outtextxy(ui_layout::campus_list_left + 18,\n        ui_layout::campus_list_top + 14, "嘉定校区内部节点");\n\n    const int column_width = 490;\n    const int start_x = ui_layout::campus_list_left + 18;\n    const int start_y = ui_layout::campus_list_top + 54;\n    set_map_font(20);''',
'''        int y = start_y + row * 50;''': '''        int y = start_y + row * 72;''',
'''        station_label_text label = build_station_label_text(vertices[i].name, 225);''': '''        station_label_text label = build_station_label_text(vertices[i].name, 430);''',
'''            outtextxy(x + 14, y + 15, label.second_line.c_str());''': '''            outtextxy(x + 14, y + 22, label.second_line.c_str());''',
'''            setlinestyle(PS_DASH, 1);''': '''            setlinestyle(PS_DASH, 2);''',
'''            setlinestyle(PS_SOLID, 2);''': '''            setlinestyle(PS_SOLID, 3);''',
'''    setlinestyle(PS_SOLID, 4);''': '''    setlinestyle(PS_SOLID, 6);''',
'''            setlinestyle(PS_DASH, 3);''': '''            setlinestyle(PS_DASH, 5);''',
'''            setlinestyle(PS_SOLID, 7);''': '''            setlinestyle(PS_SOLID, 10);'''
}

for old, new in replacements.items():
    if old not in text:
        raise RuntimeError('找不到待替换内容:\n' + old[:120])
    text = text.replace(old, new, 1)

anchor = '''/***************************************************************************\n  函数名称：build_station_label_text'''
if anchor not in text:
    raise RuntimeError('找不到站名构造函数锚点')

helper = r'''/***************************************************************************
  函数名称：get_map_display_name
  功    能：生成总览图专用的简洁站名
  输入参数：const vertex vertices[]：顶点数组；int vertex_id：物理节点编号
  返 回 值：仅用于总览显示的精简名称
  说    明：不修改CSV和算法站名，只缩短道路型公交站名称，减少密集区域文字拥挤
***************************************************************************/
string get_map_display_name(const vertex vertices[], int vertex_id)
{
    string name = get_entity_station_name(vertices, vertex_id);

    const string road_prefixes[] = {
        "曹安公路", "百安公路", "昌吉东路"
    };
    const int prefix_number =
        sizeof(road_prefixes) / sizeof(road_prefixes[0]);

    for (int i = 0; i < prefix_number; i++) {
        const string& prefix = road_prefixes[i];
        if (name.find(prefix) == 0 && name.size() > prefix.size())
            return name.substr(prefix.size());
    }

    if (name == "公交昌吉东路站")
        return "昌吉东路站";
    if (name == "同济大学沪西校区教师班车点")
        return "沪西校区班车点";
    if (name == "同济大学沪北校区")
        return "沪北校区";
    if (name == "四平路校区西南门停车场")
        return "四平路校区西南门";

    return name;
}

'''
text = text.replace(anchor, helper + anchor, 1)

# 放大后的站点圆圈与字体匹配
text = text.replace('fillcircle(point.x, point.y, 5);', 'fillcircle(point.x, point.y, 7);', 1)
text = text.replace('fillcircle(campus_point.x, campus_point.y, 7);', 'fillcircle(campus_point.x, campus_point.y, 10);', 1)

# 检查关键目标
checks = [
    'const int window_width = 2600;',
    'const int window_height = 1500;',
    'const int map_width = 2050;',
    'string get_map_display_name(',
    'get_map_display_name(vertices, vertex_id)',
    '"曹安公路", "百安公路", "昌吉东路"'
]
for item in checks:
    if item not in text:
        raise RuntimeError('重构结果缺少: ' + item)

path.write_bytes(text.replace('\n', '\r\n').encode('gb2312'))
print('站点显示修正完成')
print('窗口2600x1500，地图2050x1500')
print('总览公交道路站名已简化，原始数据保持不变')

#include <iostream>
#include <string>
#include <iomanip>
#include "include/common/structure.h"
#include "include/csv_data.h"
#include "include/adjacency_list.h"
#include "include/min_heap.h"
#include "include/dijkstra.h"
#include "include/easyx.h"
using namespace std;

/***************************************************************************
  函数名称：main
  功    能：程序入口
  输入参数：无
  返 回 值：正常完成整个项目功能则返回0
  说    明：先开启画面放大，然后开始从CSV里以及嘉定校区小图里读数据，再开始执行整个EasyX可视化
***************************************************************************/
int main()
{
    enable_high_dpi_rendering();

    string lines_path = "data/lines.csv";//线路CSV路径
    string stations_path = "data/stations.csv";//站点CSV路径
    string edges_path = "data/edges.csv";//边CSV路径
    transit_line lines[max_lines];
    int line_number = 0;//当前实际线路数量，初始还没加站点所以为0
    if (!load_transit_lines(lines_path, lines, line_number)) {
        cout << "加载线路CSV文件失败" << endl;
        return 1;
    }

    adjacency_list graph;
    if (!load_vertices(stations_path, graph)) {
        cout << "加载站点CSV文件失败" << endl;
        return 2;
    }

    if (!load_edges(edges_path, graph, lines, line_number)) {
        cout << "加载边CSV文件失败" << endl;
        return 3;
    }

    vertex* vertices = graph.get_vertices();
    int vertex_number = graph.get_vertex_number();


#if test_adjacency_list
    for (int i = 0; i < vertex_number; i++)
        graph.output_one_step_vertex(vertices[i]);
#endif

    initgraph(ui_layout::window_width, ui_layout::window_height, EX_SHOWCONSOLE);
    lock_easyx_window_size();
    IMAGE jiading_background;

    loadimage(&jiading_background, "data/jiading_campus.png");

    jiading_map_node jiading_map_nodes[] = {
    {56, 536,382, 545, 383, "教学楼/定班车点"},
    {57, 408, 490, 360, 470, "济事楼"},
    {58, 642, 326, 660, 305, "友园8号楼"},
    {59, 350, 510, 240, 530, "绿苑路曹安公路"},
    {60, 482, 330, 495, 310, "仰望星空停靠点"},
    {61, 601, 353, 615, 355, "7号楼候车点"},
    {62, 586, 72, 600, 82, "北门"},
    {63, 12, 30, 30, 52, "大桥东侧骑车点"},
    {69, 550, 62, 585, 40, "昌吉东路绿苑路"},
    {73, 95, 390, 115, 365, "曹安公路二十三号桥"},
    {84, 668, 652, 690, 620, "曹安公路嘉松北路"}
    };

    const int jiading_map_node_number = sizeof(jiading_map_nodes) / sizeof(jiading_map_nodes[0]);

#if test_min_heap
    min_heap test;
    test.initialize_heap(2);
    test.insert_heap(0, 2);
    test.insert_heap(1, 5);
    test.insert_heap(2, 3);
    cout << "测试扩容功能，当前size和capacity分别是：" << test.get_size() << " " << test.get_capacity() << endl;
    test.insert_heap(3, 8);
    test.insert_heap(4, 9);
    test.insert_heap(5, 4);
    test.insert_heap(6, 1);
    cout << "测试扩容功能，当前size和capacity分别是：" << test.get_size() << " " << test.get_capacity() << endl;
    while (!test.is_heap_empty()) {//只要还没空，就一直取元素，相比直接判断size在忘记extract_min会自动--的时候额外加了一行test.size--且初始size为奇数时不会死循环
        heap_node min;
        test.extract_min(min);
        cout << "本轮取出的堆顶元素序号和距离是：" << min.vertex_id << " " << min.distance << endl;
    }
    heap_node empty;
    cout << "测试空堆弹出是否正常，0是对的：" << test.extract_min(empty) << endl;
    test.release_heap();
#endif

#if test_weight
    edge_node cal;
    cal.time_cost = 6;
    cal.fare_cost = 0.5;
    cout << calculate_weight(cal, 0) << endl << calculate_weight(cal, 8) << endl;
#endif


    ui_state state;

    run_easyx_interface(
        vertices,
        vertex_number,
        lines,
        line_number,
        jiading_map_nodes,
        jiading_map_node_number,
        jiading_background,
        state);

    for (int i = 0; i < vertex_number; i++)
        graph.release_edges(vertices[i]);
    closegraph();

    return 0;
}

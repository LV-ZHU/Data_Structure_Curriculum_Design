#pragma once

#include <climits>
#include <limits>
#include <string>
using namespace std;

#define test_csv_lines 0 //文件打开、表头、整行和三个字段解析
#define test_transit_lines 0 //线路添加、类型限制、编号连续性 
#define test_adjacency_list	 0 //顶点、头插法、无向边、遍历、释放
#define test_min_heap 0 //初始化、交换、上浮、扩容、下沉、弹出、空堆、释放
#define test_weight 0 //时间、经济权值和骑行有效时间
#define test_dijkstra 0 //松弛、前驱、重复入堆和旧堆节点跳过

const int max_vertices = 150; //最多150个顶点
const int physical_vertex_number = 92; //前92个为EasyX可见、可点击的真实物理节点
const int max_lines = 50;//线路最多50条

enum class edge_type { METRO, BUS, TRANSFER };//边类型：地铁，公交，换乘
enum class station_type { METRO, BUS };//站点类型:地铁，公交

//边结构体，E
struct edge_node {
    int to = INT_MAX;//目标顶点编号
    int time_cost = INT_MAX;//这条边记录的耗时
    double fare_cost = std::numeric_limits<double>::infinity();//费用
    edge_type type = edge_type::METRO;//边的类型，默认为地铁
    int line_id = -1; //步行(骑行)默认-1，由于有多条地铁和公交，该字段联合type可确定具体是哪一条线路
    int bike_time_cost = -1;//地铁和公交默认-1，换乘情况若可骑行则改为对应骑行时间
    edge_node* next = nullptr;//指向同一邻接链表中的下一个边结点
};
//顶点结构体，V
struct vertex {
    int id = INT_MAX;//站点编号
    string name = "";//站点名称
    station_type type = station_type::METRO;//站点类型，默认为地铁
    edge_node* first_edge = nullptr;//邻接链表的第一个边节点地址
    int x;//EasyX需要的站点圆心x坐标
    int y;//EasyX需要的站点圆心y坐标
};
//候选顶点及其距离结构体
struct heap_node {
    int vertex_id;//顶点编号
    double distance;//当前距离
};
//线路表
struct transit_line {
    int id = -1;//线路编号，等于线路在数组里的下标
    string name = "";//线路中文名称
    edge_type type = edge_type::METRO;//线路类型，默认为地铁，只允许是地铁/公交类型不允许为换乘
};
//路线阶段：把Dijkstra原始边压缩成用户真正看到的一整段行程
struct route_segment {
    int start_vertex = -1;//本段起点物理顶点编号
    int end_vertex = -1;//本段终点物理顶点编号
    edge_type type = edge_type::TRANSFER;//地铁、公交或换乘
    int line_id = -1;//地铁/公交线路编号，步行/骑行保持-1
    bool use_bike = false;//TRANSFER边实际是否选择骑行
    int time_cost = 0;//本段累计耗时
    double fare_cost = 0;//本段累计费用
    int station_edge_number = 0;//本段真正跨越的物理站间边数量
};
//嘉定部分
struct jiading_map_node {
    int vertex_id = -1;       //原图中的物理顶点编号
    int x = 0;                //嘉定局部图中的圆心x
    int y = 0;                //嘉定局部图中的圆心y
    int label_x = 0;          //标签左上角x
    int label_y = 0;          //标签左上角y
    string short_name = "";   //局部图显示的短名称
};
//EasyX界面状态
struct ui_state {
    int start_vertex = -1;
    int end_vertex = -1;
    int k = 0;
    bool allow_bike = false;
    bool show_all_names = true;

    int start_hour = 8;
    int start_minute = 30;

    double distance[max_vertices];
    int previous_vertex[max_vertices];
    const edge_node* previous_edge[max_vertices];
    int path[max_vertices];
    int path_vertex_number = 0;

    route_segment segments[max_vertices];
    int segment_number = 0;
    int guide_page = 0;
    int stop_number = 0;
    int hovered_vertex = -1;

    int total_time_cost = 0;
    double total_fare_cost = 0;
    int arrival_hour = 0;
    int arrival_minute = 0;
    int days_passed = 0;

    bool route_ready = false;
    string message = "请选择起点";
    bool is_jiading_campus = false;
};

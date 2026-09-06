#include "../include/adjacency_list.h"
#include <iostream>
using namespace std;

/***************************************************************************
  函数名称：add_vertex
  功    能：加入新顶点
  输入参数：string vertex_name：站点名称
  station_type type：站点类型
  int x：站点圆心x坐标
  int y：站点圆心y坐标
  返 回 值：true(1)为正常，false(0)为已经放满
  说    明：先添加，再自增，从0开始；顶点数组和当前顶点数量由邻接表对象保存
***************************************************************************/
bool adjacency_list::add_vertex(string vertex_name, station_type type, int x, int y)
{
    if (vertex_number >= max_vertices)
        return false;//超额，添加失败
    else {
        vertices[vertex_number].id = vertex_number;//id即为当前号码
        vertices[vertex_number].name = vertex_name;
        vertices[vertex_number].type = type;
        vertices[vertex_number].x = x;
        vertices[vertex_number].y = y;
        vertex_number++;//全添加完再自增
        return true;
    }
}

/***************************************************************************
  函数名称：add_directed_edge
  功    能：用头插法添加一条边，由于添加的是有向边，在无向边情况下两个方向各调用一次该函数
  输入参数：vertex& from_vertex：要修改的顶点，
    int to：目标顶点编号，
    int time_cost：耗时，
    double fare_cost：费用，
    edge_type type：边类型，
    int line_id：仅公交/地铁时代表线路号，换乘边默认-1
    int bike_time_cost：仅存在骑行且type为transfer时才赋值，其余时候默认-1
  返 回 值：空
  说    明：修改来源顶点的 first_edge，把新边插入邻接链表
***************************************************************************/
void adjacency_list::add_directed_edge(vertex& from_vertex, int to, int time_cost, double fare_cost, edge_type type, int line_id, int bike_time_cost)
{
    edge_node* new_edge = new edge_node;
    new_edge->to = to;
    new_edge->time_cost = time_cost;
    new_edge->fare_cost = fare_cost;
    new_edge->type = type;
    new_edge->next = from_vertex.first_edge;
    if (new_edge->type == edge_type::METRO || new_edge->type == edge_type::BUS)
        new_edge->line_id = line_id;
    else if (new_edge->type == edge_type::TRANSFER)
        new_edge->bike_time_cost = bike_time_cost;
    from_vertex.first_edge = new_edge;

}
/***************************************************************************
  函数名称：add_undirected_edge
  功    能：调用两次add_directed_edge完成两个方向
  输入参数：vertex& first_vertex, vertex& second_vertex：两个顶点，其余五个参数两边共享，含义同add_directed_edge函数
  返 回 值：空
  说    明：添加无向边
***************************************************************************/
void adjacency_list::add_undirected_edge(vertex& first_vertex, vertex& second_vertex, int time_cost, double fare_cost, edge_type type, int line_id, int bike_time_cost)
{
    add_directed_edge(first_vertex, second_vertex.id, time_cost, fare_cost, type, line_id, bike_time_cost);
    add_directed_edge(second_vertex, first_vertex.id, time_cost, fare_cost, type, line_id, bike_time_cost);
}
/***************************************************************************
  函数名称：release_edges
  功    能：释放add_edge函数创建的各个edge_node
  输入参数：vertex& release_vertex：需要delete的邻接表的顶点
  返 回 值：空
  说    明：从顶点的first_edge开始按顺序释放整个链表的各个new出来的节点
***************************************************************************/
void adjacency_list::release_edges(vertex& release_vertex)
{
    while (release_vertex.first_edge) {
        edge_node* next_node = release_vertex.first_edge->next;
        delete release_vertex.first_edge;
        release_vertex.first_edge = next_node;
    }
}
/***************************************************************************
  函数名称：output_one_step_vertex
  功    能：输出从start_station开始所有的邻居
  输入参数：start_station：开始的车站
  返 回 值：空
  说    明：依次遍历开始站点的邻接表
***************************************************************************/
void adjacency_list::output_one_step_vertex(vertex& start_station) const
{
    if (start_station.first_edge == nullptr)
        cout << "该节点为孤立顶点，当前站点暂无可用路线" << endl;
    else {
        edge_node* current_edge = start_station.first_edge;//指针从站点第一个节点开始
        while (current_edge) {
            cout << current_edge->to << " ";
            current_edge = current_edge->next;
        }
        cout << endl;
    }
}
/***************************************************************************
  函数名称：find_directed_edge
  功    能：找有向边
  输入参数：const vertex & from_vertex：起点顶点，int to：目标顶点编号
  返 回 值：const edge_node*，返回找到的边
  说    明：打印时需要具体路径
***************************************************************************/
const edge_node* adjacency_list::find_directed_edge(const vertex& from_vertex, int to) const
{
    const edge_node* current_edge = from_vertex.first_edge;
    while (current_edge) {
        if (current_edge->to == to)
            return current_edge;
        current_edge = current_edge->next;
    }
    return nullptr;//遍历后没找到这条边
}

/***************************************************************************
  函数名称：~adjacency_list
  功    能：释放邻接表中所有动态申请的边结点
  输入参数：无
  返 回 值：无
  说    明：main中仍保留显式release_edges流程；已释放的链表在析构时不会重复释放
***************************************************************************/
adjacency_list::~adjacency_list()
{
    for (int i = 0; i < vertex_number; i++)
        release_edges(vertices[i]);
}

/***************************************************************************
  函数名称：get_vertex_number
  功    能：取得邻接表中当前实际顶点数量
  输入参数：无
  返 回 值：当前vertex_number
  说    明：用于CSV读取、Dijkstra和EasyX共用同一份顶点数据
***************************************************************************/
int adjacency_list::get_vertex_number() const
{
    return vertex_number;
}

/***************************************************************************
  函数名称：get_vertices
  功    能：取得邻接表内部顶点数组首地址
  输入参数：无
  返 回 值：vertex*
  说    明：保留原有Dijkstra和EasyX的数组参数形式，避免为了类化邻接表重写其他模块
***************************************************************************/
vertex* adjacency_list::get_vertices()
{
    return vertices;
}

/***************************************************************************
  函数名称：get_vertices
  功    能：取得只读的邻接表内部顶点数组首地址
  输入参数：无
  返 回 值：const vertex*
  说    明：const对象调用时只允许读取顶点数组
***************************************************************************/
const vertex* adjacency_list::get_vertices() const
{
    return vertices;
}

#include <iostream>
#include <string>
#include <climits>
using namespace std;

const int max_vertices = 100; //最多100个顶点

enum class edge_type { METRO, BUS, TRANSFER };//边类型：地铁，公交，换乘
enum class station_type{METRO, BUS};//站点类型:地铁，公交

//边结构体，E
struct edge_node {
	int to = INT_MAX;//目标顶点编号
	int time_cost = INT_MAX;//这条边记录的耗时
	double fare_cost = 0;//费用
	edge_type type = edge_type::METRO;//边的类型，默认为地铁
	edge_node* next = nullptr;//指向同一邻接链表中的下一个边结点
};
//顶点结构体，V
struct vertex {
	int id = INT_MAX;//站点编号
	string name = "";//站点名称
	station_type type = station_type::METRO;//站点类型，默认为地铁
	edge_node* first_edge = nullptr;//邻接链表的第一个边节点地址
};
//候选顶点及其距离结构体
struct heap_node {
	int vertex_id;//顶点编号
	double distance;//当前距离
};
//最小堆
struct min_heap {
	heap_node* data = nullptr;//当前指向元素位置
	int size = 0;//当前实际存有的堆元素数量
	int capacity = 0;//当前数组最多能容纳多少个元素，应该始终满足0≤size≤capacity
};

/***************************************************************************
  函数名称：add_directed_edge
  功    能：用头插法添加一条边，由于添加的是有向边，在无向边情况下两个方向各调用一次该函数
  输入参数：vertex& from_vertex：要修改的顶点，
	int to：目标顶点编号，
	int time_cost：耗时，
	double fare_cost：费用，
	edge_type type：边类型
  返 回 值：空
  说    明：修改来源顶点的 first_edge，把新边插入邻接链表
***************************************************************************/
void add_directed_edge(vertex& from_vertex, int to, int time_cost, double fare_cost, edge_type type) 
{
	edge_node* new_edge = new edge_node;
	new_edge->to = to;
	new_edge->time_cost = time_cost;
	new_edge->fare_cost = fare_cost;
	new_edge->type = type;
	new_edge->next = from_vertex.first_edge;
	from_vertex.first_edge = new_edge;
}

/***************************************************************************
  函数名称：add_undirected_edge
  功    能：调用两次add_directed_edge完成两个方向
  输入参数：vertex& first_vertex, vertex& second_vertex：两个顶点，其余三个参数两边共享，含义同add_directed_edge函数
  返 回 值：空
  说    明：添加无向边
***************************************************************************/
void add_undirected_edge(vertex& first_vertex, vertex& second_vertex, int time_cost, double fare_cost, edge_type type)
{
	add_directed_edge(first_vertex, second_vertex.id, time_cost, fare_cost, type);
	add_directed_edge(second_vertex, first_vertex.id, time_cost, fare_cost, type);
}

/***************************************************************************
  函数名称：release_edges
  功    能：释放add_edge函数创建的各个edge_node
  输入参数：vertex& release_vertex：需要delete的邻接表的顶点
  返 回 值：空
  说    明：从顶点的first_edge开始按顺序释放整个链表的各个new出来的节点
***************************************************************************/
void release_edges(vertex& release_vertex)
{
	while (release_vertex.first_edge) {
		edge_node *next_node= release_vertex.first_edge->next;
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
void output_one_step_vertex(vertex& start_station)
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
  函数名称：add_vertex
  功    能：加入新顶点
  输入参数：vertex vertices[max_vertices]:顶点数组
  int& vertex_number：当前顶点数量的引用，实时修改所以使用引用
  string vertex_name：站点名称
  station_type type：站点类型
  返 回 值：true(1)为正常，false(0)为已经放满
  说    明：先添加，再自增，从0开始
***************************************************************************/
bool add_vertex(vertex vertices[max_vertices], int& vertex_number,string vertex_name, station_type type)
{
	if (vertex_number >= max_vertices)
		return false;//超额，添加失败
	else {
		vertices[vertex_number].id = vertex_number;//id即为当前号码
		vertices[vertex_number].name = vertex_name;
		vertices[vertex_number].type= type;
		vertex_number++;//全添加完再自增
		return true;
	}	
}

/***************************************************************************
  函数名称：initialize_heap
  功    能：初始化最小堆
  输入参数：min_heap& heap：需要改变成最小堆的堆
  int initial_capacity：初始容量
  返 回 值：false表示失败，true表示成功
  说    明：
***************************************************************************/
bool initialize_heap(min_heap& heap,int initial_capacity)
{
	if (initial_capacity <= 0|| heap.data)
		return false;
	heap_node* p = new heap_node[initial_capacity];
	heap.data = p;
	heap.size = 0;
	heap.capacity = initial_capacity;
	return true;
}

/***************************************************************************
  函数名称：release_heap
  功    能：释放整个堆
  输入参数：min_heap& heap：需要释放的堆
  返 回 值：无
  说    明：先释放动态内存申请的数组，然后重置结构体里各个成员的值
***************************************************************************/
void release_heap(min_heap& heap) 
{
	delete[] heap.data;
	heap.data = nullptr;
	heap.size = 0;
	heap.capacity = 0;
}

int main()
{
	cout << "Transit Navigator started." << endl;
	vertex vertices[max_vertices];

	int vertex_number = 0;//当前实际顶点数量，初始还没加站点所以为0
	add_vertex(vertices, vertex_number, "Tongji University", station_type::METRO);
	add_vertex(vertices, vertex_number, "Siping Road", station_type::METRO);
	add_vertex(vertices, vertex_number, "Guokang Road Siping Road", station_type::BUS);

	cout << vertex_number << endl;

	add_undirected_edge(vertices[0], vertices[1], 3, 0.3, edge_type::METRO);
	add_undirected_edge(vertices[0], vertices[2], 6, 0, edge_type::TRANSFER);
	for (int i = 0; i < vertex_number; i++)
		output_one_step_vertex(vertices[i]);
	for (int i = 0; i < vertex_number; i++)
		release_edges(vertices[i]);

	return 0;
}
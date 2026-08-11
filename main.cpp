#include <iostream>
#include <string>
using namespace std;

enum class edge_type { METRO, BUS, TRANSFER };//边类型：地铁，公交，换乘
enum class station_type{METRO, BUS};//站点类型:地铁，公交

//边结构体，E
struct edge_node {
	int to;//目标顶点编号
	int time_cost;//这条边记录的耗时
	double fare_cost;//费用
	edge_type type;//边的类型
	edge_node* next = nullptr;//指向同一邻接链表中的下一个边结点
};
//顶点结构体，V
struct vertex {
	int id;//站点编号
	string name;//站点名称
	station_type type;//站点类型
	edge_node* first_edge = nullptr;//邻接链表的第一个边节点地址
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

int main()
{
	cout << "Transit Navigator started." << endl;
	vertex station_a;
	station_a.id = 0;
	station_a.name = "Tongji University";
	station_a.type = station_type::METRO;
	
	vertex station_b;
	station_b.id = 1;
	station_b.name = "Siping Road";
	station_b.type = station_type::METRO;

	vertex station_c;
	station_c.id = 2;
	station_c.name = "Guokang Road Siping Road";
	station_c.type = station_type::BUS;

	add_undirected_edge(station_a, station_b, 3, 0.3, edge_type::METRO);
	add_undirected_edge(station_a, station_c, 6, 0, edge_type::TRANSFER);
	


	if (station_a.first_edge == nullptr)
		cout << "该节点为孤立顶点，当前站点暂无可用路线" << endl;
	else {
		edge_node* current_edge = station_a.first_edge;//指针从站点第一个节点开始
		while (current_edge) {
			cout << current_edge->to << " ";
			current_edge = current_edge->next;
		}
	}
	release_edges(station_a);
	release_edges(station_b);


	return 0;
}
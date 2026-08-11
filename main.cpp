#include <iostream>
#include <string>
using namespace std;

enum class station_type{METRO, BUS};//站点类型:地铁，公交
enum class edge_type{METRO,BUS,TRANSFER};//边类型：地铁，公交，换乘
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
  函数名称：add_edge
  功    能：添加一条边，由于添加的是有向边，在无向边情况下两个方向各调用一次该函数
  输入参数：vertex& from_vertex：要修改的顶点，
	int to：目标顶点编号；
	int time_cost：耗时；
	double fare_cost：费用；
	edge_type type：边类型
  返 回 值：空
  说    明：修改来源顶点的 first_edge，把新边插入邻接链表
***************************************************************************/
void add_edge(vertex& from_vertex, int to, int time_cost, double fare_cost, edge_type type) {
	edge_node* new_edge = new edge_node;

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

	edge_node edge_a_to_b;
	edge_a_to_b.to = 1;
	edge_a_to_b.time_cost = 3;
	edge_a_to_b.fare_cost = 0.3;
	edge_a_to_b.type = edge_type::METRO;
	station_a.first_edge = &edge_a_to_b;

	edge_node edge_b_to_a;
	edge_b_to_a.to = 0;
	edge_b_to_a.time_cost = 3;
	edge_b_to_a.fare_cost = 0.3;
	edge_b_to_a.type = edge_type::METRO;
	station_b.first_edge = &edge_b_to_a;

	edge_node edge_a_to_c;
	edge_a_to_c.to = 2;
	edge_a_to_c.time_cost = 6;
	edge_a_to_c.fare_cost = 0;//换乘边不额外收费
	edge_a_to_c.type = edge_type::TRANSFER;
	edge_a_to_c.next = station_a.first_edge;//头插法
	station_a.first_edge = &edge_a_to_c;
	


	if (station_a.first_edge == nullptr)
		cout << "该节点为孤立顶点，当前站点暂无可用路线" << endl;
	else {
		edge_node* current_edge = station_a.first_edge;//指针从站点第一个节点开始
		while (current_edge) {
			cout << current_edge->to << " ";
			current_edge = current_edge->next;
		}
	}
		


	return 0;
}
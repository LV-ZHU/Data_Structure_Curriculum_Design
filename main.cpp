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


int main()
{
	cout << "Transit Navigator started." << endl;
	vertex station_a;
	station_a.id = 0;
	station_a.name = "Tongji University";
	station_a.type = station_type::METRO;
	
	edge_node edge_a_to_b;
	edge_a_to_b.to = 1;
	edge_a_to_b.time_cost = 3;
	edge_a_to_b.fare_cost = 0.3;
	edge_a_to_b.type = edge_type::METRO;
	station_a.first_edge = &edge_a_to_b;
	if (station_a.first_edge == nullptr)
		cout << "该节点为孤立顶点，当前站点暂无可用路线" << endl;
	else
		cout << station_a.first_edge->to;//这里语法有点绕，通过指针访问它所指对象的成员使用箭头有点晕，是相当于(*station_a.first_edge).to的语法糖是吧


	return 0;
}
#include <iostream>
#include <string>
#include <climits>
#include <limits>
using namespace std;

#define test_mode 0
const int max_vertices = 100; //最多100个顶点

enum class edge_type { METRO, BUS, TRANSFER };//边类型：地铁，公交，换乘
enum class station_type{METRO, BUS};//站点类型:地铁，公交

//边结构体，E
struct edge_node {
	int to = INT_MAX;//目标顶点编号
	int time_cost = INT_MAX;//这条边记录的耗时
	double fare_cost = std::numeric_limits<double>::infinity();//费用
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
	int size = 0;//当前实际存有的堆元素数量，用来控制下标小于size防止读取时越界
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
	if (initial_capacity <= 0|| heap.data)//容量非正或data已经存放了地址
		return false;
	heap_node* p = new heap_node[initial_capacity];
	heap.data = p;//data字段指向动态堆数组的第一个元素
	heap.size = 0;
	heap.capacity = initial_capacity;
	return true;
}

/***************************************************************************
  函数名称：swap_heap_node
  功    能：交换两个堆结点，连带着顶点编号和当前距离两个结构体成员一起交换
  输入参数：heap_node& node1, heap_node& node2：需要交换的两个堆结点，交换后node1、node2所有成员互换
  返 回 值：无
  说    明：注意要同时交换结构体里的编号和距离确保依然匹配，由于结构体里只有普通成员，也可以直接交换整个结构体，可用自带swap
***************************************************************************/
void swap_heap_node(heap_node& node1, heap_node& node2)
{
	heap_node tmp;

	tmp.vertex_id = node1.vertex_id;
	tmp.distance = node1.distance;
	node1.vertex_id = node2.vertex_id;
	node1.distance = node2.distance;
	node2.vertex_id = tmp.vertex_id;
	node2.distance = tmp.distance;
}

/***************************************************************************
  函数名称：sift_up
  功    能：上浮单个位置的节点，和父节点进行大小关系判断
  输入参数：min_heap& heap：需要改变的堆，int index：需要判断是否交换的下标
  返 回 值：无
  说    明：index = father配合while语句可以实现逐层检查直至堆顶
***************************************************************************/
void sift_up(min_heap& heap, int index)
{
	while (index > 0) {
		int father = (index - 1) / 2;
		double father_distance = heap.data[father].distance;
		double index_distance = heap.data[index].distance;
		if (father_distance > index_distance) {
			swap_heap_node(heap.data[father], heap.data[index]);
			index = father;//如果交换，交换后将index赋值为父节点的值
		}
		else
			break;//该节点已经到了正确的位置上
	}
}

/***************************************************************************
  函数名称：expand_heap
  功    能：堆满时扩容至原来容量的两倍
  输入参数：min_heap& heap：需要扩容的堆
  返 回 值：false代表扩容失败，true代表成功
  说    明：Dijkstra中同一顶点可能因为距离更新而多次入堆，所以容量不一定等于顶点数；默认扩为两倍
***************************************************************************/
bool expand_heap(min_heap& heap)
{
	if (!heap.data || (heap.capacity <= 0))
		return false;
	heap_node* bigger_heap = new heap_node[heap.capacity * 2];
	for (int i = 0; i < heap.size; i++)
		bigger_heap[i] = heap.data[i];//复制，heap.size不变
	delete[] heap.data;//释放
	heap.data = bigger_heap;//data指向新堆
	heap.capacity *= 2;
	return true;
}

/***************************************************************************
  函数名称：insert_heap
  功    能：在堆中插入某个元素，加入sift_up调用包括重新对最小堆排序，堆满时会调用expand_heap扩容
  输入参数：min_heap& heap：需要插入的堆
   int vertex_id：插入顶点的编号
   double distance：插入顶点的当前距离
  返 回 值：false代表插入失败，true代表成功
  说    明：heap.size >= heap.capacity指没空余容量；!heap.data表明堆当前没有动态数组，通常因为initialize_heap初始化失败
***************************************************************************/
bool insert_heap(min_heap& heap, int vertex_id, double distance)
{
	if (!heap.data)
		return false;
	if( heap.size >= heap.capacity) {//其实只能=，所以只需要扩一次容即可，不需要while循环
		if (!expand_heap(heap))
			return false;//没return就完成了扩容
	}
	(heap.data + heap.size)->vertex_id = vertex_id;//等价于(*(heap.data + heap.size)).vertex_id或是heap.data[heap.size].vertex_id
	(heap.data + heap.size)->distance = distance;
	heap.size++;//插入完后实际数量加1
	sift_up(heap, heap.size - 1);//这里不和上一行换顺序是因为要把新元素正式纳入有效范围
	return true;
}

/***************************************************************************
  函数名称：sift_down
  功    能：下沉单个位置的节点，和子节点进行大小关系判断
  输入参数：min_heap& heap：需要改变的堆，int index：需要判断是否交换的下标
  返 回 值：无
  说    明：index = left_child/right_child;配合while语句可以实现逐层检查直至叶子节点
***************************************************************************/
void sift_down(min_heap& heap, int index)
{
	while (2 * index + 1 < heap.size) { //左孩子在下标0..(heap.size-1)范围内
		int left_child = 2 * index + 1;
		int right_child = 2 * index + 2;
		double left_child_distance = heap.data[left_child].distance;
		double index_distance = heap.data[index].distance;
		if (right_child >= heap.size) {//这个时候只有左孩子
			if (index_distance > left_child_distance) {
				swap_heap_node(heap.data[left_child], heap.data[index]);
				index = left_child;
			}
			break;//堆是完全二叉树，唯一的左孩子必然是最后一个元素也是叶子结点，所以处理完它之后无论是否交换，本轮下沉都已经结束，而不是else
		}
		double right_child_distance = heap.data[right_child].distance;//有右孩子才能拿对应下标
		if (index_distance < left_child_distance && index_distance < right_child_distance)//父最小，已到位
			break;
		else if (left_child_distance <= right_child_distance) {//左不超过右侧，父节点和较小的左换
			swap_heap_node(heap.data[left_child], heap.data[index]);
			index = left_child;
		}
		else {//换父和右
			swap_heap_node(heap.data[right_child], heap.data[index]);
			index = right_child;
		}
	}
}

/***************************************************************************
  函数名称：is_heap_empty
  功    能：检查堆是否是空堆
  输入参数：const min_heap &heap：需要检查的堆，不用修改，由于只传值会复制data指针、size和capacity故采用const限定
  返 回 值：true代表确实空，false代表并不空
  说    明：只根据size是否为0判断，不根据data判断，因为已经初始化但没有元素时，data不为空但堆仍然是空堆。
***************************************************************************/
bool is_heap_empty(const min_heap &heap)
{
	if (heap.size == 0)
		return true;
	else
		return false;
}

/***************************************************************************
  函数名称：extract_min
  功    能：弹出最小元素，需要1.删除堆顶，2.把原堆顶的heap_node交给调用者
  输入参数：min_heap& heap：需要弹出的堆
  heap_node &minimum_node：用于放原堆顶的heap_node
  返 回 值：false代表弹出失败，true代表成功
  说    明：
***************************************************************************/
bool extract_min(min_heap& heap, heap_node& minimum_node)
{
	if (is_heap_empty(heap))
		return false;
	minimum_node = heap.data[0];//存放堆顶元素
	heap.data[0] = heap.data[heap.size - 1];
	heap.size--;//位置-1
	sift_down(heap, 0);//从堆顶开始下沉

	return true;
}



/***************************************************************************
  函数名称：release_heap
  功    能：释放整个堆
  输入参数：min_heap& heap：需要释放的堆
  返 回 值：无
  说    明：先释放data对象动态内存申请的数组，然后重置结构体里各个成员的值
***************************************************************************/
void release_heap(min_heap& heap) 
{
	delete[] heap.data;
	heap.data = nullptr;
	heap.size = 0;
	heap.capacity = 0;
}

/***************************************************************************
  函数名称：calculate_weight
  功    能：按照W=time_cost+k×fare_cost计算权重
  输入参数：const edge_node& node：需要计算权重的边，double k：预设的倾向于费用还是倾向于时间的比例
  返 回 值：算式的结果
  说    明：由于不用修改node且为结构体，所以用常量引用
***************************************************************************/
double calculate_weight(const edge_node& node, double k) 
{
	return node.time_cost + k * node.fare_cost;
}

/***************************************************************************
  函数名称：output_dijkstra_arrays
  功    能：打印最新最短距离distance数组和previous_vertex前驱数组，仅用cout展现
  输入参数：const string& prompt：表明这是什么状态下的数组
   const double distance[]：最短距离数组
   const int previous_vertex[]：前驱数组，用来记录最短路径
   int vertex_number：需要输出多少个顶点
  返 回 值：空
  说    明：用cout打印数组各个元素，因为只用读所以传参用const，只有调试的时候需要用到这个函数记录变化状态
***************************************************************************/
void output_dijkstra_arrays(const string& prompt,const double distance[], const int previous_vertex[], int vertex_number)
{
	cout << prompt << endl << "distance: ";
	for (int i = 0; i < vertex_number; i++)
		cout << distance[i] << "  ";
	cout << endl;
	cout << "previous_vertex: ";
	for (int i = 0; i < vertex_number; i++)
		cout << previous_vertex[i] << "  ";
	cout << endl << endl;
}

/***************************************************************************
  函数名称：dijkstra
  功    能：完整的dijkstra流程
  输入参数：vertex vertices[]：顶点数组
   int vertex_number：顶点个数
   int start_vertex：起点顶点的下标
   int k：时间+k×费用里的策略参数k
   double distance[]：完成dijkstra流程时维护的最短路径长数组
   int previous_vertex[]：完成dijkstra流程时维护的前驱数组顶点，记录路径
  返 回 值：参数不在正确范围内或堆操作失败返回false，成功返回true
  说    明：distance和previous_vertex是结果数组,为需要修改的核心表格，函数内部会进行修改
	由于同一套顶点允许多次入堆，不采用visited数组写法bool visited[max_vertices] = {false}，
    而是判断堆里的distance元素是否已更新为distance结果数组里的最小值
***************************************************************************/
bool dijkstra(vertex vertices[], int vertex_number, int start_vertex, int k, double distance[], int previous_vertex[])
{
	//范围检查
	if (vertex_number <= 0 || vertex_number > max_vertices)
		return false;//顶点数应该在1..max_vertices之间
	if (start_vertex < 0 || start_vertex >= vertex_number)
		return false;//开始下标应该在0..vertex_number-1之间
	if (k < 0)
		return false;//Dijkstra对负权图无效
	//初始化值
	for (int i = 0; i < vertex_number; i++)
		distance[i] = std::numeric_limits<double>::infinity();
	distance[start_vertex] = 0;//从起点到起点距离显然为0，起点到其余为inf
	for (int i = 0; i < vertex_number; i++)
		previous_vertex[i] = -1;//初始化为-1，约定-1代表当前还没有前驱节点
	
	min_heap heap;
	if(!initialize_heap(heap, vertex_number))
		return false;
	if (!insert_heap(heap, start_vertex, distance[start_vertex])) {
		release_heap(heap);//插入失败，则释放
		return false;
	}
	while (!is_heap_empty(heap)) {
		heap_node current_node;
		if (!extract_min(heap, current_node)) {
			release_heap(heap);//拿出堆顶最小元素失败，则释放
			return false;
		}
		int current_vertex = current_node.vertex_id;
		if (current_node.distance > distance[current_vertex])
			continue;//如果弹出的堆顶距离比当前记录的距离大，说明这个顶点已经被更新过了，直接跳过旧堆，避免重复处理

		edge_node* current_edge = vertices[current_vertex].first_edge;//当前边指针指向当前顶点邻接表的第一条边
		while (current_edge) {//只要当前顶点还有邻接边，就一直遍历
			int next_vertex = current_edge->to;//当前邻接边的目标顶点编号

			double candidate_distance = distance[current_vertex] + calculate_weight(*current_edge, k);//计算当前顶点到邻接顶点的候选距离
			if (candidate_distance < distance[next_vertex]) {//如果候选距离比原来的距离小，就更新
				distance[next_vertex] = candidate_distance;//更新distance数组记录的距离
				previous_vertex[next_vertex] = current_vertex;//更新前驱数组记录的前驱顶点
				if (!insert_heap(heap, next_vertex, distance[next_vertex])){//把邻接顶点加入堆中，等待下一轮弹出
					release_heap(heap);//插入失败，则释放
					return false;
				}	
			}

			current_edge = current_edge->next;
		}
	}
	release_heap(heap);

	return true;
}


/***************************************************************************
  函数名称：output_paths
  功    能：
  输入参数：
  返 回 值：
  说    明：
***************************************************************************/



/***************************************************************************
  函数名称：
  功    能：
  输入参数：
  返 回 值：
  说    明：
***************************************************************************/
int main()
{
	vertex vertices[max_vertices];

	int vertex_number = 0;//当前实际顶点数量，初始还没加站点所以为0
	add_vertex(vertices, vertex_number, "Tongji University", station_type::METRO);
	add_vertex(vertices, vertex_number, "Siping Road", station_type::METRO);
	add_vertex(vertices, vertex_number, "Guokang Road Siping Road", station_type::BUS);

#if test_mode
	cout << vertex_number << endl;
#endif

	add_undirected_edge(vertices[0], vertices[1], 3, 0.3, edge_type::METRO);
	add_undirected_edge(vertices[0], vertices[2], 6, 0, edge_type::TRANSFER);

#if test_mode
	for (int i = 0; i < vertex_number; i++)
		output_one_step_vertex(vertices[i]);
#endif

	int start_vertex = 0;
	int k = 0;
	double distance[max_vertices];
	int previous_vertex[max_vertices];

	if (dijkstra(vertices, vertex_number, start_vertex, k, distance, previous_vertex))
		output_dijkstra_arrays("最终寻路结果：", distance, previous_vertex, vertex_number);
	else
		cout << "寻路失败" << endl;

	cout << "最短路径逆序为：" << endl;
	int end_vertex = 2;
	int current_vertex = end_vertex;
	while (current_vertex != start_vertex) {
		cout << current_vertex << " ";
		current_vertex = previous_vertex[current_vertex];
	}
	cout << start_vertex << endl;





#if test_mode
	min_heap test;
	initialize_heap(test, 2);
	insert_heap(test, 0, 2);
	insert_heap(test, 1, 5);
	insert_heap(test, 2, 3);
	cout << "测试扩容功能，当前size和capacity分别是：" << test.size << " " << test.capacity << endl;
	insert_heap(test, 3, 8);
	insert_heap(test, 4, 9);
	insert_heap(test, 5, 4);
	insert_heap(test, 6, 1);
	cout << "测试扩容功能，当前size和capacity分别是：" << test.size << " " << test.capacity << endl;
	while (!is_heap_empty(test)) {//只要还没空，就一直取元素，相比直接判断size在忘记extract_min会自动--的时候额外加了一行test.size--且初始size为奇数时不会死循环
		heap_node min;
		extract_min(test, min);
		cout << "本轮取出的堆顶元素序号和距离是：" << min.vertex_id << " " << min.distance << endl;
	}
	heap_node empty;
	cout << "测试空堆弹出是否正常，0是对的：" << extract_min(test, empty) << endl;
	release_heap(test);
#endif

#if test_mode
	edge_node cal;
	cal.time_cost = 6;
	cal.fare_cost = 0.5;
	cout << calculate_weight(cal, 0) << endl << calculate_weight(cal, 8) << endl;
#endif
	
	for (int i = 0; i < vertex_number; i++)
		release_edges(vertices[i]);

	return 0;
}
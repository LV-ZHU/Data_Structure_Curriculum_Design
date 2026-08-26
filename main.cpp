#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <climits>
#include <limits>
#include <iomanip>
#include <stdexcept>
#include <graphics.h>
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
enum class station_type{METRO, BUS};//站点类型:地铁，公交

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
//最小堆
struct min_heap {
	heap_node* data = nullptr;//当前指向元素位置
	int size = 0;//当前实际存有的堆元素数量，用来控制下标小于size防止读取时越界
	int capacity = 0;//当前数组最多能容纳多少个元素，应该始终满足0≤size≤capacity
};
//线路表
struct transit_line {
	int id = -1;//线路编号，等于线路在数组里的下标
	string name = "";//线路中文名称
	edge_type type= edge_type::METRO;//线路类型，默认为地铁，只允许是地铁/公交类型不允许为换乘
};
//EasyX界面状态
struct ui_state {
	int start_vertex = -1;
	int end_vertex = -1;
	int k = 0;
	bool allow_bike = false;

	int start_hour = 8;
	int start_minute = 30;

	double distance[max_vertices];
	int previous_vertex[max_vertices];
	const edge_node* previous_edge[max_vertices];
	int path[max_vertices];
	int path_vertex_number = 0;
	
	int total_time_cost = 0;
	double total_fare_cost = 0;
	int arrival_hour = 0;
	int arrival_minute = 0;
	int days_passed = 0;

	bool route_ready = false;
	string message = "请选择起点";
};
/***************************************************************************
  函数名称：add_transit_line
  功    能：添加一条线路
  输入参数：transit_line lines[]：线路数组
  int& line_number：当前线路数量
  string line_name：加入的线路名称
  edge_type line_type：加入的线路类型（公交/地铁）
  返 回 值：true代表加入成功，false代表加入失败
  说    明：当前线路数量需要自增，所以传递引用形式
***************************************************************************/
bool add_transit_line(transit_line lines[], int& line_number, string line_name, edge_type line_type)
{
	if (line_number >= max_lines)
		return false;
	if (line_type != edge_type::METRO && line_type != edge_type::BUS)
		return false;
	lines[line_number].id = line_number;//添加前lines里有0..line_number-1线路，现在在line_number位置添加变为0..line_number
	lines[line_number].name = line_name;
	lines[line_number].type = line_type;
	line_number++;
	return true;
}

/***************************************************************************
  函数名称：load_transit_lines
  功    能：从指定CSV文件读取全部线路数据，并存入线路数组；依次校验表头、字段、编号连续性和线路类型
  输入参数：const string& file_path：要读取的线路CSV文件路径
  transit_line lines[]：存放读取结果的线路数组；
  int& line_number：记录成功加入的线路数量
  返 回 值：true代表读取成功，false代表读取失败
  说    明：const string&避免无意义复制；line_number需要修改所以用引用
***************************************************************************/
bool load_transit_lines(const string& file_path, transit_line lines[], int &line_number)
{
	ifstream lines_file(file_path);
	if (!lines_file.is_open()) {
		cout << "线路CSV文件无法打开，无法进行后续计算" << endl;
		return false;
	}
	string header;
	if (!getline(lines_file, header)) {//把lines_file第一行内容拿出，放入header里，当前文件指针位于第二行开头
		cout << "首行信息读取失败" << endl;
		return false;
	}
	if (header.substr(0, 3) == "\xEF\xBB\xBF")
		header.erase(0, 3);//UTF8 BOM格式标识符是EF、BB、BF，如果是BOM格式去除前缀，非BOM格式的正常CSV则会跳过该if 
#if test_csv_lines
	cout << "首行header为：" << header << endl;
#endif
	if (header != "id,name,type") {
		cout << "该文件首行不是id,name,type，可能不是线路文件，请检查data/lines.csv 内容" << endl;
		return false;
	}

	string data_line;
	while (getline(lines_file, data_line)) {
#if test_csv_lines
		cout << "线路数据为：" << data_line << endl;
#endif
		string line_id_text;
		string line_name_text;
		string line_type_text;
		stringstream line_stream(data_line);
		if (!getline(line_stream, line_id_text, ',') || !getline(line_stream, line_name_text, ',')
			|| !getline(line_stream, line_type_text, ',')) {//逐个获取三个字段，每次遇到,就停止
			cout << "线路数据字段不完整" << endl;
			return false;
		}

		int line_id;
		try {
			line_id = stoi(line_id_text);
		}
		catch (const invalid_argument&) {//检查参数是否合法
			cout << "线路编号不是合法整数" << endl;
			return false;
		}
		catch (const out_of_range&) {//检查参数是否超过int范围
			cout << "线路编号超范围" << endl;
			return false;
		}
		edge_type line_type;
		if (line_type_text == "METRO")
			line_type = edge_type::METRO;
		else if (line_type_text == "BUS")
			line_type = edge_type::BUS;
		else {
			cout << "线路类型既不是地铁也不是公交，非法" << endl;
			return false;
		}

#if test_csv_lines
		cout << "线路id为：" << line_id << endl;
		cout << "线路名称为：" << line_name_text << endl;
		cout << "线路类型为：" << line_type_text << endl;

#endif	

		if (line_number == line_id) {//校验当前线路编号和CSV文件里读到的是否一致
			if (add_transit_line(lines, line_number, line_name_text, line_type)) {
#if test_csv_lines
				cout << lines[line_id].id << " " << lines[line_id].name << endl;
#endif
			}
			else {
				cout << "线路初始化失败" << endl;
				return false;
			}
		}
		else {
			cout << "线路编号不连续或顺序错误" << endl;
			return false;
		}
	}
	if (line_number == 0) {//只有表头没有具体线路
		cout << "线路CSV中没有有效线路数据" << endl;
		return false;
	}
	lines_file.close();//关文件

	return true;
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
void add_directed_edge(vertex& from_vertex, int to, int time_cost, double fare_cost, edge_type type, int line_id = -1,int bike_time_cost = -1)
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
void add_undirected_edge(vertex& first_vertex, vertex& second_vertex, int time_cost, double fare_cost, edge_type type, int line_id = -1,int bike_time_cost = -1)
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
  int x：站点圆心x坐标
  int y：站点圆心y坐标
  返 回 值：true(1)为正常，false(0)为已经放满
  说    明：先添加，再自增，从0开始
***************************************************************************/
bool add_vertex(vertex vertices[max_vertices], int& vertex_number, string vertex_name, station_type type, int x, int y)
{
	if (vertex_number >= max_vertices)
		return false;//超额，添加失败
	else {
		vertices[vertex_number].id = vertex_number;//id即为当前号码
		vertices[vertex_number].name = vertex_name;
		vertices[vertex_number].type= type;
		vertices[vertex_number].x = x;
		vertices[vertex_number].y = y;
		vertex_number++;//全添加完再自增
		return true;
	}	
}

/***************************************************************************
  函数名称：load_vertices
  功    能：从指定CSV文件读取全部站点数据
  输入参数：const string& file_path：要读取的站点CSV文件路径
  vertex vertices[]：存放读取结果的站点数组
  int& vertex_number：记录成功加入的站点数量
  返 回 值：true代表读取成功，false代表读取失败
  说    明：
***************************************************************************/
bool load_vertices(const string& file_path, vertex vertices[], int& vertex_number)
{
	ifstream stations_file(file_path);
	if (!stations_file.is_open()) {
		cout << "站点CSV文件无法打开，无法进行后续计算" << endl;
		return false;
	}
	string header;	
	if (!getline(stations_file, header)) {//把stations_file第一行内容拿出，放入header里，当前文件指针位于第二行开头
		cout << "首行信息读取失败" << endl;
		return false;
	}
	if (header.substr(0, 3) == "\xEF\xBB\xBF")
		header.erase(0, 3);//UTF8 BOM格式标识符是EF、BB、BF，如果是BOM格式去除前缀，非BOM格式的正常CSV则会跳过该if 
#if test_csv_lines
	cout << "首行header为：" << header << endl;
#endif
	if (header != "id,name,type,x,y") {
		cout << "该文件首行不是id,name,type，可能不是站点文件，请检查data/stations.csv 内容" << endl;
		return false;
	}

	string data_line;
	while (getline(stations_file, data_line)) {
#if test_csv_lines
		cout << "站点数据为：" << data_line << endl;
#endif
		string station_id_text;
		string station_name_text;
		string station_type_text;
		string station_x_text;
		string station_y_text;
		stringstream station_stream(data_line);
		if (!getline(station_stream, station_id_text, ',') 
			|| !getline(station_stream, station_name_text, ',')
			|| !getline(station_stream, station_type_text, ',')
			|| !getline(station_stream, station_x_text, ',')
			|| !getline(station_stream, station_y_text, ',')
			) {//逐个获取五个字段，每次遇到,就停止
			cout << "站点数据字段不完整" << endl;
			return false;
		}

		int station_id;
		int station_x;
		int station_y;
		try {
			station_id = stoi(station_id_text);
			station_x = stoi(station_x_text);
			station_y = stoi(station_y_text);
		}
		catch (const invalid_argument&) {//检查参数是否合法
			cout << "站点编号/坐标不是合法整数" << endl;
			return false;
		}
		catch (const out_of_range&) {//检查参数是否超过int范围
			cout << "站点编号/坐标超范围" << endl;
			return false;
		}
		station_type parsed_station_type;
		if (station_type_text == "METRO")
			parsed_station_type = station_type::METRO;
		else if (station_type_text == "BUS")
			parsed_station_type = station_type::BUS;
		else {//一般不会执行到这里
			cout << "站点类型既不是地铁也不是公交，不在枚举类型里，非法" << endl;
			return false;
		}

#if test_csv_lines
		cout << "站点id为：" << station_id << endl;
		cout << "站点名称为：" << station_name_text << endl;
		cout << "站点类型为：" << station_type_text << endl;
		cout << "站点x坐标为：" << station_x_text << endl;
		cout << "站点y坐标为：" << station_y_text << endl;
#endif	

		if (vertex_number == station_id) {//校验当前站点编号和CSV文件里读到的是否一致
			if (add_vertex(vertices, vertex_number, station_name_text, parsed_station_type, station_x, station_y)) {
#if test_csv_lines
				cout << vertices[station_id].id << " " << vertices[station_id].name << endl;
#endif
			}
			else {
				cout << "站点初始化失败" << endl;
				return false;
			}
		}
		else {
			cout << "站点编号不连续或顺序错误" << endl;
			return false;
		}
	}
	if (vertex_number == 0) {//只有表头没有具体站点
		cout << "站点CSV中没有有效站点数据" << endl;
		return false;
	}
	stations_file.close();//关文件

	return true;
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
  函数名称：get_effective_time_cost
  功    能：从time_cost和bike_time_cost里选一个合理值
  输入参数：const edge_node& node：需要判断的边，bool allow_bike：是否允许骑车来代替获得更短的time_cost
  返 回 值：最终选定的time_cost
  说    明：如果允许骑车；类型为换乘边；骑车用时合理同时满足time_cost为骑行用时
***************************************************************************/
int get_effective_time_cost(const edge_node& node, bool allow_bike = false)
{
	if (allow_bike && node.type == edge_type::TRANSFER && node.bike_time_cost >= 0 && node.bike_time_cost < node.time_cost)
		return node.bike_time_cost;
	else
		return node.time_cost;
}

/***************************************************************************
  函数名称：calculate_weight
  功    能：按照W=time_cost+k×fare_cost计算权重
  输入参数：const edge_node& node：需要计算权重的边，double k：预设的倾向于费用还是倾向于时间的比例，bool allow_bike：是否允许骑车来代替获得更短的time_cost
  返 回 值：算式的结果
  说    明：由于不用修改node且为结构体，所以用常量引用
***************************************************************************/
double calculate_weight(const edge_node & node, double k, bool allow_bike = false)
{
	return get_effective_time_cost(node, allow_bike) + k * node.fare_cost;
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
  函数名称：is_valid_vertex_id
  功    能：检查vertex_id是否在0..vertex_number-1之间
  输入参数：const int vertex_id：需要检查的顶点下标，const int vertex_number：总顶点数
  返 回 值：true表示在范围内，false表示不在范围内
  说    明：很多函数需要检查vertex_id是否在正确的数组下标范围内，可调用该函数
***************************************************************************/
bool is_valid_vertex_id(const int vertex_id,const int vertex_number)
{
	if (vertex_id >= 0 && vertex_id < vertex_number)
		return true;//开始下标应该在0..vertex_number-1之间，数组下标
	else
		return false;
}

/***************************************************************************
  函数名称：load_edges
  功    能：从指定CSV文件读取全部边数据
  输入参数：const string& file_path：要读取的边CSV文件路径
  vertex vertices[]：需要修改邻接表的顶点数组
  int vertex_number：检查两个端点编号
  const transit_line lines[]：线路数组
  int line_number：检查 line_id 范围
  返 回 值：true代表读取成功，false代表读取失败
  说    明：此处vertex_number转非引用是因为不再需要修改，仅用于检查范围；一行CSV表示一条无向边，函数内部调用add_undirected_edge创建两个方向
***************************************************************************/
bool load_edges(const string& file_path, vertex vertices[], int vertex_number, const transit_line lines[], int line_number)
{
	ifstream edges_file(file_path);
	if (!edges_file.is_open()) {
		cout << "边CSV文件无法打开，无法进行后续计算" << endl;
		return false;
	}
	string header;
	if (!getline(edges_file, header)) {//把edges_file第一行内容拿出，放入header里，当前文件指针位于第二行开头
		cout << "首行信息读取失败" << endl;
		return false;
	}
	if (header.substr(0, 3) == "\xEF\xBB\xBF")
		header.erase(0, 3);//UTF8 BOM格式标识符是EF、BB、BF，如果是BOM格式去除前缀，非BOM格式的正常CSV则会跳过该if 
#if test_csv_lines
	cout << "首行header为：" << header << endl;
#endif
	if (header != "first_vertex_id,second_vertex_id,time_cost,fare_cost,type,line_id,bike_time_cost") {
		cout << "该文件首行不是first_vertex_id,second_vertex_id,time_cost,fare_cost,type,line_id,bike_time_cost. 很可能不是边文件，请检查data/edges.csv 内容" << endl;
		return false;
	}

	int current_line_number = 1;//记录为
	string data_line;
	while (getline(edges_file, data_line)) {

#if test_csv_lines
		cout << "边数据为：" << data_line << endl;
#endif

		string first_vertex_id_text;
		string second_vertex_id_text;
		string time_cost_text;
		string fare_cost_text;
		string edge_type_text;
		string line_id_text;
		string bike_time_cost_text;
		stringstream edges_stream(data_line);
		if (!getline(edges_stream, first_vertex_id_text, ',') ||
			!getline(edges_stream, second_vertex_id_text, ',') ||
			!getline(edges_stream, time_cost_text, ',') ||
			!getline(edges_stream, fare_cost_text, ',') ||
			!getline(edges_stream, edge_type_text, ',') ||
			!getline(edges_stream, line_id_text, ',') ||
			!getline(edges_stream, bike_time_cost_text, ',')) {//逐个获取七个字段，每次遇到,就停止
			cout << "边数据字段不完整" << endl;
			return false;
		}
		int first_vertex_id;
		int second_vertex_id;
		int time_cost;
		double fare_cost;
		int line_id;
		int bike_time_cost;
		try {
			first_vertex_id = stoi(first_vertex_id_text);
			second_vertex_id = stoi(second_vertex_id_text);
			time_cost = stoi(time_cost_text);
			fare_cost = stod(fare_cost_text);//注意，fare_cost是double型所以用stod
			line_id = stoi(line_id_text);
			bike_time_cost = stoi(bike_time_cost_text);
		}
		catch (const invalid_argument&) {
			cout << "第" << current_line_number << "条边" << "某个数字字段非数字类型" << endl;
			return false;
		}
		catch (const out_of_range&) {
			cout << "第" << current_line_number << "条边" << "某个数字字段超过合法范围" << endl;
			return false;
		}
		edge_type parsed_edge_type;
		if (edge_type_text == "METRO")
			parsed_edge_type = edge_type::METRO;
		else if (edge_type_text == "BUS")
			parsed_edge_type = edge_type::BUS;
		else if (edge_type_text == "TRANSFER")
			parsed_edge_type = edge_type::TRANSFER;
		else {//一般不会执行到这里
			cout << "第" << current_line_number << "条边" << "边类型既不是地铁也不是公交也不是换乘，非法" << endl;
			return false;
		}

		if (!is_valid_vertex_id(first_vertex_id, vertex_number) ||
			!is_valid_vertex_id(second_vertex_id, vertex_number)) {//顶点编号范围
			cout << "第" << current_line_number << "条边" << "站点编号范围不合法" << endl;
			return false;
		}
		if (first_vertex_id == second_vertex_id) {//检查起点终点编号
			cout << "第" << current_line_number << "条边" << "站点起点终点编号相同，形成错误自环" << endl;
			return false;
		}
		if (time_cost < 0) {//注意，0也不行，因为要处理公交站问题
			cout << "第" << current_line_number << "条边" << "时间小于等于0，数据错误的权重不能用Dijkstra" << endl;
			return false;
		}
		if (fare_cost < 0) {
			cout << "第" << current_line_number << "条边" << "费用小于0，数据错误的权重不能用Dijkstra" << endl;
			return false;
		} 
		if (parsed_edge_type == edge_type::METRO || parsed_edge_type == edge_type::BUS) {
			if (line_id < 0 || line_id >= line_number) {
				cout << "第" << current_line_number << "条边" << "公交/地铁线路的编号范围错误" << endl;
				return false;
			}
			if (lines[line_id].type!= parsed_edge_type) {
				cout << "第" << current_line_number << "条边" << "线路类型和CSV类型不匹配" << endl;
				return false;
			}
			if (bike_time_cost != -1) {
				cout << "第" << current_line_number << "条边" << "线路类型为地铁或公交，但骑行边非-1，是否输入了错误的类型？" << endl;
				return false;
			}
			switch (parsed_edge_type) {
				case edge_type::METRO:
					if (vertices[first_vertex_id].type != station_type::METRO || vertices[second_vertex_id].type != station_type::METRO) {
						cout << "第" << current_line_number << "条地铁边的两端不全是地铁站，类型错误" << endl;
						return false;
					}
					break;
				case edge_type::BUS:
					if (vertices[first_vertex_id].type != station_type::BUS || vertices[second_vertex_id].type != station_type::BUS) {
						cout << "第" << current_line_number << "条公交边的两端不全是公交站，类型错误" << endl;
						return false;
					}
					break;
			}
		}
		else if (parsed_edge_type == edge_type::TRANSFER) {
			if (line_id != -1) {
				cout << "第" << current_line_number << "条边" << "线路类型为换乘，但线路编号非-1，是否输入了错误的类型？" << endl;
				return false;
			}
			if (fare_cost != 0) {
				cout << "第" << current_line_number << "条边" << "线路类型为换乘，但费用不为0，是否输入了错误的类型？" << endl;
				return false;
			}
			if (bike_time_cost != -1 && (bike_time_cost <= 0 || bike_time_cost >= time_cost)){
				cout << "第" << current_line_number << "条边" << "是换乘边，允许骑行，但是骑行耗时不合法(非正值或并未比步行节省时间)" << endl;
				return false;
			}
		}
		
			
		
		

	

#if test_csv_lines
		cout << "边的一端顶点编号为：" << first_vertex_id << endl;
		cout << "边的另一端顶点编号为：" << second_vertex_id << endl;
		cout << "边的时间花费为：" << time_cost << endl;
		cout << "边的费用为：" << fare_cost << endl;
		cout << "边的类型为：" << edge_type_text << endl;
		cout << "线路编号为：" << line_id << endl;
		cout << "边骑行时长为：" << bike_time_cost << endl;
#endif	



		add_undirected_edge(vertices[first_vertex_id], vertices[second_vertex_id],
			time_cost, fare_cost, parsed_edge_type, line_id, bike_time_cost);
		current_line_number++;
	}

	if (current_line_number == 1) {//依然停留在1，表示只有表头没有具体边
		cout << "边CSV中没有有效边数据" << endl;
		return false;
	}
	edges_file.close();//关文件

	return true;
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
	const edge_node* previous_edge[]：保存最短路径中到达各顶点实际采用的边
   bool allow_bike = false：是否允许骑车，默认不允许
  返 回 值：参数不在正确范围内或堆操作失败返回false，成功返回true
  说    明：distance和previous_vertex是结果数组,为需要修改的核心表格，函数内部会进行修改
	由于同一套顶点允许多次入堆，不采用visited数组写法bool visited[max_vertices] = {false}，
	而是判断堆里的distance元素是否已更新为distance结果数组里的最小值
***************************************************************************/
bool dijkstra(vertex vertices[], int vertex_number, int start_vertex, int k, double distance[], int previous_vertex[],
	const edge_node* previous_edge[], bool allow_bike = false)
{
	//范围检查
	if (vertex_number <= 0 || vertex_number > max_vertices)
		return false;//顶点数应该在1..max_vertices之间
	if (!is_valid_vertex_id(start_vertex, vertex_number))
		return false;//开始下标应该在0..vertex_number-1之间
	if (k < 0)
		return false;//Dijkstra对负权图无效
	//初始化值
	for (int i = 0; i < vertex_number; i++)
		distance[i] = std::numeric_limits<double>::infinity();
	distance[start_vertex] = 0;//从起点到起点距离显然为0，起点到其余为inf
	for (int i = 0; i < vertex_number; i++) {
		previous_vertex[i] = -1;//初始化为-1，约定-1代表当前还没有前驱节点
		previous_edge[i] = nullptr;
	}
		

	min_heap heap;
	if (!initialize_heap(heap, vertex_number))
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

			double candidate_distance = distance[current_vertex] + calculate_weight(*current_edge, k, allow_bike);//计算当前顶点到邻接顶点的候选距离
			if (candidate_distance < distance[next_vertex]) {//如果候选距离比原来的距离小，就更新
				distance[next_vertex] = candidate_distance;//更新distance数组记录的距离
				previous_vertex[next_vertex] = current_vertex;//更新前驱数组记录的前驱顶点
				previous_edge[next_vertex] = current_edge;
				if (!insert_heap(heap, next_vertex, distance[next_vertex])) {//把邻接顶点加入堆中，等待下一轮弹出
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
  函数名称：build_paths
  功    能：建立paths数组，存储最短路径整个过程经过哪些顶点
  输入参数：const int previous_vertex[]：前驱数组
	int vertex_number：总顶点数，用于范围检查和防止异常循环
	int start_vertex：起点顶点序号；
	int end_vertex：终点顶点序号；
	int path[]：输出路径数组；
	int& path_vertex_number：路径总长度，确定数组边界
  返 回 值：false表示失败，true表示成功
  说    明：path[]为反向数组
***************************************************************************/
bool build_paths(const int previous_vertex[], int vertex_number, int start_vertex, int end_vertex, int path[], int& path_vertex_number)
{
	//范围检查
	if (vertex_number <= 0 || vertex_number > max_vertices)
		return false;//顶点数应该在1..max_vertices之间
	if (!(is_valid_vertex_id(start_vertex, vertex_number) && is_valid_vertex_id(end_vertex, vertex_number)))
		return false;//起点、终点下标都应该在0..vertex_number-1之间

	path_vertex_number = 0;//不依赖传入的值，重置为0，因为用的引用所以无法设置函数默认参数
	int current_vertex = end_vertex;
	while (path_vertex_number < vertex_number) {//若大于等于vertex_number根据鸽巢原理一定有重复顶点出现，则有环
		if (!is_valid_vertex_id(current_vertex, vertex_number)) {//不可达，正常情况下这种情况current_vertex为-1
			path_vertex_number = 0;
			return false;
		}
		path[path_vertex_number] = current_vertex;
		path_vertex_number++;
		if (current_vertex == start_vertex)
			return true;//找到起点则返回true，注意此处等号左边不能用path[path_vertex_number]，因为path_vertex_number已经自增

		current_vertex = previous_vertex[current_vertex];//更新current_vertex为它的前驱节点

	}
	//不满足path_vertex_number<vertex_number，说明不正常
	path_vertex_number = 0;
	return false;
}

/***************************************************************************
  函数名称：find_directed_edge
  功    能：找有向边
  输入参数：const vertex & from_vertex：起点顶点，int to：目标顶点编号
  返 回 值：const edge_node*，返回找到的边
  说    明：打印时需要具体路径
***************************************************************************/
const edge_node* find_directed_edge(const vertex& from_vertex, int to)
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
  函数名称：output_route_guide
  功    能：打印路径，目前output命名的先只cout呈现
  输入参数：const vertex vertices[]：顶点数组
  const int path[]：输出路径数组
  const int path_vertex_number：路径总长度
  const edge_node* previous_edge[]：Dijkstra实际选中的前驱边数组
  bool allow_bike：允许骑行
  const transit_line lines[]：线路数组
  const int line_number：线路数量
  返 回 值：找不到过程边返回false，路线正确返回true
  说    明：path[]为反向数组，故反向遍历打印
***************************************************************************/
bool output_route_guide(const vertex vertices[],
	const int path[],
	const int path_vertex_number,
	const edge_node* previous_edge[],
	bool allow_bike,
	const transit_line lines[],
	const int line_number)
{
	cout << "推荐路线：";
	for (int i = path_vertex_number - 1; i >= 0; i--)
		cout << vertices[path[i]].name << (i ? " -> " : "");//i为0的时候说明到终点了，不输出分隔符，其余时间->连接
	cout << endl;

	cout << "详细路线：" << endl;
	int segment_number = 1;
	while (segment_number < path_vertex_number) {
		int start_position = path_vertex_number - segment_number;
		int from_vertex_id = path[start_position];//起点顶点编号
		int end_position = start_position - 1;
		int to_vertex_id = path[end_position];//终点顶点编号
		const edge_node* current_edge = previous_edge[to_vertex_id];
		if (!current_edge) {
			cout << "路线数据错误" << endl;
			return false;
		}
		int time_cost = get_effective_time_cost(*current_edge, allow_bike);
		double fare_cost = current_edge->fare_cost;//注意fare_cost类型为double
		string type_way_in_chinese = "";
		switch (current_edge->type)
		{
			case edge_type::METRO:
			case edge_type::BUS:
				if (current_edge->line_id < 0 || current_edge->line_id >= line_number) {//只有地铁/公交才检查范围
					cout << "路线编号范围错误" << endl;
					return false;
				}
				type_way_in_chinese = "乘坐" + lines[current_edge->line_id].name;
				break;
			case edge_type::TRANSFER:
				if (time_cost < current_edge->time_cost)
					type_way_in_chinese = "骑行";
				else //正常情况下此处time_cost == current_edge->time_cost
					type_way_in_chinese = "步行";
				break;
		}

		cout << "第" << segment_number << "段：从" << vertices[from_vertex_id].name << type_way_in_chinese << "前往"
			<< vertices[to_vertex_id].name << "，耗时" << time_cost << "分钟，费用" << fare_cost << "元。" << endl;
		segment_number++;
	}
	return true;
}


/***************************************************************************
  函数名称：calculate_path_statistics
  功    能：统计路径总时间、总费用，结果分别存放在引用变量int& total_time_cost和double& total_fare_cost中
  输入参数：const vertex vertices[]：顶点数组
  const int path[]：路径倒序数组
  int path_vertex_number：路径数量
  int& total_time_cost：存放结果的总耗时
  double& total_fare_cost：存放结果的总费用
  bool allow_bike = false：是否允许骑行，默认false
  返 回 值：成功为true，找不到路径中的边则为false
  说    明：find_directed_edge返回边指针；调用get_effective_time_cost函数时需要解引用该指针
***************************************************************************/
/***************************************************************************
  函数名称：calculate_path_statistics
  功    能：根据Dijkstra实际采用的前驱边统计路径总时间和总费用
  输入参数：const int path[]：路径倒序数组
  int path_vertex_number：路径中的顶点数量
  const edge_node* previous_edge[]：Dijkstra保存的各顶点实际前驱边
  int& total_time_cost：存放总耗时
  double& total_fare_cost：存放总费用
  bool allow_bike：是否允许骑行，默认false
  返 回 值：统计成功返回true，路径中的前驱边无效返回false
  说    明：path[]按照终点到起点倒序保存；对于相邻的path[i]和path[i-1]，
  到达path[i-1]所实际采用的边就是previous_edge[path[i-1]]
***************************************************************************/
bool calculate_path_statistics(const int path[],
	int path_vertex_number,
	const edge_node* previous_edge[],
	int& total_time_cost,
	double& total_fare_cost,
	bool allow_bike = false)
{
	total_time_cost = 0;
	total_fare_cost = 0;

	for (int i = path_vertex_number - 1; i > 0; i--) {
		int to_vertex = path[i - 1];
		const edge_node* current_edge = previous_edge[to_vertex];
		if (!current_edge)
			return false;
		total_time_cost +=
			get_effective_time_cost(*current_edge, allow_bike);
		total_fare_cost += current_edge->fare_cost;
	}

	return true;
}

/***************************************************************************
  函数名称：calculate_arrival_time
  功    能：根据开始时间和路程时间计算到达时间
  输入参数：int start_hour：出发小时（0-23）
	int start_minute：出发分钟（0-59）
	int total_minutes：路线总分钟数（>=0）
	int& arrival_hour：到达小时
	int& arrival_minute：到达分钟
	int& days_passed：跨过的天数
  返 回 值：参数符合范围为true，不符合范围则为false
  说    明：后三个参数为输出参数，故使用引用
***************************************************************************/
bool calculate_arrival_time(int start_hour, int start_minute, int total_minutes,
	int& arrival_hour, int& arrival_minute, int& days_passed)
{
	if (start_hour < 0 || start_hour>23 || start_minute < 0 || start_minute>59 || total_minutes < 0)
		return false;
	int total_arrival_minutes = start_hour * 60 + start_minute + total_minutes;
	days_passed = total_arrival_minutes / 1440;
	int remain_minutes_in_one_day = total_arrival_minutes % 1440;
	arrival_hour = remain_minutes_in_one_day / 60;
	arrival_minute = remain_minutes_in_one_day % 60;
	return true;
}

/***************************************************************************
  函数名称：draw_all_edges
  功    能：在EasyX窗口中绘制当前邻接表里的全部无向边
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  返 回 值：无
  说    明：无向边在邻接表中保存两个方向，只在当前顶点编号小于目标顶点编号时绘制，避免重复绘制
***************************************************************************/
void draw_all_edges(const vertex vertices[], int vertex_number)
{
	for (int i = 0; i < vertex_number; i++) {
		const edge_node* current_edge = vertices[i].first_edge;
		while (current_edge) {
			if (i < current_edge->to) {//无向边只处理编号较小的一端，防止重复处理
				if (current_edge->type == edge_type::TRANSFER) {
					setlinecolor(RGB(135, 145, 153));
					setlinestyle(PS_DASH, 1);
				}
				else if (current_edge->type == edge_type::BUS) {
					if (current_edge->line_id == 10 || current_edge->line_id == 11)
						setlinecolor(RGB(47, 124, 89));
					else
						setlinecolor(RGB(33, 118, 163));
					setlinestyle(PS_SOLID, 2);
				}
				else {
					switch (current_edge->line_id) {
						case 0:
							setlinecolor(RGB(111, 75, 126));
							break;
						case 1:
							setlinecolor(RGB(124, 74, 43));
							break;
						case 2:
							setlinecolor(RGB(123, 115, 15));
							break;
						case 3:
							setlinecolor(RGB(177, 36, 50));
							break;
						default:
							setlinecolor(RGB(70, 100, 150));
							break;
					}
					setlinestyle(PS_SOLID, 3);
				}

				line(vertices[i].x, vertices[i].y,
					vertices[current_edge->to].x, vertices[current_edge->to].y);
			}
			current_edge = current_edge->next;
		}
	}
	setlinestyle(PS_SOLID, 1);
}

/***************************************************************************
  函数名称：draw_all_vertices
  功    能：在EasyX窗口中绘制全部站点圆圈
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  返 回 值：无
  说    明：地铁站使用蓝色边框，公交/地点节点使用橙色边框；本阶段暂不绘制站名
***************************************************************************/
void draw_all_vertices(const vertex vertices[], int vertex_number)
{
	setlinestyle(PS_SOLID, 1);
	setfillcolor(RGB(255, 255, 255));

	int draw_vertex_number = vertex_number;

	if (draw_vertex_number > physical_vertex_number)
		draw_vertex_number = physical_vertex_number;

	for (int i = 0; i < draw_vertex_number; i++) {
		if (vertices[i].type == station_type::METRO)
			setlinecolor(RGB(40, 120, 212));
		else
			setlinecolor(RGB(220, 122, 36));

		fillcircle(vertices[i].x, vertices[i].y, 5);
	}
}

/***************************************************************************
  函数名称：find_clicked_vertex
  功    能：根据鼠标点击位置判断是否选中了某个站点
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  int mouse_x：鼠标点击位置的x坐标
  int mouse_y：鼠标点击位置的y坐标
  返 回 值：命中站点返回对应顶点下标，未命中任何站点返回-1
  说    明：站点显示半径为5，点击判定半径使用10以提高鼠标操作容错性
***************************************************************************/
int find_clicked_vertex(const vertex vertices[], int vertex_number,
	int mouse_x, int mouse_y)
{
	const int click_radius = 10;
	int clickable_vertex_number = vertex_number;

	if (clickable_vertex_number > physical_vertex_number)
		clickable_vertex_number = physical_vertex_number;

	for (int i = 0; i < clickable_vertex_number; i++) {
		int dx = mouse_x - vertices[i].x;
		int dy = mouse_y - vertices[i].y;

		if (dx * dx + dy * dy <= click_radius * click_radius)
			return i;
	}

	return -1;
}

/***************************************************************************
  函数名称：is_point_in_rectangle
  功    能：判断指定坐标是否位于一个矩形区域内部
  输入参数：int point_x：需要判断的点的x坐标
  int point_y：需要判断的点的y坐标
  int left：矩形左边界
  int top：矩形上边界
  int right：矩形右边界
  int bottom：矩形下边界
  返 回 值：位于矩形内部返回true，否则返回false
  说    明：EasyX屏幕坐标系原点位于左上角，x向右增大，y向下增大
***************************************************************************/
bool is_point_in_rectangle(int point_x, int point_y,
	int left, int top, int right, int bottom)
{
	return point_x >= left && point_x <= right
		&& point_y >= top && point_y <= bottom;
}

/***************************************************************************
  函数名称：draw_selected_vertices
  功    能：在普通站点上方额外绘制当前选中的起点和终点标记
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  const ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：起点使用绿色圆形，终点使用红色圆形；只有编号合法时才绘制
***************************************************************************/
void draw_selected_vertices(const vertex vertices[], int vertex_number,
	const ui_state& state)
{
	if (is_valid_vertex_id(state.start_vertex, vertex_number)) {
		setlinecolor(RGB(20, 110, 60));
		setfillcolor(RGB(47, 191, 113));
		fillcircle(vertices[state.start_vertex].x,
			vertices[state.start_vertex].y, 8);
	}

	if (is_valid_vertex_id(state.end_vertex, vertex_number)) {
		setlinecolor(RGB(140, 35, 35));
		setfillcolor(RGB(239, 90, 90));
		fillcircle(vertices[state.end_vertex].x,
			vertices[state.end_vertex].y, 8);
	}
}

/***************************************************************************
  函数名称：draw_route_highlight
  功    能：根据当前规划出的path数组绘制黄色推荐路线
  输入参数：const vertex vertices[]：顶点数组
  const ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：path数组按照终点到起点倒序存储，因此绘制时从数组末尾向前遍历
***************************************************************************/
void draw_route_highlight(const vertex vertices[], const ui_state& state)
{
	if (!state.route_ready || state.path_vertex_number < 2)
		return;

	setlinecolor(RGB(255, 210, 40));
	setlinestyle(PS_SOLID, 7);

	for (int i = state.path_vertex_number - 1; i > 0; i--) {
		int from_vertex = state.path[i];
		int to_vertex = state.path[i - 1];
		bool from_is_virtual = from_vertex >= physical_vertex_number;
		bool to_is_virtual = to_vertex >= physical_vertex_number;

		//物理节点与对应公交状态节点之间只是上下车计费边，不作为乘车指南的一段显示
		if (from_is_virtual != to_is_virtual)
			continue;

		line(vertices[from_vertex].x, vertices[from_vertex].y,
			vertices[to_vertex].x, vertices[to_vertex].y);
	}

	setlinestyle(PS_SOLID, 1);
}

/***************************************************************************
  函数名称：draw_control_panel
  功    能：绘制EasyX窗口右侧控制区域，包括起终点、时间、策略、骑行和操作按钮
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  const ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：右侧面板范围约为x=900~1199；按钮坐标需要和handle_left_click中的判断保持一致
***************************************************************************/
void draw_control_panel(const vertex vertices[], int vertex_number,
	const ui_state& state)
{
	setbkmode(TRANSPARENT);
	settextcolor(RGB(35, 45, 60));
	settextstyle(18, 0, "微软雅黑");

	outtextxy(920, 20, "城市多模态交通导航");

	outtextxy(920, 60, "起点：");
	if (is_valid_vertex_id(state.start_vertex, vertex_number))
		outtextxy(980, 60, vertices[state.start_vertex].name.c_str());
	else
		outtextxy(980, 60, "未选择");

	outtextxy(920, 90, "终点：");
	if (is_valid_vertex_id(state.end_vertex, vertex_number))
		outtextxy(980, 90, vertices[state.end_vertex].name.c_str());
	else
		outtextxy(980, 90, "未选择");

	outtextxy(920, 135, "出发时间");

	setlinecolor(RGB(150, 160, 170));

	rectangle(930, 165, 980, 200);
	outtextxy(947, 172, "-5");

	stringstream time_stream;
	time_stream << setw(2) << setfill('0') << state.start_hour
		<< ":" << setw(2) << state.start_minute;
	string time_text = time_stream.str();
	outtextxy(1015, 172, time_text.c_str());

	rectangle(1110, 165, 1160, 200);
	outtextxy(1127, 172, "+5");

	outtextxy(920, 230, "路线策略");

	if (state.k == 0)
		setfillcolor(RGB(210, 232, 250));
	else
		setfillcolor(RGB(245, 247, 250));

	solidrectangle(930, 260, 1045, 295);
	setlinecolor(RGB(120, 140, 160));
	rectangle(930, 260, 1045, 295);
	outtextxy(943, 267, "时间最短");

	if (state.k == 8)
		setfillcolor(RGB(210, 232, 250));
	else
		setfillcolor(RGB(245, 247, 250));

	solidrectangle(1055, 260, 1170, 295);
	rectangle(1055, 260, 1170, 295);
	outtextxy(1068, 267, "经济优先");

	outtextxy(920, 325, "允许骑行");

	setlinecolor(RGB(120, 140, 160));
	rectangle(1030, 325, 1050, 345);

	if (state.allow_bike) {
		setfillcolor(RGB(47, 191, 113));
		solidrectangle(1034, 329, 1046, 341);
	}

	setfillcolor(RGB(35, 105, 165));
	solidrectangle(930, 380, 1045, 420);
	settextcolor(RGB(255, 255, 255));
	outtextxy(947, 390, "开始规划");

	setfillcolor(RGB(230, 235, 240));
	solidrectangle(1055, 380, 1170, 420);
	setlinecolor(RGB(150, 160, 170));
	rectangle(1055, 380, 1170, 420);
	settextcolor(RGB(35, 45, 60));
	outtextxy(1090, 390, "重置");

	outtextxy(920, 450, "当前状态：");
	outtextxy(920, 480, state.message.c_str());
}

/***************************************************************************
  函数名称：draw_route_result
  功    能：在右侧面板绘制当前规划路线的时间、费用、到达时间和简要路线指南
  输入参数：const vertex vertices[]：顶点数组
  const transit_line lines[]：线路数组
  int line_number：线路数量
  const ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：第一版最多绘制三段路线指南，过长路线后续再加入分页或自动换行
***************************************************************************/
void draw_route_result(const vertex vertices[],
	const transit_line lines[], int line_number,
	const ui_state& state)
{
	if (!state.route_ready)
		return;

	settextcolor(RGB(35, 45, 60));
	settextstyle(16, 0, "微软雅黑");

	stringstream result_stream;

	result_stream << "总时间：" << state.total_time_cost << "分钟";
	string result_text = result_stream.str();
	outtextxy(920, 520, result_text.c_str());

	result_stream.str("");
	result_stream.clear();

	result_stream << fixed << setprecision(2)
		<< "总费用：" << state.total_fare_cost << "元";
	result_text = result_stream.str();
	outtextxy(920, 545, result_text.c_str());

	result_stream.str("");
	result_stream.clear();

	result_stream << "预计到达："
		<< setw(2) << setfill('0') << state.arrival_hour
		<< ":" << setw(2) << state.arrival_minute;

	if (state.days_passed > 0)
		result_stream << " +" << state.days_passed << "天";

	result_text = result_stream.str();
	outtextxy(920, 570, result_text.c_str());

	int guide_number = 1;
	int y = 600;

	for (int i = state.path_vertex_number - 1;
		i > 0 && guide_number <= 3; i--) {

		int from_vertex = state.path[i];
		int to_vertex = state.path[i - 1];

		const edge_node* current_edge = state.previous_edge[to_vertex];

		if (!current_edge)
			continue;

		string way_text;

		if (current_edge->type == edge_type::TRANSFER) {
			if (get_effective_time_cost(*current_edge,
				state.allow_bike) < current_edge->time_cost)
				way_text = "骑行";
			else
				way_text = "步行";
		}
		else if (current_edge->line_id >= 0
			&& current_edge->line_id < line_number) {

			way_text = lines[current_edge->line_id].name;
		}

		result_stream.str("");
		result_stream.clear();

		result_stream << guide_number << ". "
			<< way_text << " -> "
			<< vertices[to_vertex].name;

		result_text = result_stream.str();

		outtextxy(920, y, result_text.c_str());

		y += 25;
		guide_number++;
	}

	if (state.path_vertex_number - 1 > 3)
		outtextxy(920, y, "...");
}


/***************************************************************************
  函数名称：draw_easyx_interface
  功    能：按照固定图层顺序完整刷新EasyX界面
  输入参数：const vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  const transit_line lines[]：线路数组
  int line_number：线路数量
  const ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：绘制顺序为背景、普通边、推荐路线、普通站点、起终点标记、右侧控制面板和结果
***************************************************************************/
void draw_easyx_interface(const vertex vertices[], int vertex_number,
	const transit_line lines[], int line_number,
	const ui_state& state)
{
	setbkcolor(RGB(247, 250, 252));
	cleardevice();

	draw_all_edges(vertices, vertex_number);

	if (state.route_ready)
		draw_route_highlight(vertices, state);

	draw_all_vertices(vertices, vertex_number);
	draw_selected_vertices(vertices, vertex_number, state);

	setlinecolor(RGB(190, 200, 210));
	setlinestyle(PS_SOLID, 1);
	line(900, 0, 900, 700);

	draw_control_panel(vertices, vertex_number, state);

	if (state.route_ready)
		draw_route_result(vertices, lines, line_number, state);

	FlushBatchDraw();
}

/***************************************************************************
  函数名称：reset_ui_state
  功    能：把EasyX界面状态恢复到程序启动时的默认状态
  输入参数：ui_state& state：需要重置的界面状态
  返 回 值：无
  说    明：数组中的旧数据无需逐项清零，只要路径长度归零且route_ready为false即可视为无效
***************************************************************************/
void reset_ui_state(ui_state& state)
{
	state.start_vertex = -1;
	state.end_vertex = -1;

	state.k = 0;
	state.allow_bike = false;

	state.start_hour = 8;
	state.start_minute = 30;

	state.path_vertex_number = 0;

	state.total_time_cost = 0;
	state.total_fare_cost = 0;

	state.arrival_hour = 0;
	state.arrival_minute = 0;
	state.days_passed = 0;

	state.route_ready = false;
	state.message = "请选择起点";
}

/***************************************************************************
  函数名称：calculate_route_for_ui
  功    能：根据当前ui_state中的起终点和策略调用已有算法完成一次路线规划
  输入参数：vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  ui_state& state：当前EasyX界面状态，规划结果写回其中
  返 回 值：路线规划成功返回true，失败返回false
  说    明：内部严格按照dijkstra、build_paths、calculate_path_statistics、
  calculate_arrival_time的顺序调用原有算法，界面层不重新实现寻路算法
***************************************************************************/
bool calculate_route_for_ui(vertex vertices[], int vertex_number,
	ui_state& state)
{
	state.route_ready = false;
	state.path_vertex_number = 0;
	state.total_time_cost = 0;
	state.total_fare_cost = 0;

	if (!is_valid_vertex_id(state.start_vertex, vertex_number)
		|| !is_valid_vertex_id(state.end_vertex, vertex_number)) {

		state.message = "请先选择起点和终点";
		return false;
	}

	if (state.start_vertex == state.end_vertex) {
		state.message = "起点和终点不能相同";
		return false;
	}

	if (!dijkstra(vertices, vertex_number,
		state.start_vertex, state.k,
		state.distance, state.previous_vertex,
		state.previous_edge,
		state.allow_bike)) {
		state.message = "Dijkstra计算失败";
		return false;
	}

	if (!build_paths(state.previous_vertex, vertex_number,
		state.start_vertex, state.end_vertex,
		state.path, state.path_vertex_number)) {

		state.message = "当前起终点之间没有可用路线";
		return false;
	}

	if (!calculate_path_statistics(
		state.path, state.path_vertex_number,
		state.previous_edge,
		state.total_time_cost,
		state.total_fare_cost,
		state.allow_bike)) {

		state.message = "路线统计失败";
		return false;
	}

	if (!calculate_arrival_time(
		state.start_hour,
		state.start_minute,
		state.total_time_cost,
		state.arrival_hour,
		state.arrival_minute,
		state.days_passed)) {

		state.message = "到达时间计算失败";
		return false;
	}

	state.route_ready = true;
	state.message = "路线规划完成";

	return true;
}


/***************************************************************************
  函数名称：handle_left_click
  功    能：处理一次鼠标左键点击，根据点击位置修改站点选择、时间、策略、骑行或规划状态
  输入参数：int mouse_x：鼠标点击位置的x坐标
  int mouse_y：鼠标点击位置的y坐标
  vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：左侧x<900区域用于选择站点；右侧区域按照固定矩形坐标判断不同控件
***************************************************************************/
void handle_left_click(int mouse_x, int mouse_y,
	vertex vertices[], int vertex_number,
	ui_state& state)
{
	if (mouse_x < 900) {
		int clicked_vertex =
			find_clicked_vertex(vertices, vertex_number,
				mouse_x, mouse_y);

		if (clicked_vertex == -1)
			return;

		if (state.start_vertex == -1) {
			state.start_vertex = clicked_vertex;
			state.end_vertex = -1;
			state.route_ready = false;
			state.path_vertex_number = 0;
			state.message = "请选择终点";
		}
		else {
			if (clicked_vertex == state.start_vertex) {
				state.message = "终点不能与起点相同";
				return;
			}

			state.end_vertex = clicked_vertex;
			state.route_ready = false;
			state.path_vertex_number = 0;
			state.message = "起终点已选择，请开始规划";
		}

		return;
	}

	if (is_point_in_rectangle(mouse_x, mouse_y,
		930, 165, 980, 200)) {

		int total_minutes =
			state.start_hour * 60 + state.start_minute;

		total_minutes -= 5;

		if (total_minutes < 0)
			total_minutes += 24 * 60;

		state.start_hour = total_minutes / 60;
		state.start_minute = total_minutes % 60;

		state.route_ready = false;
	}

	else if (is_point_in_rectangle(mouse_x, mouse_y,
		1110, 165, 1160, 200)) {

		int total_minutes =
			state.start_hour * 60 + state.start_minute;

		total_minutes = (total_minutes + 5) % (24 * 60);

		state.start_hour = total_minutes / 60;
		state.start_minute = total_minutes % 60;

		state.route_ready = false;
	}

	else if (is_point_in_rectangle(mouse_x, mouse_y,
		930, 260, 1045, 295)) {

		state.k = 0;
		state.route_ready = false;
		state.message = "已选择时间最短策略";
	}

	else if (is_point_in_rectangle(mouse_x, mouse_y,
		1055, 260, 1170, 295)) {

		state.k = 8;
		state.route_ready = false;
		state.message = "已选择经济优先策略";
	}

	else if (is_point_in_rectangle(mouse_x, mouse_y,
		1030, 325, 1050, 345)) {

		state.allow_bike = !state.allow_bike;
		state.route_ready = false;

		if (state.allow_bike)
			state.message = "已允许骑行接驳";
		else
			state.message = "已关闭骑行接驳";
	}

	else if (is_point_in_rectangle(mouse_x, mouse_y,
		930, 380, 1045, 420)) {

		calculate_route_for_ui(vertices, vertex_number, state);
	}

	else if (is_point_in_rectangle(mouse_x, mouse_y,
		1055, 380, 1170, 420)) {

		reset_ui_state(state);
	}
}

/***************************************************************************
  函数名称：run_easyx_interface
  功    能：运行EasyX主界面消息循环，持续接收鼠标和键盘消息并根据状态重新绘制界面
  输入参数：vertex vertices[]：顶点数组
  int vertex_number：顶点数量
  const transit_line lines[]：线路数组
  int line_number：线路数量
  ui_state& state：当前EasyX界面状态
  返 回 值：无
  说    明：鼠标左键触发handle_left_click后重新绘制；按ESC或关闭窗口结束循环
***************************************************************************/
void run_easyx_interface(vertex vertices[], int vertex_number,
	const transit_line lines[], int line_number,
	ui_state& state)
{
	BeginBatchDraw();

	draw_easyx_interface(vertices, vertex_number,
		lines, line_number, state);

	while (true) {
		ExMessage message =
			getmessage(EX_MOUSE | EX_KEY | EX_WINDOW);

		if (message.message == WM_CLOSE)
			break;

		if (message.message == WM_KEYDOWN
			&& message.vkcode == VK_ESCAPE)
			break;

		if (message.message == WM_LBUTTONDOWN) {

			handle_left_click(message.x, message.y,
				vertices, vertex_number, state);

			draw_easyx_interface(vertices, vertex_number,
				lines, line_number, state);
		}
	}

	EndBatchDraw();
}





















/***************************************************************************
  函数名称：
  功    能：
  输入参数：
  返 回 值：
  说    明：
***************************************************************************/
int main()
{
	string lines_path = "data/lines.csv";//线路CSV路径
	string stations_path = "data/stations.csv";//站点CSV路径
	string edges_path = "data/edges.csv";//边CSV路径
	transit_line lines[max_lines];
	int line_number = 0;//当前实际线路数量，初始还没加站点所以为0
	if (!load_transit_lines(lines_path, lines, line_number)) {
		cout << "加载线路CSV文件失败" << endl;
		return 1;
	}

	vertex vertices[max_vertices];
	int vertex_number = 0;//当前实际顶点数量，初始还没加站点所以为0
	if (!load_vertices(stations_path, vertices, vertex_number)) {
		cout << "加载站点CSV文件失败" << endl;
		return 2;
	}

	if (!load_edges(edges_path, vertices, vertex_number, lines, line_number)) {
		cout << "加载边CSV文件失败" << endl;
		return 3;
	}
	

#if test_adjacency_list
	for (int i = 0; i < vertex_number; i++)
		output_one_step_vertex(vertices[i]);
#endif

	initgraph(1200, 700, EX_SHOWCONSOLE);

#if test_min_heap
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

#if test_weight
	edge_node cal;
	cal.time_cost = 6;
	cal.fare_cost = 0.5;
	cout << calculate_weight(cal, 0) << endl << calculate_weight(cal, 8) << endl;
#endif
	
	
	ui_state state;
	run_easyx_interface(vertices, vertex_number, lines, line_number, state);

	for (int i = 0; i < vertex_number; i++)
		release_edges(vertices[i]);
	closegraph();

	return 0;
}
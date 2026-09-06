#include "../include/dijkstra.h"
#include "../include/min_heap.h"
#include <iostream>
#include <iomanip>
using namespace std;

/***************************************************************************
  函数名称：get_effective_time_cost
  功    能：从time_cost和bike_time_cost里选一个合理值
  输入参数：const edge_node& node：需要判断的边，bool allow_bike：是否允许骑车来代替获得更短的time_cost
  返 回 值：最终选定的time_cost
  说    明：如果允许骑车；类型为换乘边；骑车用时合理同时满足time_cost为骑行用时
***************************************************************************/
int get_effective_time_cost(const edge_node& node, bool allow_bike)
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
double calculate_weight(const edge_node& node, double k, bool allow_bike)
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
void output_dijkstra_arrays(const string& prompt, const double distance[], const int previous_vertex[], int vertex_number)
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
bool is_valid_vertex_id(const int vertex_id, const int vertex_number)
{
    if (vertex_id >= 0 && vertex_id < vertex_number)
        return true;//开始下标应该在0..vertex_number-1之间，数组下标
    else
        return false;
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
    const edge_node* previous_edge[], bool allow_bike)
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
    if (!heap.initialize_heap(vertex_number))
        return false;
    if (!heap.insert_heap(start_vertex, distance[start_vertex])) {
        heap.release_heap();//插入失败，则释放
        return false;
    }
    while (!heap.is_heap_empty()) {
        heap_node current_node;
        if (!heap.extract_min(current_node)) {
            heap.release_heap();//拿出堆顶最小元素失败，则释放
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
                if (!heap.insert_heap(next_vertex, distance[next_vertex])) {//把邻接顶点加入堆中，等待下一轮弹出
                    heap.release_heap();//插入失败，则释放
                    return false;
                }
            }

            current_edge = current_edge->next;
        }
    }
    heap.release_heap();

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
    bool allow_bike)
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

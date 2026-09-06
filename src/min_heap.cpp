#include "../include/min_heap.h"
#include <iostream>
using namespace std;

/***************************************************************************
  函数名称：initialize_heap
  功    能：初始化最小堆
  输入参数：int initial_capacity：初始容量
  返 回 值：false表示失败，true表示成功
  说    明：当前对象即为需要初始化的最小堆
***************************************************************************/
bool min_heap::initialize_heap(int initial_capacity)
{
    if (initial_capacity <= 0 || data)//容量非正或data已经存放了地址
        return false;
    heap_node* p = new heap_node[initial_capacity];
    data = p;//data字段指向动态堆数组的第一个元素
    size = 0;
    capacity = initial_capacity;
    return true;
}
/***************************************************************************
  函数名称：swap_heap_node
  功    能：交换两个堆结点，连带着顶点编号和当前距离两个结构体成员一起交换
  输入参数：heap_node& node1, heap_node& node2：需要交换的两个堆结点，交换后node1、node2所有成员互换
  返 回 值：无
  说    明：注意要同时交换结构体里的编号和距离确保依然匹配，由于结构体里只有普通成员，也可以直接交换整个结构体，可用自带swap
***************************************************************************/
void min_heap::swap_heap_node(heap_node& node1, heap_node& node2)
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
  输入参数：int index：需要判断是否交换的下标
  返 回 值：无
  说    明：index = father配合while语句可以实现逐层检查直至堆顶
***************************************************************************/
void min_heap::sift_up(int index)
{
    while (index > 0) {
        int father = (index - 1) / 2;
        double father_distance = data[father].distance;
        double index_distance = data[index].distance;
        if (father_distance > index_distance) {
            swap_heap_node(data[father], data[index]);
            index = father;//如果交换，交换后将index赋值为父节点的值
        }
        else
            break;//该节点已经到了正确的位置上
    }
}
/***************************************************************************
  函数名称：expand_heap
  功    能：堆满时扩容至原来容量的两倍
  输入参数：无
  返 回 值：false代表扩容失败，true代表成功
  说    明：Dijkstra中同一顶点可能因为距离更新而多次入堆，所以容量不一定等于顶点数；默认扩为两倍
***************************************************************************/
bool min_heap::expand_heap()
{
    if (!data || (capacity <= 0))
        return false;
    heap_node* bigger_heap = new heap_node[capacity * 2];
    for (int i = 0; i < size; i++)
        bigger_heap[i] = data[i];//复制，size不变
    delete[] data;//释放
    data = bigger_heap;//data指向新堆
    capacity *= 2;
    return true;
}
/***************************************************************************
  函数名称：insert_heap
  功    能：在堆中插入某个元素，加入sift_up调用包括重新对最小堆排序，堆满时会调用expand_heap扩容
  输入参数：int vertex_id：插入顶点的编号
   double distance：插入顶点的当前距离
  返 回 值：false代表插入失败，true代表成功
  说    明：size >= capacity指没空余容量；!data表明堆当前没有动态数组，通常因为initialize_heap初始化失败
***************************************************************************/
bool min_heap::insert_heap(int vertex_id, double distance)
{
    if (!data)
        return false;
    if (size >= capacity) {//其实只能=，所以只需要扩一次容即可，不需要while循环
        if (!expand_heap())
            return false;//没return就完成了扩容
    }
    (data + size)->vertex_id = vertex_id;//等价于(*(data + size)).vertex_id或是data[size].vertex_id
    (data + size)->distance = distance;
    size++;//插入完后实际数量加1
    sift_up(size - 1);//这里不和上一行换顺序是因为要把新元素正式纳入有效范围
    return true;
}
/***************************************************************************
  函数名称：sift_down
  功    能：下沉单个位置的节点，和子节点进行大小关系判断
  输入参数：int index：需要判断是否交换的下标
  返 回 值：无
  说    明：index = left_child/right_child;配合while语句可以实现逐层检查直至叶子节点
***************************************************************************/
void min_heap::sift_down(int index)
{
    while (2 * index + 1 < size) { //左孩子在下标0..(size-1)范围内
        int left_child = 2 * index + 1;
        int right_child = 2 * index + 2;
        double left_child_distance = data[left_child].distance;
        double index_distance = data[index].distance;
        if (right_child >= size) {//这个时候只有左孩子
            if (index_distance > left_child_distance) {
                swap_heap_node(data[left_child], data[index]);
                index = left_child;
            }
            break;//堆是完全二叉树，唯一的左孩子必然是最后一个元素也是叶子结点，所以处理完它之后无论是否交换，本轮下沉都已经结束，而不是else
        }
        double right_child_distance = data[right_child].distance;//有右孩子才能拿对应下标
        if (index_distance < left_child_distance && index_distance < right_child_distance)//父最小，已到位
            break;
        else if (left_child_distance <= right_child_distance) {//左不超过右侧，父节点和较小的左换
            swap_heap_node(data[left_child], data[index]);
            index = left_child;
        }
        else {//换父和右
            swap_heap_node(data[right_child], data[index]);
            index = right_child;
        }
    }
}
/***************************************************************************
  函数名称：is_heap_empty
  功    能：检查堆是否是空堆
  输入参数：无
  返 回 值：true代表确实空，false代表并不空
  说    明：只根据size是否为0判断，不根据data判断，因为已经初始化但没有元素时，data不为空但堆仍然是空堆。
***************************************************************************/
bool min_heap::is_heap_empty() const
{
    if (size == 0)
        return true;
    else
        return false;
}
/***************************************************************************
  函数名称：extract_min
  功    能：弹出最小元素，需要1.删除堆顶，2.把原堆顶的heap_node交给调用者
  输入参数：heap_node &minimum_node：用于放原堆顶的heap_node
  返 回 值：false代表弹出失败，true代表成功
  说    明：
***************************************************************************/
bool min_heap::extract_min(heap_node& minimum_node)
{
    if (is_heap_empty())
        return false;
    minimum_node = data[0];//存放堆顶元素
    data[0] = data[size - 1];
    size--;//位置-1
    sift_down(0);//从堆顶开始下沉

    return true;
}
/***************************************************************************
  函数名称：release_heap
  功    能：释放整个堆
  输入参数：无
  返 回 值：无
  说    明：先释放data对象动态内存申请的数组，然后重置类中各个成员的值
***************************************************************************/
void min_heap::release_heap()
{
    delete[] data;
    data = nullptr;
    size = 0;
    capacity = 0;
}

/***************************************************************************
  函数名称：~min_heap
  功    能：析构最小堆对象
  输入参数：无
  返 回 值：无
  说    明：对象离开作用域时再次保证动态数组已经释放；release_heap可重复调用
***************************************************************************/
min_heap::~min_heap()
{
    release_heap();
}

/***************************************************************************
  函数名称：get_size
  功    能：取得当前最小堆中的实际元素数量
  输入参数：无
  返 回 值：当前size
  说    明：仅用于读取，不允许类外直接修改size
***************************************************************************/
int min_heap::get_size() const
{
    return size;
}

/***************************************************************************
  函数名称：get_capacity
  功    能：取得当前最小堆动态数组的容量
  输入参数：无
  返 回 值：当前capacity
  说    明：仅用于读取，不允许类外直接修改capacity
***************************************************************************/
int min_heap::get_capacity() const
{
    return capacity;
}

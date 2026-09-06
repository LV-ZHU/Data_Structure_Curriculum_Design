#pragma once

#include "common/structure.h"

// 最小堆
class min_heap {
private:
    heap_node* data = nullptr;//当前指向元素位置
    int size = 0;//当前实际存有的堆元素数量，用来控制下标小于size防止读取时越界
    int capacity = 0;//当前数组最多能容纳多少个元素，应该始终满足0≤size≤capacity

    void swap_heap_node(heap_node& node1, heap_node& node2);
    void sift_up(int index);
    bool expand_heap();
    void sift_down(int index);

public:
    min_heap() = default;
    ~min_heap();

    // 类中含动态数组，不能直接复制，避免两个对象同时指向同一块内存
    min_heap(const min_heap&) = delete;
    min_heap& operator=(const min_heap&) = delete;

    bool initialize_heap(int initial_capacity);
    bool insert_heap(int vertex_id, double distance);
    bool is_heap_empty() const;
    bool extract_min(heap_node& minimum_node);
    void release_heap();

    int get_size() const;
    int get_capacity() const;
};

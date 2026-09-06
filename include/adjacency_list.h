#pragma once
#include "common/structure.h"

// 邻接表类，保存顶点数组，并负责边结点的建立、遍历和释放
class adjacency_list {
private:
    vertex vertices[max_vertices];
    int vertex_number = 0;

    void add_directed_edge(vertex& from_vertex, int to, int time_cost, double fare_cost,
        edge_type type, int line_id = -1, int bike_time_cost = -1);

public:
    adjacency_list() = default;
    ~adjacency_list();

    // 邻接表中含动态边结点，不能直接复制，避免重复释放同一组链表
    adjacency_list(const adjacency_list&) = delete;
    adjacency_list& operator=(const adjacency_list&) = delete;

    bool add_vertex(string vertex_name, station_type type, int x, int y);
    void add_undirected_edge(vertex& first_vertex, vertex& second_vertex, int time_cost,
        double fare_cost, edge_type type, int line_id = -1, int bike_time_cost = -1);
    void release_edges(vertex& release_vertex);
    void output_one_step_vertex(vertex& start_station) const;
    const edge_node* find_directed_edge(const vertex& from_vertex, int to) const;

    vertex* get_vertices();
    const vertex* get_vertices() const;
    int get_vertex_number() const;
};

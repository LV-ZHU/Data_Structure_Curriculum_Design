#pragma once

#include "common/structure.h"

int get_effective_time_cost(const edge_node& node, bool allow_bike = false);
double calculate_weight(const edge_node& node, double k, bool allow_bike = false);
void output_dijkstra_arrays(const string& prompt, const double distance[], const int previous_vertex[], int vertex_number);
bool is_valid_vertex_id(const int vertex_id, const int vertex_number);
bool dijkstra(vertex vertices[], int vertex_number, int start_vertex, int k, double distance[], int previous_vertex[],
    const edge_node* previous_edge[], bool allow_bike = false);
bool build_paths(const int previous_vertex[], int vertex_number, int start_vertex, int end_vertex, int path[], int& path_vertex_number);
bool calculate_path_statistics(const int path[], int path_vertex_number, const edge_node* previous_edge[],
    int& total_time_cost, double& total_fare_cost, bool allow_bike = false);
bool calculate_arrival_time(int start_hour, int start_minute, int total_minutes,
    int& arrival_hour, int& arrival_minute, int& days_passed);

#pragma once

#include "adjacency_list.h"

bool add_transit_line(transit_line lines[], int& line_number, string line_name, edge_type line_type);
void remove_trailing_carriage_return(string& text);
bool load_transit_lines(const string& file_path, transit_line lines[], int& line_number);
bool load_vertices(const string& file_path, adjacency_list& graph);
bool load_edges(const string& file_path, adjacency_list& graph, const transit_line lines[], int line_number);

#include "../include/csv_data.h"
#include "../include/adjacency_list.h"
#include "../include/dijkstra.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace std;

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
  函数名称：remove_trailing_carriage_return
  功    能：去掉文本行末尾可能残留的回车字符
  输入参数：string& text：需要检查的文本行
  返 回 值：无
  说    明：Windows文本模式通常会自动处理CRLF；保留此检查可兼容其他编译环境
***************************************************************************/
void remove_trailing_carriage_return(string& text)
{
    if (!text.empty() && text.back() == '\r')
        text.pop_back();
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
bool load_transit_lines(const string& file_path, transit_line lines[], int& line_number)
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
    remove_trailing_carriage_return(header);
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
        remove_trailing_carriage_return(data_line);
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
  函数名称：load_vertices
  功    能：从指定CSV文件读取全部站点数据
  输入参数：const string& file_path：要读取的站点CSV文件路径
  adjacency_list& graph：存放读取结果的邻接表对象
  返 回 值：true代表读取成功，false代表读取失败
  说    明：
***************************************************************************/
bool load_vertices(const string& file_path, adjacency_list& graph)
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
    remove_trailing_carriage_return(header);
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
        remove_trailing_carriage_return(data_line);
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

        if (graph.get_vertex_number() == station_id) {//校验当前站点编号和CSV文件里读到的是否一致
            if (graph.add_vertex(station_name_text, parsed_station_type, station_x, station_y)) {
#if test_csv_lines
                cout << graph.get_vertices()[station_id].id << " " << graph.get_vertices()[station_id].name << endl;
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
    if (graph.get_vertex_number() == 0) {//只有表头没有具体站点
        cout << "站点CSV中没有有效站点数据" << endl;
        return false;
    }
    stations_file.close();//关文件

    return true;
}
/***************************************************************************
  函数名称：load_edges
  功    能：从指定CSV文件读取全部边数据
  输入参数：const string& file_path：要读取的边CSV文件路径
  adjacency_list& graph：需要修改的邻接表对象
  const transit_line lines[]：线路数组
  int line_number：检查 line_id 范围
  返 回 值：true代表读取成功，false代表读取失败
  说    明：一行CSV表示一条无向边，函数内部调用邻接表对象的add_undirected_edge创建两个方向
***************************************************************************/
bool load_edges(const string& file_path, adjacency_list& graph, const transit_line lines[], int line_number)
{
    vertex* vertices = graph.get_vertices();
    int vertex_number = graph.get_vertex_number();
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
    remove_trailing_carriage_return(header);
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
        remove_trailing_carriage_return(data_line);

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
            if (lines[line_id].type != parsed_edge_type) {
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
            if (bike_time_cost != -1 && (bike_time_cost <= 0 || bike_time_cost >= time_cost)) {
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



        graph.add_undirected_edge(vertices[first_vertex_id], vertices[second_vertex_id],
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

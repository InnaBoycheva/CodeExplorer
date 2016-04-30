#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

#include "FuncStruct.h"
#include "DBConnection.h"

struct Node {
public:
	Node() {}
	std::vector<Node*> includes;
	std::vector<FuncStruct> functions;
	std::vector<std::string> defined_classes;
	std::string name;
};

class FilesGraph {
public:
	FilesGraph(DBConnection* con, int proj_id);
	~FilesGraph();
	void analyze_file(const std::string& file_name);

//private:
	DBConnection* con;
	int proj_id;
	std::map<std::string, Node> graph;

	std::string get_include(std::string line);
	void add_include(std::string include, std::string file_prefix, Node& node);
	void save_nodes_in_db();
	void prepare_files_table_update();
	void prepare_classes_table_update();
	void prepare_functions_table_update();

};
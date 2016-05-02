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

	void create_files_table();
	void create_classes_table();
	void create_functions_table();

	int add_file(std::string filename, std::string& file_content);
	void insert_classes_in_DB(int file_id, const Node& node);
	void insert_functions_in_DB(int file_id, const Node& node);

};
#include <assert.h>
#include "FilesGraph.h"
#include "Parser.h"

using namespace std;

FilesGraph::FilesGraph(DBConnection* con, int proj_id) {
	this->con = con;
	this->proj_id = proj_id;

	// Create all the tabllleeeeees
	create_files_table();
	create_classes_table();
	create_functions_table();
}

FilesGraph::~FilesGraph() {
}

string get_base_path(string path) {
	int i = path.find_last_of('\\');
	if (i != string::npos)
		return path.substr(0, i);
}

string get_name_from_path(string path) {
	int i = path.find_last_of('\\');
	if (i != string::npos)
		return path.substr(i+1, path.size() - 1);
}

void FilesGraph::link_functions(const string& path) {

	// Open file
	ifstream file(path);
	if (!file) {
		cerr << "Error Opening " << path << endl;
		return;
	}

	Parser line_parser;
	string line;

	while (!file.eof()) {
		getline(file, line);

		// Scan
		line_parser.scan_line(line, &graph[path], true);
	}
}

void FilesGraph::analyze_file(const string& path) {

	// Open file
	ifstream file(path);
	if (!file) {
		cerr << "Error Opening " << path << endl;
		return;
	}

	// Add file node if it does not exist
	auto it = graph.find(path);
	if (it == graph.end()) {
		it = graph.emplace(path, Node()).first;
		it->second.name = path;
	}
	Node& node = it->second;

	Parser line_parser;
	string line;
	string file_content;

	while (!file.eof()) {
		getline(file, line);
		file_content += line + "\n";

		// Scan for includes
		add_include(get_include(line), path, node);

		// Scan for function defs
		line_parser.scan_line(line, &node, false);
	}

	// Populate tables
	int file_id = add_file(get_name_from_path(path), file_content);
	insert_classes_in_DB(file_id, node);
	insert_functions_in_DB(file_id, node);

}

string subst_of_cstr(const char *cstr, size_t start, size_t length) {
	assert(start + length <= strlen(cstr));
	return string(cstr + start, length);
}

string FilesGraph::get_include(string line) {

	const char* c = line.c_str();
	char* include_name = "";

	for (c; *c != '\0'; ++c) {
		switch (*c) {
			case ' ':
			case '\t':
				continue;
				break;
			case '#':
				++c;
				break;
			default:
				return include_name;
		}

		int num_chars = 1;
		if (strncmp(c, "include", 7) == 0) {
			c += 7;
			for (; *c != '\0'; ++c) {
				switch (*c) {
					case '<':
						++c;
						for (; *c != '\0'; ++c) {
							if (*c == '>') {
								c -= num_chars;
								return subst_of_cstr(c, 0, num_chars+1);
							} else {
								num_chars++;
							}
						}
						break;
					case '"':
						++c;
						num_chars = 1;
						for (; *c != '\0'; ++c) {
							if (*c == '"') {
								c -= num_chars;
								return subst_of_cstr(c, 0, num_chars+1);
							} else {
								num_chars++;
							}
						}
						break;
					default:
						break;
				}
			}
		}
	}
	return include_name;
}

void FilesGraph::add_include(string include, string path, Node& node) {

	// If not an include, nothing to add
	if (include.empty())
		return;

	// User defined includes
	if (include.at(0) == '"') {

		// Get file name from full path
		string prefix = get_base_path(path);

		include = prefix + "\\" + include.substr(1, include.size() - 2);

		// Check to see if already in graph
		auto it = graph.find(include);
		if (it == graph.end()) {
			it = graph.emplace(include, Node()).first;
			it->second.name = include;
		}

		Node& child_node = it->second;
		node.includes.push_back(&child_node);
	}
}

void FilesGraph::create_files_table() {
	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");
	columns.fields.push_back("content LONGTEXT DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("files_" + string(proj), columns);
}

void FilesGraph::create_classes_table() {
	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("file_id int(11) NOT NULL");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("classes_" + string(proj), columns);
}

void FilesGraph::create_functions_table() {
	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");
	columns.fields.push_back("file_id int(11) NOT NULL");
	columns.fields.push_back("return_type VARCHAR(255) DEFAULT NULL");
	columns.fields.push_back("def_line VARCHAR(255) DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("functions_" + string(proj), columns);
}

int FilesGraph::add_file(string filename, string& file_content) {

	DBConnection::table_row columns;
	DBConnection::table_row row;

	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);

	// Add new file entries
	row.fields.push_back(filename);	// name
	row.fields.push_back(file_content);

	columns.fields.push_back("name");
	columns.fields.push_back("content");

	return con->insert_row("files_" + string(proj), columns, row);
}

void FilesGraph::insert_classes_in_DB(int file_id, const Node& node) {

	DBConnection::table_row columns;
	columns.fields.push_back("file_id");
	columns.fields.push_back("name");

	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);

	for (int i = 0; i < node.defined_classes.size(); ++i) {
		DBConnection::table_row row;

		row.fields.push_back(file_id);							// file_id
		row.fields.push_back(node.defined_classes.at(i));			// class name
		// namespace

		con->insert_row("classes_" + string(proj), columns, row);
	}
}

void FilesGraph::insert_functions_in_DB(int file_id, const Node& node) {

	DBConnection::table_row columns;
	columns.fields.push_back("name");
	columns.fields.push_back("file_id");
	columns.fields.push_back("return_type");
	columns.fields.push_back("def_line");

	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	for (int i = 0; i < node.functions.size(); ++i) {
		DBConnection::table_row row;

		row.fields.push_back(node.functions.at(i).get_name());			// function name
		row.fields.push_back(file_id);										// file id
		// class id			
		// namespace id
		row.fields.push_back(node.functions.at(i).get_return_type());	// return type
		row.fields.push_back(node.functions.at(i).get_def_line());		// def line
		// decl line

		con->insert_row("functions_" + string(proj), columns, row);
	}
}
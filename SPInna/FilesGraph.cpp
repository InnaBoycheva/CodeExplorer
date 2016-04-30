#include <assert.h>
#include "FilesGraph.h"
#include "Parser.h"

using namespace std;

FilesGraph::FilesGraph(DBConnection* con, int proj_id) {
	this->con = con;
	this->proj_id = proj_id;
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

	while (!file.eof()) {

		getline(file, line);

		// Scan for includes
		add_include(get_include(line), path, node);

		// Scan for function defs
		line_parser.scan_line(line, &node);

	}
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

void FilesGraph::save_nodes_in_db() {

	prepare_files_table_update();
	prepare_classes_table_update();

}

void FilesGraph::prepare_files_table_update() {

	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");
	columns.fields.push_back("content LONGTEXT DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("files_" + string(proj), columns);

	vector<DBConnection::table_row> rows;

	// Add new file entries
	for (auto entry : graph) {
		DBConnection::table_row row;
		row.fields.push_back(get_name_from_path(entry.first));	// name
		row.fields.push_back("test");							// content
		rows.push_back(row);
	}

	columns.fields.clear();
	columns.fields.push_back("name");
	columns.fields.push_back("content");

	con->insert_rows("files_" + string(proj), columns, rows);

}

void FilesGraph::prepare_classes_table_update() {

	// classes - file_id, class name, namespace
	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("file_id int(11) NOT NULL");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("classes_" + string(proj), columns);

	vector<DBConnection::table_row> rows;

	for (auto entry : graph) {
		DBConnection::table_row row;
		for (int i = 0; i < entry.second.defined_classes.size(); ++i) {
			string file_name = get_name_from_path(entry.first);
			int file_id = con->get_entry_id("files_" + string(proj), "name" , file_name);

			char file[256];
			_itoa_s(file_id, file, 256, 10);

			row.fields.push_back(string(file));
			row.fields.push_back(entry.second.defined_classes.at(i));	// class name
			// namespace

			rows.push_back(row);
		}
		
	}

	columns.fields.clear();
	columns.fields.push_back("file_id");
	columns.fields.push_back("name");

	con->insert_rows("classes_" + string(proj), columns, rows);
}

void FilesGraph::prepare_functions_table_update() {

	// function id
	// function name
	// class id (null if not defined)
	// namespace id (only populate if class_id = null)
	// return_type
	// def_line
	// decl_line

	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");
	columns.fields.push_back("return_type VARCHAR(255) DEFAULT NULL");
}
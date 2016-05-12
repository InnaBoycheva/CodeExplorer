#include <assert.h>
#include "FilesGraph.h"
#include "Parser.h"

using namespace std;

/**/
/*
FilesGraph::FilesGraph()

NAME

FilesGraph::FilesGraph() - creates all the tables that must exist for a project.

SYNOPSIS

FilesGraph::FilesGraph(DBConnection* con, int proj_id);
con			--> the connection to the database.
proj_id		--> the current project id from the database.

DESCRIPTION

This function will create all tables that are required of a project in the
database. So far, each project must have three corresponding tables:
- files table
- classes table
- functions table

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
FilesGraph::FilesGraph(DBConnection* con, int proj_id) {
	this->con = con;
	this->proj_id = proj_id;

	// Create all tables that will contain the project information
	create_files_table();
	create_classes_table();
	create_functions_table();
}
/*FilesGraph::FilesGraph(DBConnection* con, int proj_id);*/

FilesGraph::~FilesGraph() {
}

/**/
/*
string get_base_path()

NAME

get_base_path() - extracts the base path for the project from one pointing
	to a specific file.

SYNOPSIS

string get_base_path(string path);
path			--> full path to a file.

DESCRIPTION

This function will extract the base path for a project from a path pointing
	to a specific file in it.

RETURNS

The base path as a string.

AUTHOR

Inna Boycheva

*/
/**/
string get_base_path(string path) {
	int i = path.find_last_of('\\');
	if (i != string::npos)
		return path.substr(0, i);
}
/*string get_base_path(string path);*/

/**/
/*
string get_name_from_path(string path)

NAME

get_name_from_path() - extracts the file name from a path pointing to it.

SYNOPSIS

string get_name_from_path(string path);
path			--> full path to a file.

DESCRIPTION

This function will extract the name of a file off a path pointing to it.

RETURNS

The file name as a string.

AUTHOR

Inna Boycheva

*/
/**/
string get_name_from_path(string path) {
	int i = path.find_last_of('\\');
	if (i != string::npos)
		return path.substr(i+1, path.size() - 1);
}
/*string get_name_from_path(string path);*/

/**/
/*
string subst_of_cstr()

NAME

subst_of_cstr() - extracts a substring from a given string.

SYNOPSIS

string subst_of_cstr(const char *cstr, size_t start, size_t length);
cstr			--> the cstring.
start			--> where to start slicing.
length			--> how many characters to retain after the start.

DESCRIPTION

This function will extract a substring from a given string.

RETURNS

The substring.

AUTHOR

Inna Boycheva

*/
/**/
string subst_of_cstr(const char *cstr, size_t start, size_t length) {
	assert(start + length <= strlen(cstr));
	return string(cstr + start, length);
}
/*string subst_of_cstr(const char *cstr, size_t start, size_t length);*/

/**/
/*
void FilesGraph::link_functions(const string& path)

NAME

FilesGraph::link_functions() - lets the parser object scan the file again and
	updates the files table linked to this project.

SYNOPSIS

void FilesGraph::link_functions(const string& path);
path			--> full path to the file.

DESCRIPTION

This function will let the parses go through the files one more time, but this
time figure out how the functions defined in the project are linked. A new file
content containing annotations marking these links will be saved into the DB in
files table.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::link_functions(const string& path) {

	// Open file
	ifstream file(path);
	if (!file) {
		cerr << "Error Opening " << path << endl;
		return;
	}
	 
	Parser line_parser;
	string line;

	// Convert the project id to string
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);

	// Scan file contents line by line
	while (!file.eof()) {
		getline(file, line);

		// Scan line for the second time
		line_parser.scan_line(line, &graph[path], true);
	}

	// Update the files table with the annotated file content
	// containing information on how the functions in the project are linked
	DBConnection::table_row cols;
	DBConnection::table_row update;
	update.fields.push_back(line_parser.get_content());
	cols.fields.push_back("content");
	con->update_row("files_" + string(proj), graph[path].id, cols, update);

	// Close file
	file.close();
}
/*void FilesGraph::link_functions(const string& path);*/

/**/
/*
void FilesGraph::analyze_file()

NAME

FilesGraph::link_functions() - scan file line by line, adding nodes to populate the graph
	representing the project structure when needed.

SYNOPSIS

void FilesGraph::analyze_file(const string& path);
path			--> path to the file currently being analyzed.

DESCRIPTION

This function opens a file by a given path and goes through it line by line.
Based on the includes appearing in the file currently being scanned, add nodes
to populate the graph. Also, let the Parser object parse each line individually and
establish the relationships between the elements (if any).

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
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

	// Scan file line by line
	while (!file.eof()) {
		getline(file, line);
		file_content += line + "\n";

		// Scan for includes
		add_include(get_include(line), path, node);

		// Scan for function defs
		line_parser.scan_line(line, &node, false);
	}

	// Populate classes and function table linked to the current project
	int file_id = add_file(get_name_from_path(path), file_content);
	node.id = file_id;
	insert_classes_in_DB(file_id, node);
	insert_functions_in_DB(file_id, node);

	// Close file
	file.close();
}
/*void FilesGraph::analyze_file(const string& path);*/

/**/
/*
string FilesGraph::get_include()

NAME

FilesGraph::get_include() - searches the current line for potential includes.

SYNOPSIS

string FilesGraph::get_include(string line);
line			--> the current line of scan.

DESCRIPTION

This function scans through the current line of a file and checks for includes.
It goes through the line character by character, first looking for a "#", then
the word "include", and finally looks for the " / < characters while trying to balance
them to figure out the name of the include.

RETURNS

The include name (if include found). Empty string otherwise.

AUTHOR

Inna Boycheva

*/
/**/
string FilesGraph::get_include(string line) {

	// Interpret line as a cstring
	const char* c = line.c_str();
	char* include_name = "";

	// Check first character on line
	for (c; *c != '\0'; ++c) {
		switch (*c) {
			// If space of tab, ignore
			case ' ':
			case '\t':
				continue;
				break;
			// If # - could be a potential include
			case '#':
				++c;
				break;
			// Anything else - not an include
			default:
				return include_name;
		}

		// Check if next letters are "include"
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
					// A programmer-defined include
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
/*string FilesGraph::get_include(string line);*/

/**/
/*
void FilesGraph::add_include()

NAME

FilesGraph::add_include() - adds an include to a node.

SYNOPSIS

void FilesGraph::add_include(string include, string path, Node& node);
include			--> the include to be added.
path			--> the path to the file.
node			--> the current node.

DESCRIPTION

This function adds an include to a node in the graph. It only adds programmer-defined
includes in order to represent the structure of the project as a graph. If the include
file does not exist as a node by itself yet, add it & then point to it from the current
node.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::add_include(string include, string path, Node& node) {

	// If not an include, nothing to add
	if (include.empty())
		return;

	// Programmer-defined includes
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

		// Add include to the current node
		Node& child_node = it->second;
		node.includes.push_back(&child_node);
	}
}
/*void FilesGraph::add_include(string include, string path, Node& node);*/

/**/
/*
void FilesGraph::create_files_table()

NAME

FilesGraph::create_files_table() - creates a new files table.

SYNOPSIS

void FilesGraph::create_files_table();

DESCRIPTION

This function prepares the columns for creating a brand new files table. Then
calls the DB connection object to reset the table, thus re-create it.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::create_files_table() {

	// Prepare table header columns
	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");
	columns.fields.push_back("content LONGTEXT DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("files_" + string(proj), columns);

}
/*void FilesGraph::create_files_table();*/

/**/
/*
void FilesGraph::create_classes_table()

NAME

FilesGraph::create_classes_table() - creates a new classes table.

SYNOPSIS

void FilesGraph::create_classes_table();

DESCRIPTION

This function prepares the columns for creating a brand new classes table. Then
calls the DB connection object to reset the table, thus re-create it.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::create_classes_table() {

	// Prepare column headers
	DBConnection::table_row columns;
	columns.fields.push_back("id int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY");
	columns.fields.push_back("file_id int(11) NOT NULL");
	columns.fields.push_back("name VARCHAR(255) DEFAULT NULL");

	// Drop current table & recreate it
	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);
	con->reset_table("classes_" + string(proj), columns);

}
/*void FilesGraph::create_classes_table();*/

/**/
/*
void FilesGraph::create_functions_table()

NAME

FilesGraph::create_functions_table() - creates a new functions table.

SYNOPSIS

void FilesGraph::create_functions_table();

DESCRIPTION

This function prepares the columns for creating a brand new functions table. Then
calls the DB connection object to reset the table, thus re-create it.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::create_functions_table() {

	// Prepare column headers
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
/*void FilesGraph::create_functions_table();*/

/**/
/*
int FilesGraph::add_file()

NAME

FilesGraph::add_file() - adds a new file to the files table.

SYNOPSIS

int FilesGraph::add_file(string filename, string& file_content);
filename		--> the name of the file.
file_content	--> the file content.

DESCRIPTION

This function adds a new file to the files table.

RETURNS

The id of the inserted file entry.

AUTHOR

Inna Boycheva

*/
/**/
int FilesGraph::add_file(string filename, string& file_content) {

	DBConnection::table_row columns;
	DBConnection::table_row row;

	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);

	// Add new file entries
	row.fields.push_back(filename);

	columns.fields.push_back("name");

	return con->insert_row("files_" + string(proj), columns, row);
}
/*int FilesGraph::add_file(string filename, string& file_content);*/

/**/
/*
void FilesGraph::insert_classes_in_DB()

NAME

FilesGraph::insert_classes_in_DB() - adds a new file to the files table.

SYNOPSIS

void FilesGraph::insert_classes_in_DB(int file_id, const Node& node);
file_id		--> the file id.
node		--> the current node.

DESCRIPTION

This function saves all defined classes in a project in the database. Each class
has a name and a file_id indicating the class in which the class has been defined.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::insert_classes_in_DB(int file_id, const Node& node) {

	// Prepare the column headers
	DBConnection::table_row columns;
	columns.fields.push_back("file_id");
	columns.fields.push_back("name");

	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);

	// For each defined class in the node, add its name & corresponding file id
	for (int i = 0; i < node.defined_classes.size(); ++i) {
		DBConnection::table_row row;

		row.fields.push_back(file_id);								// file_id
		row.fields.push_back(node.defined_classes.at(i));			// class name

		con->insert_row("classes_" + string(proj), columns, row);
	}
}
/*void FilesGraph::insert_classes_in_DB(int file_id, const Node& node);*/

/**/
/*
void FilesGraph::insert_functions_in_DB()

NAME

FilesGraph::insert_functions_in_DB() - adds a new file to the files table.

SYNOPSIS

void FilesGraph::insert_functions_in_DB(int file_id, Node& node);
file_id		--> the file id.
node		--> the current node.

DESCRIPTION

This function saves all functions in node into the functions table in the
database. The appropriate column headers are prepared at first, and then
each function is saved.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void FilesGraph::insert_functions_in_DB(int file_id, Node& node) {

	// Prepare column headers
	DBConnection::table_row columns;
	columns.fields.push_back("name");
	columns.fields.push_back("file_id");
	columns.fields.push_back("return_type");
	columns.fields.push_back("def_line");

	char proj[256];
	_itoa_s(proj_id, proj, 256, 10);

	// Add each function existing in the node data to the DB
	for (int i = 0; i < node.functions.size(); ++i) {
		DBConnection::table_row row;

		row.fields.push_back(node.functions.at(i).get_name());				// function name
		row.fields.push_back(file_id);										// file id
		row.fields.push_back(node.functions.at(i).get_return_type());		// return type
		row.fields.push_back(node.functions.at(i).get_def_line());			// def line

		node.functions.at(i).set_id(con->insert_row("functions_" + string(proj), columns, row));
		
	}
}
/*void FilesGraph::insert_functions_in_DB(int file_id, Node& node);*/
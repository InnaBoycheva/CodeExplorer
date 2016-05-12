#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include "FileManager.h"
#include "FilesGraph.h"
#include "DBConnection.h"

using namespace std;

int main(int argc, char** argv) {

	// Check if correct amount of commandline arguments is provided
	if (argc != 3) {
		cout << "Usage of the program: <directory name>, <db to connect to>" << endl;
		return 0;
	}

	string dir(argv[1]);

	// Establish connection to the DB
	string db_name(argv[2]);
	DBConnection* db = new DBConnection(db_name);
	int proj_id = db->find_project(dir);

	FileManager project;

	// Get all files in the directory provided
	vector<string> files = project.get_files(dir);

	FilesGraph project_graph(db, proj_id);

	// First pase of files - Build project structure
	for (auto it = files.begin(); it != files.end(); ++it) {
		string file_path = *it;
		project_graph.analyze_file(file_path);
	}

	// Second parse of files - Establishing links
	for (auto it = files.begin(); it != files.end(); ++it) {
		string file_path = *it;
		project_graph.link_functions(file_path);
	}

	return 0;
}
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include "FileManager.h"
#include "FilesGraph.h"
#include "DBConnection.h"

using namespace std;

int main(int argc, char** argv) {

	if (argc != 3) {
		cout << "Usage of the program: <directory name>, <db to connect to>" << endl;
		return 0;
	}

	// Get all files in the directory provided
	string dir(argv[1]);

	// Establish connection to the DB
	string db_name(argv[2]);
	DBConnection* db = new DBConnection(db_name);
	int proj_id = db->find_project(dir);

	FileManager project;
	vector<string> files = project.get_files(dir);

	// Build project structure
	FilesGraph project_graph(db, proj_id);
	for (auto it = files.begin(); it != files.end(); ++it) {
		string file_path = *it;
		project_graph.analyze_file(file_path);
	}

	/*for (auto it = files.begin(); it != files.end(); ++it) {
		// close all files
	}*/

	return 0;
}
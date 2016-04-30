#include "FileManager.h"
#include "mysql_connection.h"

using namespace std;

FileManager::FileManager() {
}

FileManager::~FileManager() {
}

void FileManager::get_files_specific_ext(string dirname, string extension, vector<string> & dir_files) {

	WIN32_FIND_DATAA file_data;
	HANDLE file = FindFirstFileA((dirname + "/*." + extension).c_str(), &file_data);

	if (file != INVALID_HANDLE_VALUE) {

		string file_name, full_file_name;
		do {
			file_name = file_data.cFileName;
			full_file_name = dirname + "\\" + file_name;
			const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			// Skip all files starting w/ a dot and directories
			if (file_name[0] == '.' || is_directory) continue;

			dir_files.push_back(full_file_name);

		} while (FindNextFileA(file, &file_data));

		FindClose(file);
	}
}

vector<string> FileManager::get_files(string dirname) {

	vector<string> dir_files = vector<string>();

	if (!dirname.empty()) {
		get_files_specific_ext(dirname, "h", dir_files);
		get_files_specific_ext(dirname, "cpp", dir_files);
	}

	return dir_files;
}

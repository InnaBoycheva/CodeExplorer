#include "FileManager.h"
#include "mysql_connection.h"

using namespace std;

FileManager::FileManager() {
}

FileManager::~FileManager() {
}

/**/
/*
void FileManager::get_files_specific_ext()

NAME

FileManager::get_files_specific_ext() - extracts files from a directory having a
	particular extension.

SYNOPSIS

void FileManager::get_files_specific_ext(string dirname, string extension, vector<string> & dir_files);
dirname			--> the directory the files are being pulled from.
extension		-->	the extension of interest.
dir_files		-->	already selected files.

DESCRIPTION

This function will pull all files of a particular extension from a given directory.

RETURNS

void (files list is updated by reference)

AUTHOR

Inna Boycheva

*/
/**/
void FileManager::get_files_specific_ext(string dirname, string extension, vector<string> & dir_files) {

	WIN32_FIND_DATAA file_data;
	HANDLE file = FindFirstFileA((dirname + "/*." + extension).c_str(), &file_data);

	if (file != INVALID_HANDLE_VALUE) {

		string file_name, full_file_name;
		do {
			file_name = file_data.cFileName;
			full_file_name = dirname + "\\" + file_name;
			const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			// Skip all files starting with a dot and directories
			if (file_name[0] == '.' || is_directory) continue;

			dir_files.push_back(full_file_name);

		} while (FindNextFileA(file, &file_data));

		FindClose(file);
	}
}
/*void FileManager::get_files_specific_ext(string dirname, string extension, vector<string> & dir_files);*/

/**/
/*
vector<string> FileManager::get_files()

NAME

FileManager::get_files() - extracts all files to be analyzed.

SYNOPSIS

vector<string> FileManager::get_files(string dirname);
dirname        --> the directory the files are being pulled from.

DESCRIPTION

This function will pull all .cpp and .h files from the directory provided.

RETURNS

All files successfully taken from the directory provided.

AUTHOR

Inna Boycheva

*/
/**/
vector<string> FileManager::get_files(string dirname) {

	vector<string> dir_files = vector<string>();

	// If directory provided is not empty, pull all files with
	// .cpp and .h extensions
	if (!dirname.empty()) {
		get_files_specific_ext(dirname, "h", dir_files);
		get_files_specific_ext(dirname, "cpp", dir_files);
	}

	return dir_files;

}
/*vector<string> FileManager::get_files(string dirname);*/
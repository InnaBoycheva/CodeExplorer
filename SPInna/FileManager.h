#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

class FileManager {

public:
	FileManager();
	~FileManager();
	std::vector<std::string> get_files(std::string dirname);

private:
	void get_files_specific_ext(std::string dirname, std::string extension, std::vector<std::string> & dir_files);
};

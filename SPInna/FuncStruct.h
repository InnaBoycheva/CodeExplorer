#pragma once
#include <iostream>
#include <vector>
#include <utility>

class FuncStruct
{
public:
	FuncStruct();
	~FuncStruct();

	struct arg_struct {
		std::string name;
		std::string data_type;
	};

	std::string get_return_type();
	void set_return_type(std::string return_type);

	std::string get_name();
	void set_name(std::string name);

	std::string get_class_name();
	void set_class_name(std::string class_name);

	//std::vector<std::pair<std::string, std::string>> get_args();
	//void add_arg(std::pair<std::string, std::string> arg);

	std::vector<std::string> get_namespaces();
	void set_namespaces(std::vector<std::string> namespaces);

	void add_namespace(std::string new_namespace);

	std::vector<arg_struct> args;
	
private:
	std::string return_type;
	std::string name;
	//std::string class_name;
	std::vector<std::string> namespaces;
};

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

	std::string get_return_type() const;
	void set_return_type(std::string return_type);

	std::string get_name() const;
	void set_name(std::string name);

	std::string get_class_name() const;
	void set_class_name(std::string class_name);

	//std::vector<std::pair<std::string, std::string>> get_args();
	//void add_arg(std::pair<std::string, std::string> arg);

	std::vector<std::string> get_namespaces() const;
	void set_namespaces(std::vector<std::string> namespaces);
	void add_namespace(std::string new_namespace);

	void set_def_line(int line);
	int get_def_line() const;

	std::vector<arg_struct> args;
	
private:
	std::string return_type;
	std::string name;
	//std::string class_name;
	std::vector<std::string> namespaces;
	int def_line;
};

#include "FuncStruct.h"

FuncStruct::FuncStruct() {
}

FuncStruct::~FuncStruct() {
}

std::string FuncStruct::get_return_type() const {
	return this->return_type;
}
/*std::string FuncStruct::get_return_type() const;*/

void FuncStruct::set_return_type(std::string return_type) {
	this->return_type = return_type;
}
/*void FuncStruct::set_return_type(std::string return_type);*/

std::string FuncStruct::get_name() const {
	return this->name;
}
/*std::string FuncStruct::get_name() const;*/

void FuncStruct::set_name(std::string name) {
	this->name = name;
}
/*void FuncStruct::set_name(std::string name);*/

std::vector<std::string> FuncStruct::get_namespaces() const {
	return this->namespaces;
}
/*std::vector<std::string> FuncStruct::get_namespaces() const*/

void FuncStruct::set_namespaces(std::vector<std::string> namespaces) {
	this->namespaces = namespaces;
}
/*void FuncStruct::set_namespaces(std::vector<std::string> namespaces);*/

void FuncStruct::add_namespace(std::string new_namespace) {
	this->namespaces.push_back(new_namespace);
}
/*void FuncStruct::add_namespace(std::string new_namespace);*/

void FuncStruct::set_def_line(int line) {
	def_line = line;
}
/*void FuncStruct::set_def_line(int line);*/

int FuncStruct::get_def_line() const {
	return def_line;
}
/*int FuncStruct::get_def_line() const;*/

void FuncStruct::set_id(int id) {
	func_id = id;
}
/*void FuncStruct::set_id(int id);*/

int FuncStruct::get_id() {
	return func_id;
}
/*int FuncStruct::get_id();*/
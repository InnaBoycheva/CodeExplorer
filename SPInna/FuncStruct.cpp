#include "FuncStruct.h"

FuncStruct::FuncStruct()
{
}

FuncStruct::~FuncStruct()
{
}

std::string FuncStruct::get_return_type() const {
	return this->return_type;
}

void FuncStruct::set_return_type(std::string return_type) {
	this->return_type = return_type;
}

std::string FuncStruct::get_name() const {
	return this->name;
}

void FuncStruct::set_name(std::string name) {
	this->name = name;
}

/*std::string FuncStruct::get_class_name() {
	return class_name;
}

void FuncStruct::set_class_name(std::string class_name) {
	this->class_name = class_name;
}*/

/*std::vector<std::pair<std::string, std::string>> FuncStruct::get_args() {
	return args;
}

void FuncStruct::add_arg(std::pair<std::string, std::string> arg) {
	this->args.push_back(arg);
}*/

std::vector<std::string> FuncStruct::get_namespaces() const {
	return this->namespaces;
}

void FuncStruct::set_namespaces(std::vector<std::string> namespaces) {
	this->namespaces = namespaces;
}

void FuncStruct::add_namespace(std::string new_namespace) {
	this->namespaces.push_back(new_namespace);
}

void FuncStruct::set_def_line(int line) {
	def_line = line;
}

int FuncStruct::get_def_line() const {
	return def_line;
}
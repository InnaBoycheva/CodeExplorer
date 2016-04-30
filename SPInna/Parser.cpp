#include "Parser.h"
#include <sstream>
#include <iterator>

using namespace std;

Parser::Parser()
{
	// while parsing lines, we can have multiline comments
	// when scanning initially, assume it is not a comment
	comment = false;
	current_line = 0;
}

Parser::~Parser()
{
}

bool Parser::find_balanced(vector<token>::reverse_iterator& it, string start_with, string end_with) {

	int numChars = 1;
	string next;
	do {
		next = *++it;
		if (next == start_with) ++numChars;
		if (next == end_with) --numChars;
	} while (numChars != 0 && it != stack.rend());

	// Return true if balanced/false otherwise
	return numChars == 0;

}

void Parser::scan_line(string line, Node* node) {
	++current_line;

	const char* c = line.c_str();
	string content;

	// Disregard preprocessor directives
	if (*c == '#') return;

	// Multiline comment
	if (comment) {
		while (*c != '\0') {
			while (*c != '*' && *c != '\0') {
				c++;
			}
			if (*c == '\0') return;
			c++;
			if (*c == '/') {
				comment = false;
				c++;
				break;
			}
		}
	}

	for (c; *c != '\0'; ++c) {
		switch (*c) {
			case ';':
			case '{':
			case '(':
			case ')':
			case ',':
			case '>':
			case '<':
			case '[':
			case ']':
				if (!content.empty()) {
					stack.push_back(token(content,current_line));
				}
				content = *c;
				stack.push_back(token(content, current_line));
				content = "";
				break;
			case '}':

				// Start from beginning of stack until encountering {
				if (!content.empty()) {
					stack.push_back(token(content, current_line));
				}
				content = *c;
				stack.push_back(token(content, current_line));
				content = "";
				loc = stack.rbegin();

				if (find_balanced(loc, "}", "{")) {
					// Now parse
					loc++;
					parse(node);
				}
				break;

			// whitespace
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case '\v':
			case '\f':
				if (!content.empty()) {
					stack.push_back(content);
				}
				content = "";
				break;
			// Multiline comment /* AND /**/
			case '/':
				if (*(c + 1) == '/') return;
				if (*(c + 1) == '*') {
					comment = true;
					++c;
					while (*c != '\0') {
						if (*c == '*' && *(c + 1) == '/') {
							comment = false;
							c += 2;
							break;
						}
						++c;
					}
					if (*c == '\0') return;
				}
			default:
				content += *c;
				break;
		}
	}
	if (!content.empty()) {
		stack.push_back(content);
	}
}

void Parser::add_class(Node* node) {
	auto rit = loc;
	while (rit != stack.rend() && *rit != "}" && *rit != ";") {
		rit++;
	}
	if (*--rit == "class" || *rit == "struct") {
		string class_name = *(rit - 1);
		// save class/structure as it is defined in this file
		node->defined_classes.push_back(class_name);
	}
}

string Parser::find_function_name(FuncStruct func_struct, const char* c) {

	string name_component;

	for (c; *c != '\0'; ++c) {
		switch (*c) {
			case ':':
				if (!name_component.empty()) {
					func_struct.add_namespace(name_component);
				}
				name_component = "";
				break;
			default:
				name_component += *c;
		}
	}

	return name_component;
}

void Parser::parse(Node* node) {

	// If not ) - class / struct / lambda
	if (loc->str != ")") {

		// Ignore if lambda func
		auto rit = loc;
		while (rit != stack.rend() && rit->str != ";" && rit->str != "}") {
			if (rit->str == ">") {
				string prev = (rit + 1)->str;
				if (prev[prev.size() - 1] == '-') {
					// lambda of form "-> return_type {...}"
					return;
				}
			}
			rit++;
		}

		// Add to defined classes in file
		add_class(node);
		return;
	}

	std::vector<string> popped;

	// Try to keep number of parens balanced
	int numBrace = 1;
	string next;
	do {
		next = (++loc)->str;
		if (next == ")") ++numBrace;
		if (next == "(") --numBrace;
		popped.push_back(next);
	} while (numBrace != 0);

	// pushed one extra, pop to balance
	loc++;
	popped.pop_back();

	// Lambda w/ no return type - ignore
	if ( loc->str == "]" ) {
		return;
	}

	// Disregard control structures
	next = loc->str;
	if ( next != "if" && next != "for" &&
		 next != "else" && next != "switch" &&
		 next != "while") {

		FuncStruct func_struct;

		// Function name
		const char* c = next.c_str();
		string function_name = find_function_name(func_struct, c);
		
		// black voodoo magic to decide if it's a class or namespace
		// if (func_struct.namespaces.back() is class) func_struct.set_class_name(func_struct.namespaces.back()); func_struct.namespaces.pop_back();
		if (!function_name.empty()) {
			func_struct.set_name(function_name);
		}

		// Arguments processing
		auto args = combine_template_args(popped);
		if (!args.empty()) {
			auto arguments = prepare_args(args);
			auto it = arguments.begin();
			while (it != arguments.end()) {
				func_struct.args.push_back(*it);
				it++;
			}
		}

		popped.clear();

		loc++;
		while (loc != stack.rend() && *loc != "}" && *loc != ";") {
			popped.push_back(*loc);
			loc++;
		}

		string ret_type;
		if (!popped.empty()) {
			auto rit = popped.rbegin();
			while (rit != popped.rend()) {
				ret_type += *rit;
				rit++;
			}
			func_struct.set_return_type(ret_type);
			if (loc != stack.rend()) loc++;
		}
		
		// Save structure of the function
		node->functions.push_back(func_struct);
	} 
}

std::vector<Parser::string_or_vec> Parser::combine_template_args(std::vector<string> split) {
	vector<string_or_vec> temp;
	
	auto rit = split.begin();
	while (rit != split.end()) {
		string_or_vec str;
		str.str = *rit;
		str.is_vec = false;
		temp.push_back(str);
		if (*rit++ == "<") {
			string_or_vec vec;
			vec.is_vec = true;
			while (temp.back().is_vec || temp.back().str != ">") {
				vec.vec.push_back(temp.back());
				temp.pop_back();
			}
			vec.vec.push_back(temp.back());
			temp.pop_back();
			temp.push_back(vec);
		}
	}

	return temp;
}

std::vector<FuncStruct::arg_struct> Parser::prepare_args(const std::vector<Parser::string_or_vec>& args) {

	vector<FuncStruct::arg_struct> all_args;
	FuncStruct::arg_struct arg;
	std::vector<string_or_vec> popped;

	string_or_vec str;
	auto rit = args.rbegin();
	while (rit != args.rend()) {
		str = *rit;
		if (str.str == ",") {
			arg = prepare_single_arg(popped);
			all_args.push_back(arg);
			popped.clear();
		} else {
			popped.push_back(str);
		}
		*rit++;
	}

	arg = prepare_single_arg(popped);
	all_args.push_back(arg);

	return all_args;
}

FuncStruct::arg_struct Parser::prepare_single_arg(const std::vector<Parser::string_or_vec> popped) {

	FuncStruct::arg_struct arg;
	string temp;

	auto it = popped.begin();
	while (it + 1 != popped.end()) {
		if ((*it).is_vec) {
			auto jt = (*it).vec.begin();
			while (jt != (*it).vec.end()) {
				temp += (*jt).str;
				jt++;
			}
		}
		temp += (*it).str;
		it++;
	}
	arg.name = popped.back().str;
	arg.data_type = temp;

	return arg;
}
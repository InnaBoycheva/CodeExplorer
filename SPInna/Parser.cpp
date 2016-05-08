#include "Parser.h"
#include <sstream>
#include <iterator>

using namespace std;

Parser::Parser()
{
	// while parsing lines, we can have multiline comments
	// when scanning initially, assume it is not a comment
	comment = false;
	midstring = false;
	current_line = 0;
}

Parser::~Parser()
{
}

bool Parser::find_balanced(vector<token>::reverse_iterator& it, string start_with, string end_with) {

	int numChars = 1;
	string next;
	do {
		next = (++it)->str;
		if (next == start_with) ++numChars;
		if (next == end_with) --numChars;
	} while (numChars != 0 && it != stack.rend());

	// Return true if balanced/false otherwise
	return numChars == 0;

}

void Parser::scan_line(string line, Node* node, bool is_parse_ii) {
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
		if (midstring && (*c != string_type || (*(c-1) == '\\' && *(c-2) != '\\'))) {
			content += *c;
			continue;
		}
		switch (*c) {
			case '"':
			case '\'':
				midstring = !midstring;
				string_type = *c;
				content += string_type;
				if (!midstring) {
					stack.push_back(token(content, current_line));
					content = "";
				}
				break;
			case ')':
				if (is_parse_ii) {
					// Start from beginning of stack until encountering (
					if (!content.empty()) {
						stack.push_back(token(content, current_line));
					}
					content = *c;
					stack.push_back(token(content, current_line));
					content = "";
					loc = stack.rbegin();

					if (find_balanced(loc, ")", "(")) {
						// Now parse
						loc++;
						parse_ii(node);
					}
					break;
				}
			case ';':
			case '{':
			case '(':
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
					stack.push_back(token(content, current_line));
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
				content += *c;
				break;
			case '-':
			case '.':
			case ':':
				if (is_parse_ii) {
					if (!content.empty()) {
						stack.push_back(token(content, current_line));
					}
					content = *c;
					stack.push_back(token(content, current_line));
					content = "";
					break;
				}
			default:
				content += *c;
				break;
		}
	}
	if (!content.empty()) {
		stack.push_back(token(content, current_line));
	}
}

void Parser::add_class(Node* node) {

	auto rit = loc;
	while (rit != stack.rend()) {
		if (rit->str == "}" || rit->str == ";") {
			return;
		}
		if (rit->str == "class" || rit->str == "struct") {
			string class_name = (rit - 1)->str;
			// save class/structure
			node->defined_classes.push_back(class_name);
			return;
		}
		rit++;
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

	std::vector<token> popped;

	// Try to keep number of parens balanced
	int numBrace = 1;
	vector<token>::reverse_iterator next;
	do {
		next = ++loc;
		if (next->str == ")") ++numBrace;
		if (next->str == "(") --numBrace;
		popped.push_back(*next);
	} while (numBrace != 0);

	// pushed one extra, pop to balance
	loc++;
	popped.pop_back();

	// Lambda w/ no return type - ignore
	if ( loc->str == "]" ) {
		return;
	}

	// Disregard basic control structures
	next = loc;
	if (next->str != "if" && next->str != "for" &&
		next->str != "else" && next->str != "switch" &&
		next->str != "while") {

		FuncStruct func_struct;

		// Function name
		const char* c = next->str.c_str();
		string function_name = find_function_name(func_struct, c);
		
		// black voodoo magic to decide if it's a class or namespace
		// if (func_struct.namespaces.back() is class) func_struct.set_class_name(func_struct.namespaces.back()); func_struct.namespaces.pop_back();
		if (!function_name.empty()) {
			func_struct.set_name(function_name);
			func_struct.set_def_line(next->line);
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
		while (loc != stack.rend() && loc->str != "}" && loc->str != ";" && loc->str != "{") {
			popped.push_back(*loc);
			loc++;
		}

		string ret_type;
		if (!popped.empty()) {
			auto rit = popped.rbegin();
			while (rit != popped.rend()) {
				ret_type += rit->str;
				rit++;
			}
			func_struct.set_return_type(ret_type);
			if (loc != stack.rend()) loc++;
		}
		
		// Save structure of the function
		node->functions.push_back(func_struct);
	} 
}

std::vector<Parser::token_or_vec> Parser::combine_template_args(std::vector<token> split) {
	vector<token_or_vec> temp;
	
	auto rit = split.begin();
	while (rit != split.end()) {
		token_or_vec tok;
		tok.tok = *rit;
		tok.is_vec = false;
		temp.push_back(tok);
		if ((rit++)->str == "<") {
			token_or_vec vec;
			vec.is_vec = true;
			while (temp.back().is_vec || temp.back().tok.str != ">") {
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

std::vector<FuncStruct::arg_struct> Parser::prepare_args(const std::vector<token_or_vec>& args) {

	vector<FuncStruct::arg_struct> all_args;
	FuncStruct::arg_struct arg;
	std::vector<token_or_vec> popped;

	token_or_vec tok;
	auto rit = args.rbegin();
	while (rit != args.rend()) {
		tok = *rit;
		if (tok.tok.str == ",") {
			arg = prepare_single_arg(popped);
			all_args.push_back(arg);
			popped.clear();
		} else {
			popped.push_back(tok);
		}
		*rit++;
	}

	arg = prepare_single_arg(popped);
	all_args.push_back(arg);

	return all_args;
}

FuncStruct::arg_struct Parser::prepare_single_arg(const std::vector<token_or_vec> popped) {

	FuncStruct::arg_struct arg;
	string temp;

	auto it = popped.begin();
	while (it + 1 != popped.end()) {
		if ((*it).is_vec) {
			auto jt = (*it).vec.begin();
			while (jt != it->vec.end()) {
				temp += jt->tok.str;
				jt++;
			}
		}
		temp += it->tok.str;
		it++;
	}
	arg.name = popped.back().tok.str;
	arg.data_type = temp;

	return arg;
}

void Parser::parse_ii(Node* node) {
	if (loc->str != "if" && loc->str != "for" &&
		loc->str != "else" && loc->str != "switch" &&
		loc->str != "while") {
			string potential_func = loc->str;
			FuncStruct func;
			find_function(potential_func, *node, &func);
	}
}

bool Parser::find_function(string potential_func, const Node &node, FuncStruct* res) {
	// find in file
	for (int i = 0; i < node.functions.size(); ++i) {
		if (node.functions[i].get_name() == potential_func) {
			*res = node.functions[i];
			return true;
		}
	}
	// find in includes
	for (int i = 0; i < node.includes.size(); ++i) {
		if (find_function(potential_func, *node.includes[i], res)) {
			return true;
		}
	}
	return false;
}
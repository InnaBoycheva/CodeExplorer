#pragma once
#include <iostream>
#include <vector>
#include "FuncStruct.h"
#include "FilesGraph.h"

class Parser
{
public:
	Parser();
	~Parser();

	void scan_line(std::string line, Node* node);

//private:

	struct token {
		token(std::string s, int i) {
			str = s;
			line = i;
		};
		std::string str;
		int line;
	};

	struct token_or_vec {
		token tok;
		std::vector<token_or_vec> vec;
		bool is_vec;
		token to_token() {
			if (!is_vec) return tok;
			token combined("", INT_MAX);
			for (auto thing : vec) {
				token thing_tok = thing.to_token();
				combined.str += thing_tok.str;
				combined.line = min(combined.line, thing_tok.line);
			}
			return combined;
		}
	};

	std::vector<token>::reverse_iterator loc;
	int current_line;
	std::vector<token> stack;
	bool comment;

	void parse(Node* node);
	std::vector<token_or_vec> combine_template_args(std::vector<token> split);
	void add_class(Node* node);
	std::string find_function_name(FuncStruct func_struct, const char* c);
	bool find_balanced(std::vector<token>::reverse_iterator& it, std::string start_from, std::string end_with);
	void add_args(const std::vector<token_or_vec>& args, FuncStruct& func_struct);
	std::vector<FuncStruct::arg_struct> Parser::prepare_args(const std::vector<token_or_vec>& args);
	FuncStruct::arg_struct prepare_single_arg(const std::vector<token_or_vec> popped);
};

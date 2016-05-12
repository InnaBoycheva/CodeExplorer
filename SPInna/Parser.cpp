#include "Parser.h"
#include <sstream>
#include <iterator>

using namespace std;

/**/
/*
Parser::Parser()

NAME

Parser::Parser() - Constructor for the Parser class.

SYNOPSIS

Parser::Parser();

DESCRIPTION

This function will initialize some class variables. The comments variable, for instance,
has to do with the fact that there can be multiline comments which can cause problems
since the parser is parsing line by line. When searching initially, we assume that we're
not in a comment.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
Parser::Parser() {
	comment = false;
	midstring = false;
	current_line = 0;
}
/*Parser::Parser();*/

Parser::~Parser()
{
}

/**/
/*
bool Parser::find_balanced()

NAME

Parser::find_balanced() - Determines whether two characters can be balanced.

SYNOPSIS

bool Parser::find_balanced(vector<token>::reverse_iterator& it, string start_with, string end_with);
it				--> position in the stack
start_with		-->	starting character
end_with		--> ending character

DESCRIPTION

This function is useful in cases where we have an ending character ( ex: } or ) ) and would
like to find the opening equivalent of it ( ex: { or ( ). Since there might be other chars
of the same type in between the opening & closing chars (ex: if-else structure in a function).
We have to make sure that we do not identify an incorrect occurrence of the character as being 
the opening one. The function will keep count of how many characters like this one occur and
try to find the correct corresponding opening one.

RETURNS

True if the characters are balanced, false otherwise.

AUTHOR

Inna Boycheva

*/
/**/
bool Parser::find_balanced(vector<token>::reverse_iterator& it, string start_with, string end_with) {

	int numChars = 1;
	string next;
	do {
		next = (++it)->str;
		if (next == start_with) ++numChars;
		if (next == end_with) --numChars;
	} while (numChars != 0 && it != stack.rend());

	return numChars == 0;

}
/*bool Parser::find_balanced(vector<token>::reverse_iterator& it, string start_with, string end_with);*/

/**/
/*
void Parser::scan_line()

NAME

Parser::scan_line() - Determines whether two characters can be balanced.

SYNOPSIS

void Parser::scan_line(string line, Node* node, bool is_parse_ii);
node				--> the current node
is_parse_ii			-->	whether this is parse 2 or not

DESCRIPTION

This function scans the file content line by line and character by character.
It splits the line strings based on special characters and pushes the individual
pieces in a vector. During the first pass, the stack vector will not contain any
comments or preprocessing directives since they do not help for establishing the
project structure. During the second parse, everything will be needed since the stack
formed then will be used for displaying the front-end code area part along with
specific annotations.
See individual comments below on what each character separation means.

RETURNS

Void

AUTHOR

Inna Boycheva

*/
/**/
void Parser::scan_line(string line, Node* node, bool is_parse_ii) {

	// Keep track of which line we're on
	++current_line;

	const char* c = line.c_str();
	string content;

	// Disregard preprocessor directives if not second pass
	// Needed for pass 2 since they need to be in the stack to be displayed on the front-end
	if (*c == '#') {
		if (is_parse_ii) {
			stack.push_back(token(c, current_line));
		}
		return;
	}

	// Line part of a multiline comment
	if (comment) {
		// Keep comments in stack for pass 2 (to be displayed on the front-end)
		if (is_parse_ii) {
			stack.push_back(token(comment_content, current_line));
		}
		comment_content = "";
		while (*c != '\0') {
			// Each characted except for * and \0 is irrelevant, since it will be
			// inside of the comment => Move on to something indicating end of comment
			while (*c != '*' && *c != '\0') {
				comment_content += *c;
				c++;
			}
			// End of line
			if (*c == '\0') return;
			// Peek at next character
			c++;
			// If / => end of comment
			if (*c == '/') {
				comment = false;
				comment_content += "*/";
				// Keep entire comment on stack for parse 2
				if (is_parse_ii) {
					stack.push_back(token(comment_content, current_line));
				}
				comment_content = "";
				// Move on to next character
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
				// In parse 2 we're trying to link functions. Look for structures in
				// the code that could match function calls.
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
			// Split by these characters with no special action
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
				// During pass 1, this will be helpful when trying to match function
				// definitions. Each function definition ends with a }.
				if (!content.empty()) {
					stack.push_back(token(content, current_line));
				}
				content = *c;
				stack.push_back(token(content, current_line));
				content = "";
				loc = stack.rbegin();

				// Balance the { } brackets
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
				if (*(c + 1) == '/') {
					if (is_parse_ii) {
						if (!content.empty()) {
							stack.push_back(token(content, current_line));
						}
						// Since c-string, it will copy from // onward.
						content = c;
						stack.push_back(token(content, current_line));
					}
					return;
				}
				if (*(c + 1) == '*') {
					comment = true;
					comment_content += "/*";
					c += 2;
					while (*c != '\0') {
						if (*c == '*' && *(c + 1) == '/') {
							comment = false;
							comment_content += "*/";
							if (is_parse_ii) {
								stack.push_back(token(comment_content, current_line));
							}
							comment_content = "";
							c += 2;
							break;
						} else {
							comment_content += *c;
						}
						++c;
					}
					if (*c == '\0') return;
				}
				content += *c;
				break;
			// During parse 2 split on these characters as well since function calls
			// can be of the format obj.func() or obj->func()
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
/*void Parser::scan_line(string line, Node* node, bool is_parse_ii);*/

/**/
/*
void Parser::add_class()

NAME

Parser::add_class() - Determines whether the current line position is responsible
	for defining a class / structure.

SYNOPSIS

void Parser::add_class(Node* node);
node			--> the current node

DESCRIPTION

This function checks the current stack and if it is a class/struct that is
defined at the current position, it will save it to the appropriate node in
the graph.

RETURNS

Void

AUTHOR

Inna Boycheva

*/
/**/
void Parser::add_class(Node* node) {

	auto rit = loc;
	while (rit != stack.rend()) {
		// Not a class
		if (rit->str == "}" || rit->str == ";") {
			return;
		}
		// Class or a structure
		if (rit->str == "class" || rit->str == "struct") {
			string class_name = (rit - 1)->str;
			// Save class / structure into the node
			node->defined_classes.push_back(class_name);
			return;
		}
		rit++;
	}
}
/*void Parser::add_class(Node* node);*/

/**/
/*
string Parser::find_function_name()

NAME

Parser::find_function_name() - Extracts the name component of a function.

SYNOPSIS

string Parser::find_function_name(FuncStruct func_struct, const char* c);
func_struct			--> the function structure
c					--> the line

DESCRIPTION

This function extracts the name component and the namespace (if any) of a function.
It automatically adds the namespace to the function.

RETURNS

The function name.

AUTHOR

Inna Boycheva

*/
/**/
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
/*string Parser::find_function_name(FuncStruct func_struct, const char* c);*/

/**/
/*
void Parser::parse()

NAME

Parser::parse() - Parses a line looking at specific characters while trying to establish
	certain relationships between the code components.

SYNOPSIS

void Parser::parse(Node* node);
node			--> the current node

DESCRIPTION

This function represents the first parse of the files where we're looking for function
definitions. In the scan_line function, the last thing that we've encountered was the opening
{ brace. Now look at the previous character.
If it is ) -> We potentially have a function definition right there.
If not, it is either a class, a struct, or a lambda function. Lambda functions will be ignored &
if a class / struct, the appropriate name will be added to the node.
If ), balance to the opening ( of the function and try to parse whatever comes before it (as it could
contain a potential function definition infromation). Figure out function name, namespace, return arguments,
and save those into the function structure.

RETURNS

Void

AUTHOR

Inna Boycheva

*/
/**/
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

	// This will contain all tokens popped while trying to balance the ( )
	// parens. If we have a function definition, the tokens will correspond
	// to arguments.
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

	// Pushed one extra, pop to balance
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

		// A function definition
		FuncStruct func_struct;

		// Get function name
		const char* c = next->str.c_str();
		string function_name = find_function_name(func_struct, c);

		// Save function name and definition line
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

		// Free up the popped array
		// Need to use it when figuring out what the return type is
		popped.clear();

		loc++;
		while (loc != stack.rend() && loc->str != "}" && loc->str != ";" && loc->str != "{") {
			popped.push_back(*loc);
			loc++;
		}

		// Return type processing
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
/*void Parser::parse(Node* node);*/

/**/
/*
std::vector<Parser::token_or_vec> Parser::combine_template_args()

NAME

Parser::combine_template_args() - Combines template arguments.

SYNOPSIS

std::vector<Parser::token_or_vec> Parser::combine_template_args(std::vector<token> split)
split			--> the vector of tokens to be splitted

DESCRIPTION

Upon locating a function definition, all function arguments are placed into a vector with
no specific structure whatsoever. This function will combine the templated arguments together
making the identifying of arguments later much easier.

RETURNS

A vector of token_or_vecs bringing meaning to the arguments vector.

AUTHOR

Inna Boycheva

*/
/**/
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
/*std::vector<Parser::token_or_vec> Parser::combine_template_args(std::vector<token> split);*/

/**/
/*
std::vector<FuncStruct::arg_struct> Parser::prepare_args()

NAME

Parser::prepare_args() - Places the arguments into an arguments structure.

SYNOPSIS

std::vector<FuncStruct::arg_struct> Parser::prepare_args(const std::vector<token_or_vec>& args);
args		--> all arguments

DESCRIPTION

After processing the arguments & combining any template ones that might occur, convert the arguments
into a structure containing their name and data type.

RETURNS

A vector of argument structures.

AUTHOR

Inna Boycheva

*/
/**/
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
/*std::vector<FuncStruct::arg_struct> Parser::prepare_args(const std::vector<token_or_vec>& args);*/

/**/
/*
FuncStruct::arg_struct Parser::prepare_single_arg()

NAME

Parser::prepare_single_arg() - Places a single argument into an arguments structure.

SYNOPSIS

FuncStruct::arg_struct Parser::prepare_single_arg(const std::vector<token_or_vec> popped);
popped		--> a vector with all arguments

DESCRIPTION

After processing the arguments & combining any template ones that might occur, convert the arguments
into a structure containing their name and data type.

RETURNS

The argument structure representing the argument.

AUTHOR

Inna Boycheva

*/
/**/
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
/*FuncStruct::arg_struct Parser::prepare_single_arg(const std::vector<token_or_vec> popped);*/

/**/
/*
void Parser::parse_ii()

NAME

Parser::parse_ii() - Second parse of the files.

SYNOPSIS

void Parser::parse_ii(Node* node);
node		--> the current node

DESCRIPTION

In the parse function, we've made sure that at this point we might have a function
call and the current position in the stack of element should point at the function
name. Again, disregard basic control structures & look at the element coming right before
the opening (. If it looks like a function that has been declared in the database, annotate
it and push the annotated version on the stack. Annotations will be useful later on when
linking functions on the front end.

RETURNS

Void

AUTHOR

Inna Boycheva

*/
/**/
void Parser::parse_ii(Node* node) {
	if (loc->str != "if" && loc->str != "for" &&
		loc->str != "else" && loc->str != "switch" &&
		loc->str != "while") {
			string potential_func = loc->str;
			FuncStruct func;
			if (find_function(potential_func, *node, &func)) {
				std::ostringstream openTag;
				openTag << "[func-id=" << func.get_id() << "]";
				// add annotations to db file content
				stack.insert(stack.insert(loc.base(), token("[/func-id]", loc->line)) - 1,
					token(openTag.str(),
								   loc->line));
			}
	}
}
/*void Parser::parse_ii(Node* node);*/

/**/
/*
bool Parser::find_function()

NAME

Parser::find_function() - Checks to see if a potential function.

SYNOPSIS

bool Parser::find_function(string potential_func, const Node &node, FuncStruct* res)
potential_sting		--> the string considered to be a function name
node				--> the current node

DESCRIPTION

This function checks to see if the potential function name is an actual function
saved in the node function defs. If it is not, the function will check if it is a
function defined in some of the includes saved in the node includes.

RETURNS

True if such a function is found. False otherwise.

AUTHOR

Inna Boycheva

*/
/**/
bool Parser::find_function(string potential_func, const Node &node, FuncStruct* res) {

	// Find in file
	for (int i = 0; i < node.functions.size(); ++i) {
		if (node.functions[i].get_name() == potential_func) {
			*res = node.functions[i];
			return true;
		}
	}

	// Find in includes
	for (int i = 0; i < node.includes.size(); ++i) {
		if (find_function(potential_func, *node.includes[i], res)) {
			return true;
		}
	}
	return false;

}
/*bool Parser::find_function(string potential_func, const Node &node, FuncStruct* res);*/

/**/
/*
string Parser::get_content()

NAME

Parser::get_content() - Retrieves indented and specially annotated code.

SYNOPSIS

string Parser::get_content();

DESCRIPTION

This function adds special formatting to the content in the stack. It provides indentation
and control over the character spacing.

RETURNS

The modified code ready for display on the frontend.

AUTHOR

Inna Boycheva

*/
/**/
string Parser::get_content() {
	ostringstream strstr;
	int line_count = 1;
	string indentation = "";
	string last_in_line = "";

	// Require whitespace before.
	vector<string> ws_before { ")", "{" };

	// Require whitespace after.
	vector<string> ws_after { "(", "," };

	// Require no whitespace.
	vector<string> special_tokens { ":", "<", ">", ";", ".", "]", "[" };

	special_tokens.insert(special_tokens.end(), ws_before.begin(), ws_before.end());
	special_tokens.insert(special_tokens.end(), ws_after.begin(), ws_after.end());
	for (int i = 0; i < stack.size(); i++) {
		if (stack[i].line != line_count) {
			while (line_count < stack[i].line) {
				strstr << "\n";
				line_count++;
			}
			if (stack[i].str == "{") {
				strstr << indentation;
				indentation += "\t";
			} else if (stack[i].str == "}") {
				indentation = indentation.substr(0, indentation.length() - 1);
				strstr << indentation;
			} else {
				strstr << indentation;
			}
			last_in_line = "";
		} else {
			if ((std::find(special_tokens.begin(), special_tokens.end(), last_in_line) == special_tokens.end() &&
				std::find(special_tokens.begin(), special_tokens.end(), stack[i].str) == special_tokens.end()) ||
				std::find(ws_before.begin(), ws_before.end(), stack[i].str) != ws_before.end()) {
				// If two non-special tokens or a special token that requires ws before it.
				strstr << " ";
			}
			// We still need to check indentation for brackets on same line.
			if (stack[i].str == "{") {
				indentation += "\t";
			} else if (stack[i].str == "}") {
				indentation = indentation.substr(0, indentation.length() - 1);
			}
		}
		strstr << stack[i].str;
		if (std::find(ws_after.begin(), ws_after.end(), stack[i].str) != ws_after.end()) {
			strstr << " ";
		}
		last_in_line = stack[i].str;
	}
	return strstr.str();
}
/*string Parser::get_content();*/
#include "stdafx.h"
#include "Parser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

TEST_CLASS(ParserTest)
{
public:
		
	TEST_METHOD(TestCommentsParse)
	{
		Parser parser;
		Node n;
		parser.scan_line( "vector<Node*> Node::getNeighbors() {", &n );
		Assert::IsFalse(parser.comment, L"1");
		parser.scan_line("/*", &n);
		Assert::IsTrue(parser.comment, L"2");
		parser.scan_line("vector<Node*> Node::getNeighbors() {", &n);
		Assert::AreEqual(size_t(8), parser.stack.size(), L"parsed inside of comment");
		parser.scan_line("*/ function", &n);
		Assert::IsFalse(parser.comment, L"3");
		Assert::AreEqual("function", parser.stack.back().c_str(), L"ignored after comment");
		Assert::AreNotEqual("/", (parser.stack.rbegin()+1)->c_str(), L"added unnecessary / char");

		// more comment weirdness
		parser.scan_line("/**/", &n);
		Assert::IsFalse(parser.comment, L"single line w/ star");

		parser.stack.clear();

		parser.scan_line("	/*", &n);
		Assert::IsTrue(parser.comment, L"whitespace with comment");
		Assert::IsTrue(parser.stack.empty(), L"failed to ignore comment");
	}

	TEST_METHOD(TestLambda)
	{
		Parser parser;
		Node n;
		parser.scan_line("std::transform(v.begin(), v.end(), v.begin(),", &n);
		parser.scan_line("[epsilon](double d) -> double {", &n);
		parser.scan_line("	if (d < epsilon) {", &n);
		parser.scan_line("		return 0;", &n);
		parser.scan_line("	} else {", &n);
		parser.scan_line("		return d;", &n);
		parser.scan_line("	}", &n);
		parser.scan_line("});", &n);
		Assert::IsTrue(n.functions.empty(), L"Lambda added as defined func (/w return val)");

		parser.scan_line("std::transform(v.begin(), v.end(), v.begin(),", &n);
		parser.scan_line("[epsilon](double d) {", &n);
		parser.scan_line("	if (d < epsilon) {", &n);
		parser.scan_line("		return 0;", &n);
		parser.scan_line("	} else {", &n);
		parser.scan_line("		return d;", &n);
		parser.scan_line("	}", &n);
		parser.scan_line("});", &n);
		Assert::IsTrue(n.functions.empty(), L"Lambda added as defined func (no return val)");
	}

	TEST_METHOD(TestTemplateReturnType) {
		Parser parser;
		Node n;
		parser.scan_line("function<double(double, double)> func_name(string arg) {", &n);
		parser.scan_line("}", &n);
		Assert::AreEqual("function<double(double,double)>", n.functions[0].get_return_type().c_str(), L"Didn't catch template return type");
	}



};
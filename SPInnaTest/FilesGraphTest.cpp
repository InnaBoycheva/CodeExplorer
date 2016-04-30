#include "stdafx.h"
#include "FilesGraph.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

TEST_CLASS(FilesGraphTest)
{
public:
		
	TEST_METHOD(TestGetIncludeLine)
	{
		FilesGraph graph;

		// not valid
		string actual = graph.get_include("not an include line");
		string expected = "";
		Assert::AreEqual(expected, actual, L"they ain't equal");
		actual = graph.get_include("#pragma once");
		Assert::AreEqual(expected, actual, L"they ain't equal");

		// with quotes
		actual = graph.get_include("#include \"some_include\"");
		expected = "\"some_include\"";
		Assert::AreEqual(expected, actual, L"they ain't equal");

		// with brackets
		actual = graph.get_include("#include <iostream>");
		expected = "<iostream>";
		Assert::AreEqual(expected, actual, L"they ain't equal");
	}

	TEST_METHOD(TestAddInclude)
	{
		FilesGraph graph;

		Node n;
		graph.add_include("\"some_include\"", "C:\\some\\folder\\my_file.cpp", n);
		Assert::IsFalse(n.includes.empty());
		Assert::AreEqual("C:\\some\\folder\\some_include", n.includes[0]->name.c_str(), L"not cool");
	}

};
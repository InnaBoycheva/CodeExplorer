#pragma once
#include <memory>
#include <vector>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace std;

// projects - id, base path
// files_project_id - name of file, id of file, file content
// classes - file_id, class name, namespace
// functions_project_id - function id, function name, class id (null if not defined),
//		namespace id (only populate if class_id = null), return type, def_line, declaration_line
// arguments_project_id - function id, arg name, data type

class DBConnection
{
public:
	struct table_row {
		std::vector<std::string> fields;
	};

	DBConnection(std::string db_name);

	void setup_projects_table();
	int find_project(std::string path);
	void drop_table(std::string table);
	void create_table(std::string table, table_row columns);
	void reset_table(std::string table, table_row columns);
	bool insert_rows(std::string table, table_row columns, std::vector<table_row> rows);
	int get_entry_id(std::string table, std::string column, std::string search_for);

private:
	sql::Driver *driver;
	std::unique_ptr<sql::Connection> con;
	std::unique_ptr<sql::Statement> stmt;
	std::unique_ptr<sql::ResultSet> res;
	sql::PreparedStatement *pstmt;
	std::string db_name;
};

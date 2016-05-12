#include "DBConnection.h"

using namespace std;

/**/
/*
DBConnection::DBConnection()

NAME

DBConnection::DBConnection() - creates a connection to a selected database.

SYNOPSIS

DBConnection::DBConnection(string db_name);
db_name        --> the name of the database to connect to.

DESCRIPTION

This function will attempt to connect to the database provided.
It will also set up the initial projects table that will contain all
projects analyzed by the program.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
DBConnection::DBConnection(string db_name) {

	try {

		// Create a connection
		driver = get_driver_instance();
		con.reset(driver->connect("tcp://127.0.0.1:3306", "root", "mysql"));

		// Connect to the selected MySQL database
		con->setSchema(db_name.c_str());

		this->db_name = db_name;

		// Setup the initial projects table
		setup_projects_table();

	} catch (sql::SQLException &e) {
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line "
			<< __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
}
/*DBConnection::DBConnection(string db_name);*/

/**/
/*
int DBConnection::find_project()

NAME

DBConnection::find_project() - retrieves the id of a project stored in the database
	based on a provided path.

SYNOPSIS

int DBConnection::find_project(string path);
path        --> the path to the project.

DESCRIPTION

This function will try to retrieve the ID of a project based on a project path.
If the path query does not produce any results, this means that the project has not 
been added yet, so the function will add it and return the corresponding ID.

RETURNS

The id of the project as stored in the database.

AUTHOR

Inna Boycheva

*/
/**/
int DBConnection::find_project(string path) {

	// The project ID
	int id;

	// Perform the query with the provided path
	string query = "SELECT id FROM projects WHERE base_path=?";
	pstmt = con->prepareStatement(query.c_str());
	pstmt->setString(1, path.c_str());
	res.reset(pstmt->executeQuery());

	if (!res->next()) {

		// Project does not exist in DB yet. Add it
		pstmt = con->prepareStatement("INSERT INTO projects(base_path) VALUES (?)");
		pstmt->setString(1, path.c_str());
		pstmt->execute();

		// Retrieve ID of newly inserted project
		stmt.reset(con->createStatement());
		sql::ResultSet* res = stmt->executeQuery("SELECT @@identity AS id");
		res->next();
		id = res->getInt("id");

	} else {

		// Query successful. Grab the ID.
		id = res->getInt("id");

	}
	
	return id;
}
/*int DBConnection::find_project(string path);*/

/**/
/*
void DBConnection::setup_projects_table()

NAME

DBConnection::setup_projects_table() - creates the projects table.

SYNOPSIS

void DBConnection::setup_projects_table();

DESCRIPTION

This function will perform a check to see if a projects table exists. If
it does not, one will be created.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void DBConnection::setup_projects_table() {
	string query = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='" + db_name + "' AND table_name = 'projects'";
	pstmt = con->prepareStatement(query.c_str());
	res.reset(pstmt->executeQuery());
	if (res->next() && res->getInt(1) == 0) {
		// No projects table exists yet. Create one.
		stmt.reset(con->createStatement());
		stmt->execute("CREATE TABLE projects(id INT, base_path VARCHAR(255))");
	}
}
/*void DBConnection::setup_projects_table();*/

/**/
/*
void DBConnection::drop_table()

NAME

DBConnection::drop_table() - drops a table in the database.

SYNOPSIS

void DBConnection::drop_table(string table);
table		--> the table to be dropped.

DESCRIPTION

This function will drop a table specified by the user (if it exists).

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void DBConnection::drop_table(string table) {
	stmt.reset(con->createStatement());
	string query = "DROP TABLE IF EXISTS " + table;
	stmt->execute(query.c_str());
}
/*void DBConnection::drop_table(string table);*/

/**/
/*
void DBConnection::create_table()

NAME

DBConnection::create_table() - creates a table.

SYNOPSIS

void DBConnection::create_table(string table, table_row columns);
table		--> the name of the table to be created.
columns		--> the table column headers.

DESCRIPTION

This function will create a table based on a provided name and a set of column headers.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void DBConnection::create_table(string table, table_row columns) {
	stmt.reset(con->createStatement());
	string query = "CREATE TABLE " + table + "(";
	for (auto column : columns.fields) {
		query += column.string_val + ",";
	}
	query = query.substr(0, query.size() - 1);
	query += ")";
	stmt->execute(query.c_str());
}
/*void DBConnection::create_table(string table, table_row columns);*/

/**/
/*
void DBConnection::reset_table()

NAME

DBConnection::reset_table() - resets a table.

SYNOPSIS

void DBConnection::reset_table(string table, table_row columns);
table		--> the table to be reset.
columns		--> the new column headers.

DESCRIPTION

This function will "reset" a table by dropping it and re-creating it immediately
after.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void DBConnection::reset_table(string table, table_row columns) {
	drop_table(table);
	create_table(table, columns);
}
/*void DBConnection::reset_table(string table, table_row columns);*/

/**/
/*
void DBConnection::insert_rows()

NAME

DBConnection::insert_rows() - insert a set of rows into a table.

SYNOPSIS

void DBConnection::insert_rows(string table, table_row columns, vector<table_row> rows);
table		--> the table in which the rows will be inserted.
columns		--> the table column headers.
rows		--> all rows to be inserted.

DESCRIPTION

This function will insert multiple rows into a table one by one.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void DBConnection::insert_rows(string table, table_row columns, vector<table_row> rows) {
	for (auto row : rows) {
		insert_row(table, columns, row);
	}
}
/*void DBConnection::insert_rows(string table, table_row columns, vector<table_row> rows);*/

/**/
/*
int DBConnection::insert_row()

NAME

DBConnection::insert_row() - inserts a single row.

SYNOPSIS

int DBConnection::insert_row(string table, table_row columns, table_row row);
table		--> the table in which the row will be inserted.
columns		--> the table column headers.
row			--> the row to be inserted.

DESCRIPTION

This function will insert a single row into a table and return its assigned id.

RETURNS

Returns the id assigned to the newly added row.

AUTHOR

Inna Boycheva

*/
/**/
int DBConnection::insert_row(string table, table_row columns, table_row row) {

	// Prepare the statement
	string query = "INSERT INTO " + table + "(";
	for (auto column : columns.fields) {
		query += column.string_val + ",";
	}
	query = query.substr(0, query.size() - 1);
	query += ") VALUES(";

	for (int i = 0; i < columns.fields.size(); ++i) {
		query += "?,";
	}
	query = query.substr(0, query.size() - 1);
	query += ")";
	pstmt = con->prepareStatement(query.c_str());

	// Row entries added could be of mixed type (ex: int + string)
	// Prepare statement using appropriate data type based on entry types
	for (int i = 0; i < row.fields.size(); ++i) {
		table_entry entry = row.fields[i];
		switch (entry.type) {
			case table_entry::STRING:
				pstmt->setString(i + 1, entry.string_val.c_str());
				break;
			case table_entry::INT:
				pstmt->setInt(i + 1, entry.int_val);
				break;
		}
	}

	// Execute the query
	pstmt->execute();

	// Return the corresponding id
	sql::ResultSet* res = stmt->executeQuery("SELECT @@identity AS id");
	res->next();
	return res->getInt("id");
}
/*int DBConnection::insert_row(string table, table_row columns, table_row row);*/

/**/
/*
void DBConnection::update_row()

NAME

DBConnection::update_row() - updates a table row.

SYNOPSIS

void DBConnection::update_row(string table, int id, table_row columns, table_row row);
table		--> the table in which the row will be updated.
id			--> the id of the entry to be updated.
columns		--> the table column headers.
row			--> the new row values.

DESCRIPTION

This function will update a single table row.

RETURNS

void

AUTHOR

Inna Boycheva

*/
/**/
void DBConnection::update_row(string table, int id, table_row columns, table_row row) {

	// Prepare statement
	string query = "UPDATE " + table + " SET ";
	for (auto column : columns.fields) {
		query += column.string_val + "=?,";
	}
	query = query.substr(0, query.size() - 1);
	query += " WHERE id=?";
	pstmt = con->prepareStatement(query.c_str());

	// Row entries added could be of mixed type (ex: int + string)
	// Prepare statement using appropriate data type based on entry types
	for (int i = 0; i < row.fields.size(); ++i) {
		table_entry entry = row.fields[i];
		switch (entry.type) {
			case table_entry::STRING:
				pstmt->setString(i + 1, entry.string_val.c_str());
				break;
			case table_entry::INT:
				pstmt->setInt(i + 1, entry.int_val);
				break;
		}
	}

	// ID of entry to be updated
	pstmt->setInt(row.fields.size() + 1, id);

	// Execute query
	pstmt->execute();
}
/*void DBConnection::update_row(std::string table, int id, table_row columns, table_row row);*/

/**/
/*
int DBConnection::get_entry_id()

NAME

DBConnection::get_entry_id() - gets an entry id.

SYNOPSIS

int DBConnection::get_entry_id(string table, string column, string search_for);
table				--> the table where the entry is located.
column				--> the column by which the entry is filtered.
search_for			--> the value to search for.

DESCRIPTION

This function will query the database looking for a specific entry in an attempt
to retrieve its id.

RETURNS

Return the id of the entry (if found), 0 otherwise.

AUTHOR

Inna Boycheva

*/
/**/
int DBConnection::get_entry_id(string table, string column, string search_for) {
	string query = "SELECT * FROM " + table + " WHERE " + column + "=?";
	pstmt = con->prepareStatement(query.c_str());
	pstmt->setString(1,search_for.c_str());
	res.reset(pstmt->executeQuery());
	if (res->next()) {
		return res->getInt("id");
	}
	return 0;
}
/*int DBConnection::get_entry_id(string table, string column, string search_for);*/
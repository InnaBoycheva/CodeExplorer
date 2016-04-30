#include "DBConnection.h"

using namespace std;

DBConnection::DBConnection(string db_name) {

	try {

		// Create a connection
		driver = get_driver_instance();
		con.reset(driver->connect("tcp://127.0.0.1:3306", "root", "mysql"));

		// Connect to the selected MySQL database
		con->setSchema(db_name.c_str());

		this->db_name = db_name;

		// Setup "projects" table
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

int DBConnection::find_project(string path) {

	int id;

	string query = "SELECT id FROM projects WHERE base_path=?";
	pstmt = con->prepareStatement(query.c_str());
	pstmt->setString(1, path.c_str());
	res.reset(pstmt->executeQuery());

	if (!res->next()) {
		// Project does not exist in DB; Add it
		pstmt = con->prepareStatement("INSERT INTO projects(base_path) VALUES (?)");
		pstmt->setString(1, path.c_str());
		pstmt->execute();
		// Retrieve id of just inserted project
		stmt.reset(con->createStatement());
		sql::ResultSet* res = stmt->executeQuery("SELECT @@identity AS id");
		res->next();
		id = res->getInt("id");
	} else {
		id = res->getInt("id");
	}
	
	return id;
}

void DBConnection::setup_projects_table() {
	string query = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='" + db_name + "' AND table_name = 'projects'";
	pstmt = con->prepareStatement(query.c_str());
	res.reset(pstmt->executeQuery());
	if (res->next() && res->getInt(1) == 0) {
		// No projects table exists yet; Create one
		stmt.reset(con->createStatement());
		stmt->execute("CREATE TABLE projects(id INT, base_path VARCHAR(255))");
	}
}

void DBConnection::drop_table(string table) {
	stmt.reset(con->createStatement());
	string query = "DROP TABLE IF EXISTS " + table;
	stmt->execute(query.c_str());
}

void DBConnection::create_table(string table, table_row columns) {
	stmt.reset(con->createStatement());
	string query = "CREATE TABLE " + table + "(";
	for (auto column : columns.fields) {
		query += column + ",";
	}
	query = query.substr(0, query.size() - 1);
	query += ")";
	stmt->execute(query.c_str());
}

void DBConnection::reset_table(string table, table_row columns) {
	drop_table(table);
	create_table(table, columns);
}

bool DBConnection::insert_rows(string table, table_row columns, vector<table_row> rows) {
	string query = "INSERT INTO " + table + "(";
	for (auto column : columns.fields) {
		query += column + ",";
	}
	query = query.substr(0, query.size() - 1);
	query += ") VALUES(";
	
	for (int i = 0; i < columns.fields.size(); ++i) {
		query += "?,";
	}
	query = query.substr(0, query.size() - 1);
	query += ")";
	pstmt = con->prepareStatement(query.c_str());

	for (auto row : rows) {
		for (int i = 0; i < row.fields.size(); ++i) {
			pstmt->setString(i+1, row.fields.at(i).c_str());
		}
		pstmt->execute();
	}

	return true;
}

int DBConnection::get_entry_id(string table, string column, string search_for) {
	string query = "SELECT * FROM " + table + " WHERE " + column + "=?";
	pstmt = con->prepareStatement(query.c_str());
	pstmt->setString(1,search_for);
	res.reset(pstmt->executeQuery());
	if (res->next()) {
		return res->getInt("id");
	}
	return 0;
}
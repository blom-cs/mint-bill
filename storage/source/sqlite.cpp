/*******************************************************************************
 * @file    sqlite.cpp
 * @brief   Turso connection wrapper. No encryption and no keyring.
 ******************************************************************************/
#include <algorithm>
#include <cctype>
#include <cstring>
#include <errors.h>
#include <sqlite.h>
#include <syslog.h>


namespace {
bool is_retryable(int code, sqlite3 *database)
{
	if (code == SQLITE_BUSY)
	{
		return true;
	}

	const char *message{sqlite3_errmsg(database)};
	if (message == nullptr)
	{
		return false;
	}

	std::string text{message};
	std::transform(text.begin(), text.end(), text.begin(), [] (unsigned char ch) {
		return static_cast<char> (std::tolower(ch));
	});
	// Turso rolls these back (or leaves them stale) and expects the whole
	// transaction to be retried. The C API reports them as SQLITE_ERROR.
	return text.find("conflict") != std::string::npos
		|| text.find("stale") != std::string::npos
		|| text.find("commit dependency") != std::string::npos;
}

bool configure(sqlite3 *database)
{
	const char *pragmas[] = {
		"PRAGMA foreign_keys=ON;",
		"PRAGMA journal_mode=mvcc;",
		"PRAGMA synchronous=NORMAL;",
		"PRAGMA busy_timeout=5000;",
	};
	for (const char *sql : pragmas)
	{
		if (sqlite3_exec(database, sql, nullptr, nullptr, nullptr) != SQLITE_OK)
		{
			syslog(LOG_CRIT, "SQLITE: failed to execute %s - filename %s, line number %d",
					sql, __FILE__, __LINE__);
			return false;
		}
	}

	return true;
}
}


storage::database::sqlite::sqlite(const std::string& _path)
{
	if (_path.empty() == true)
	{
		syslog(LOG_CRIT, "SQLITE: invalid parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		throw app::errors::construction;
	}

	const int flags{SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX};
	if (sqlite3_open_v2(_path.c_str(), &this->database, flags, nullptr) != SQLITE_OK)
	{
		sqlite3_close_v2(this->database);
		this->database = nullptr;
		syslog(LOG_CRIT, "SQLITE: failed to open the database - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		throw app::errors::construction;
	}

	if (configure(this->database) == false)
	{
		sqlite3_close_v2(this->database);
		this->database = nullptr;
		throw app::errors::construction;
	}
}

storage::database::sqlite::~sqlite()
{
	if (sqlite3_close_v2(this->database) != SQLITE_OK)
	{
		syslog(LOG_CRIT, "SQLITE: failed to close the database connection - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
}

bool storage::database::sqlite::transaction(const std::string& _sql_query)
{
	if (_sql_query.empty() == true)
	{
		syslog(LOG_CRIT, "SQLITE: invalid parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
	}

	part::sql_operations sql_operations{this->database, _sql_query, &this->last_retryable};
	return sql_operations.single_execute();
}

bool storage::database::sqlite::usert(const std::string& _sql_query, const std::vector<param_values>& _sql_query_params)
{
	if (_sql_query.empty() || _sql_query_params.empty())
	{
		syslog(LOG_CRIT, "SQLITE: invalid parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
	}

	part::sql_operations sql_operations{this->database, _sql_query, &this->last_retryable};
	if (sql_operations.bind_params(_sql_query_params) == false)
	{
		syslog(LOG_CRIT, "SQLITE: failed to bind the parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
	}

	return sql_operations.single_execute();
}

storage::database::part::rows storage::database::sqlite::select(const std::string& _sql_query, const std::vector<param_values>& _sql_query_params)
{
	part::rows rows;
	if (_sql_query.empty() || _sql_query_params.empty())
	{
		syslog(LOG_CRIT, "SQLITE: invalid parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return rows;
	}

	part::sql_operations sql_operations{this->database, _sql_query, &this->last_retryable};
	if (sql_operations.bind_params(_sql_query_params) == false)
	{
		syslog(LOG_CRIT, "SQLITE: failed to bind the parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return rows;
	}

	return sql_operations.multi_execute();
}

storage::database::part::rows storage::database::sqlite::select(const std::string& _sql_query)
{
	if (_sql_query.empty())
	{
		syslog(LOG_CRIT, "SQLITE: invalid parameter - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return {};
	}

	part::sql_operations sql_operations{this->database, _sql_query, &this->last_retryable};
	return sql_operations.multi_execute();
}

bool storage::database::sqlite::concurrently(const std::function<bool()>& _work, int _attempts)
{
	if (_work == nullptr || _attempts < 1)
	{
		return false;
	}

	for (int attempt{0}; attempt < _attempts; ++attempt)
	{
		this->last_retryable = false;
		if (this->transaction("BEGIN CONCURRENT;") == false)
		{
			if (this->last_retryable == true)
			{
				continue;
			}
			return false;
		}

		bool worked{false};
		try
		{
			worked = _work();
		}
		catch (...)
		{
			(void) this->transaction("ROLLBACK;");
			throw;
		}

		if (worked == false)
		{
			// ROLLBACK can replace the conflict error, so keep the flag first.
			const bool retry{this->last_retryable};
			(void) this->transaction("ROLLBACK;");
			if (retry == true)
			{
				continue;
			}
			return false;
		}

		if (this->transaction("COMMIT;") == true)
		{
			return true;
		}

		const bool retry{this->last_retryable};
		(void) this->transaction("ROLLBACK;");
		if (retry == false)
		{
			return false;
		}
	}

	return false;
}


storage::database::part::sql_operations::sql_operations(sqlite3* _db_conn, const std::string& _sql_query, bool *_retryable)
	: retryable{_retryable}
{
	if (_db_conn == nullptr || _sql_query.empty())
	{
		syslog(LOG_CRIT, "SQL_OPERATIONS: invalid parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		throw app::errors::construction;
	}

	if (sqlite3_prepare_v2(_db_conn, _sql_query.c_str(), -1, &this->sql_stmt, nullptr) != SQLITE_OK)
	{
		if (this->retryable != nullptr)
		{
			*this->retryable = is_retryable(sqlite3_errcode(_db_conn), _db_conn);
		}
		syslog(LOG_CRIT, "SQL_OPERATIONS: failed to prepare sql_stmt - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		throw app::errors::construction;
	}
}

storage::database::part::sql_operations::~sql_operations()
{
	if (sqlite3_finalize(this->sql_stmt) != SQLITE_OK)
	{
		syslog(LOG_CRIT, "SQL_OPERATIONS: failed to finalize sql_stmt - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		this->sql_stmt = nullptr;
	}
}

void storage::database::part::sql_operations::note(int _code)
{
	if (this->retryable == nullptr || this->sql_stmt == nullptr)
	{
		return;
	}

	*this->retryable = is_retryable(_code, sqlite3_db_handle(this->sql_stmt));
}

bool storage::database::part::sql_operations::bind_params(const std::vector<param_values>& _sql_query_params)
{
	if (_sql_query_params.empty() == true)
	{
		syslog(LOG_CRIT, "SQL_OPERATIONS: invalid parameters - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
	}

	if (sqlite3_bind_parameter_count(this->sql_stmt) != static_cast<int> (_sql_query_params.size()))
	{
		syslog(LOG_CRIT, "SQL_OPERATIONS: failed to bind the parameter count - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
	}

	int index{0};
	const bool failed{std::any_of(_sql_query_params.begin(), _sql_query_params.end(),
		[&] (const param_values& value) {
			return std::visit(binder{sql_stmt, ++index}, value) != SQLITE_OK;
		})};
	return failed == false;
}

bool storage::database::part::sql_operations::single_execute()
{
	int return_code{sqlite3_step(this->sql_stmt)};
	if (return_code == SQLITE_ROW)
	{
		while (sqlite3_step(this->sql_stmt) == SQLITE_ROW)
		{
		}
		return_code = SQLITE_DONE;
	}
	if (return_code != SQLITE_DONE)
	{
		this->note(return_code);
		return false;
	}

	return true;
}

storage::database::part::rows storage::database::part::sql_operations::multi_execute()
{
	rows rows;
	int return_code{SQLITE_ROW};
	while ((return_code = sqlite3_step(this->sql_stmt)) == SQLITE_ROW)
	{
		rows.emplace_back(std::move(this->collect_row_data()));
	}
	if (return_code != SQLITE_DONE)
	{
		this->note(return_code);
	}

	return rows;
}

storage::database::part::row storage::database::part::sql_operations::collect_row_data()
{
	row row;
	const int number_of_columns = sqlite3_column_count(this->sql_stmt);
	row.reserve(static_cast<std::size_t> (number_of_columns));
	for (int col = 0; col < number_of_columns; ++col)
	{
		row.emplace_back(std::move(convert_column_value(col)));
	}

	return row;
}

storage::database::part::column_value storage::database::part::sql_operations::convert_column_value(const int& _col_index)
{
	column_value column_value;
	int column_type{sqlite3_column_type(this->sql_stmt, _col_index)};
	if (column_type == SQLITE_NULL)
	{
		column_value = nullptr;
	}
	else if (column_type == SQLITE_INTEGER)
	{
		column_value = static_cast<sqlite3_int64>(sqlite3_column_int64(this->sql_stmt, _col_index));
	}
	else if (column_type == SQLITE_FLOAT)
	{
		column_value = sqlite3_column_double(this->sql_stmt, _col_index);
	}
	else if (column_type == SQLITE_TEXT)
	{
		column_value = std::string(reinterpret_cast<const char*>(
					sqlite3_column_text(this->sql_stmt, _col_index)),
					static_cast<std::size_t> (sqlite3_column_bytes(this->sql_stmt, _col_index)));
	}
	else if (column_type == SQLITE_BLOB)
	{
		int number_of_bytes = sqlite3_column_bytes(this->sql_stmt, _col_index);
		const void* bytes = sqlite3_column_blob(this->sql_stmt, _col_index);
		storage::database::blob blob;
		if (number_of_bytes > 0 && bytes != nullptr)
		{
			blob.resize(static_cast<std::size_t> (number_of_bytes));
			std::memcpy(blob.data(), bytes, static_cast<std::size_t> (number_of_bytes));
		}

		column_value = blob;
	}

	return column_value;
}

int storage::database::part::binder::operator()(std::nullptr_t) const
{
	return sqlite3_bind_null(this->sql_stmt, this->id_index);
}

int storage::database::part::binder::operator()(const sqlite3_int64& _data) const
{
	return sqlite3_bind_int64(this->sql_stmt, this->id_index, _data);
}

int storage::database::part::binder::operator()(const double& _data) const
{
	return sqlite3_bind_double(this->sql_stmt, this->id_index, _data);
}

int storage::database::part::binder::operator()(const std::string& _data) const
{
	return sqlite3_bind_text(this->sql_stmt, this->id_index, _data.data(), static_cast<int> (_data.size()), SQLITE_TRANSIENT);
}

int storage::database::part::binder::operator()(const blob& _data) const
{
	return sqlite3_bind_blob(this->sql_stmt, this->id_index, _data.data(), static_cast<int> (_data.size()), SQLITE_TRANSIENT);
}

/*******************************************************************************
 * @file admin_model.cpp
 *
 * @brief Implementation of the admin model backed by a plain Turso database.
 *
 * @details
 * This source file defines the behavior of the `model::admin` class, which
 * is responsible for loading and saving administrator data to an SQLite
 * database. Data is represented as `data::admin` and `data::business`
 * objects and transported across the model boundary using `std::any`.
 *
 * Key responsibilities:
 *  - Opening a plain Turso connection from the configured database file.
 *    There is no database password.
 *  - Loading admin data:
 *      * `load()`       – fetches admin data without a business key.
 *      * `load(name)`   – fetches admin data for a specific business name.
 *  - Saving admin and business details in a single transaction using
 *    UPSERT-style queries to keep information consistent.
 *
 * The implementation relies on:
 *  - `serialize::admin` and `serialize::business` to translate between
 *    result sets and domain objects.
 *  - `storage::database::sqlite` for transaction handling and query
 *    execution.
 *  - `syslog` for reporting validation failures and database errors.
 *******************************************************************************/
#include <string>
#include <syslog.h>
#include <admin_model.h>
#include <admin_serialize.h>
#include <business_serialize.h>


model::admin::admin(const std::string& _database_file)
	: database_file{_database_file} {}

model::admin::~admin() {}

std::any model::admin::load()
{
        data::admin admin_data;
	serialize::admin admin_serialize{};
	storage::database::sqlite database{this->database_file};
	admin_data = std::move(std::any_cast<data::admin> (
				admin_serialize.extract_data(
					database.select(sql::query::admin_no_name_select)
					)
				)
			);

        return admin_data;
}

std::any model::admin::load(const std::string& _business_name)
{
        data::admin admin_data;
	if (_business_name.empty() == true)
	{
                syslog(LOG_CRIT, "ADMIN_MODEL: invalid argument - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		serialize::admin admin_serialize{};
		storage::database::sql_parameters admin_param = {_business_name};
		storage::database::sqlite database{this->database_file};
		admin_data = std::move(std::any_cast<data::admin> (
					admin_serialize.extract_data(
						database.select(sql::query::admin_select, admin_param)
						)
					)
				);
	}

        return admin_data;
}

bool model::admin::save(const std::any& _data)
{
        bool saved{false};
	data::admin admin_data{std::any_cast<data::admin> (_data)};
        if (admin_data.is_valid() == false)
	{
                syslog(LOG_CRIT, "ADMIN_MODEL: data is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
        {
		serialize::admin admin_serialize{};
		data::business business_data{admin_data};
		serialize::business business_serialize{};
		storage::database::sql_parameters admin_params{
			admin_data.get_name(),
			admin_data.get_bank(),
			admin_data.get_branch_code(),
			admin_data.get_account_number(),
			admin_data.get_password(),
			admin_data.get_client_message()
		};
		storage::database::sql_parameters business_params{
			business_data.get_name(),
			business_data.get_address(),
			business_data.get_area_code(),
			business_data.get_town(),
			business_data.get_cellphone(),
			business_data.get_email()
		};
		storage::database::sqlite database{this->database_file};
		saved = database.concurrently([&] () {
			if (database.usert(sql::query::business_details_usert, business_params) == false)
			{
				syslog(LOG_CRIT, "ADMIN_MODEL: failed to execute sql query - "
						 "filename %s, line number %d", __FILE__, __LINE__);
				return false;
			}
			if (database.usert(sql::query::admin_usert, admin_params) == false)
			{
				syslog(LOG_CRIT, "ADMIN_MODEL: failed to execute sql query - "
						 "filename %s, line number %d", __FILE__, __LINE__);
				return false;
			}
			return true;
		});
        }

        return saved;
}

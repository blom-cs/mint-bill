/*******************************************************************************
 * @file client_model.cpp
 *
 * @brief Implementation of the client model persistence logic.
 *
 * @details
 * Defines the behavior of `model::client`, which coordinates between:
 *  - The SQLite storage layer (`storage::database::sqlite`),
 *  - The client and business serializers (`serialize::client`,
 *    `serialize::business`),
 *  - And the domain objects (`data::client`, `data::business`).
 *
 * Core operations:
 *  - `load(const std::string&)`:
 *      * Validates the provided business name.
 *      * Executes `sql::query::client_select` with the business name as a
 *        parameter.
 *      * Delegates row-to-object conversion to `serialize::client`, returning
 *        a populated `data::client` wrapped in `std::any`.
 *
 *  - `save(const std::any&)`:
 *      * Unpacks and validates the `data::client` instance.
 *      * Derives a corresponding `data::business` view for the same entity.
 *      * Executes a transactional sequence:
 *          - UPSERT business details (`business_details_usert`),
 *          - UPSERT client details (`client_usert`),
 *          - Commit or rollback on failure.
 *      * Logs all error paths via `syslog` for traceability.
 *
 * Error handling:
 *  - Invalid arguments and database/transaction failures are reported using
 *    `LOG_CRIT` messages with file and line context.
 *  - On any failure within the transaction, a rollback is attempted and
 *    `false` is returned to the caller.
 ******************************************************************************/
#include <syslog.h>
#include <client_data.h>
#include <client_model.h>
#include <client_serialize.h>
#include <business_serialize.h>


model::client::client(const std::string& _database_file)
	: database_file{_database_file}
{
}

model::client::~client()
{
}

std::any model::client::load(const std::string& _business_name)
{
        data::client client_data{};
        if (_business_name.empty() == true)
	{
                syslog(LOG_CRIT, "CLIENT_MODEL: invalid argument - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
        {
		serialize::client client_serialize{};
		storage::database::sql_parameters client_param = {_business_name};
		storage::database::sqlite database{this->database_file};
		client_data = std::move(std::any_cast<data::client> (
					client_serialize.extract_data(
						database.select(sql::query::client_select, client_param)
						)
					)
				);
        }

        return client_data;
}

bool model::client::save(const std::any& _data)
{
        bool saved{false};
	data::client client_data = std::any_cast<data::client>(_data);
        if (client_data.is_valid() == false)
	{
                syslog(LOG_CRIT, "CLIENT_MODEL: invalid argument - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
        {
		data::business business_data{client_data};
		serialize::client client_serialize{};
		serialize::business business_serialize{};
		storage::database::sql_parameters client_params{
			client_data.get_name(),
			client_data.get_vat_number(),
			client_data.get_statement_schedule()
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
				syslog(LOG_CRIT, "CLIENT_MODEL: failed to execute sql query - "
						 "filename %s, line number %d", __FILE__, __LINE__);
				return false;
			}
			if (database.usert(sql::query::client_usert, client_params) == false)
			{
				syslog(LOG_CRIT, "CLIENT_MODEL: failed to execute sql query - "
						 "filename %s, line number %d", __FILE__, __LINE__);
				return false;
			}
			return true;
		});
        }

        return saved;
}

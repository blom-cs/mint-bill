/*******************************************************************************
 * @file invoice_model.cpp
 *
 * @brief Implementation of the invoice model operations.
 *
 * @details
 * Provides the concrete behavior for `model::invoice`, tying together
 * database access, serialization components, and PDF generation.
 *
 * Core operations:
 *  - `load(const std::string&)`:
 *      * Validates the business name.
 *      * Queries the database for admin, client, invoice, and labor data.
 *      * Assembles each invoice into a `data::pdf_invoice` object containing
 *        invoice, client, and business details, then returns a vector of
 *        `std::any` wrapping these objects.
 *      * If no invoice rows are found, still returns a single
 *        `data::pdf_invoice` with business and client information.
 *
 *  - `save(const std::any&)`:
 *      * Validates a `data::invoice` object extracted from the `std::any`.
 *      * Starts a transaction and:
 *          - Upserts statement metadata for the client and billing period.
 *          - Deletes all existing labor rows for the invoice.
 *          - Upserts the invoice header.
 *          - Re-inserts each description and material line item.
 *      * Commits the transaction on success, or attempts a rollback on error,
 *        logging failures via syslog.
 *
 *  - `prepare_for_email(const std::vector<std::any>&)`:
 *      * Extracts the first `data::pdf_invoice` to determine client and
 *        business metadata.
 *      * Generates PDFs asynchronously for each invoice using `feature::invoice_pdf`.
 *      * Returns a `data::email` populated with subject and attachments.
 *
 *  - `prepare_for_print(const std::vector<std::any>&)`:
 *      * Asynchronously generates and returns a vector of PDF documents
 *        (encoded as strings) for printing.
 *
 * Error handling:
 *  - Uses `syslog(LOG_CRIT, ...)` with file and line information to report
 *    invalid arguments and transaction/SQL issues.
 *  - Relies on the surrounding infrastructure (SQLite wrapper and serializers)
 *    to report lower-level database or mapping errors.
 ******************************************************************************/
#include <invoice_model.h>
#include <invoice_pdf.h>
#include <future>
#include <syslog.h>
#include <sqlite.h>
#include <admin_data.h>
#include <client_data.h>
#include <invoice_data.h>
#include <admin_serialize.h>
#include <client_serialize.h>
#include <invoice_serialize.h>
#include <statement_serialize.h>
#include <date_manager.h>



model::invoice::invoice(const std::string& _database_file)
	: database_file{_database_file} {}

model::invoice::~invoice() {}

std::vector<std::any> model::invoice::load(const std::string& _business_name) const
{
	std::vector<std::any> pdf_invoices_data{};
        if (_business_name.empty())
	{
		syslog(LOG_CRIT, "INVOICE_MODEL: invalid argument - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
        {
		storage::database::sqlite database{this->database_file};
		serialize::admin admin_serialize{};
		data::admin admin_data{
			std::any_cast<data::admin>(
				admin_serialize.extract_data(
					database.select(sql::query::invoice_admin_select)
				)
			)
		};

		storage::database::sql_parameters client_params = {_business_name};
		serialize::client client_serialize{};
		data::client client_data{
			std::any_cast<data::client>(
				client_serialize.extract_data(
					database.select(sql::query::invoice_client_select, client_params)
				)
			)
		};

		serialize::labor labor_serialize{};
		serialize::invoice invoice_serialize{};
		storage::database::sql_parameters invoice_params = {_business_name};
		for (const std::any& data : invoice_serialize.extract_data(
				database.select(sql::query::invoice_select, invoice_params)))
		{
			data::invoice invoice_data{std::any_cast<data::invoice> (data)};
			storage::database::sql_parameters column_params = {std::stoi(invoice_data.get_id())};
			std::vector<data::column> material_column_data{labor_serialize.extract_data(
						database.select(sql::query::material_labor_select, column_params)
					)};
			std::vector<data::column> description_column_data{labor_serialize.extract_data(
						database.select(sql::query::description_labor_select, column_params)
					)};

			invoice_data.set_material_column(material_column_data);
			invoice_data.set_description_column(description_column_data);

			data::pdf_invoice pdf_invoice_data{};
			pdf_invoice_data.set_invoice(invoice_data);
			pdf_invoice_data.set_client(client_data);
			pdf_invoice_data.set_business(admin_data);

			pdf_invoices_data.emplace_back(std::move(pdf_invoice_data));
		}

		if (pdf_invoices_data.empty() == true)
		{
			data::pdf_invoice pdf_invoice_data{};
			pdf_invoice_data.set_client(client_data);
			pdf_invoice_data.set_business(admin_data);

			pdf_invoices_data.emplace_back(std::move(pdf_invoice_data));
		}
        }


        return pdf_invoices_data;
}

bool model::invoice::save(const std::any& _data) const
{
        bool success{false};
	data::invoice invoice_data{std::any_cast<data::invoice> (_data)};
        if (invoice_data.is_valid() == false)
	{
		syslog(LOG_CRIT, "INVOICE_MODEL: invalid argument - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
        {
		storage::database::sqlite database{this->database_file};
		storage::database::sql_parameters labor_delete_all_params{std::stoi(invoice_data.get_id())};

		serialize::admin admin_serialize{};
		data::admin admin_data{
			std::any_cast<data::admin>(
				admin_serialize.extract_data(
					database.select(sql::query::admin_no_name_select)
				)
			)
		};

		if (admin_data.is_valid() == false)
		{
			return success;
		}

		storage::database::sql_parameters params = {invoice_data.get_name()};
		serialize::client client_serialize{};
		data::client client_data{
			std::any_cast<data::client>(
				client_serialize.extract_data(
					database.select(sql::query::invoice_client_select, params)
				)
			)
		};

		utility::date_manager date_manager{};
		utility::period_bounds pb{date_manager.compute_period_bounds(client_data.get_statement_schedule())};
		serialize::statement statement_serialize{};
		storage::database::sql_parameters statement_params{
			invoice_data.get_name(),
			pb.period_start,
			pb.period_end,
			date_manager.current_date(),
			"Not Paid",
		};

		storage::database::sql_parameters invoice_params{
			static_cast<long long>(std::stoi(invoice_data.get_id())),
			invoice_data.get_name(),
			invoice_data.get_name(),
			pb.period_start,
			pb.period_end,
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			invoice_data.get_date(),
			invoice_data.get_paid_status(),
			invoice_data.get_material_total(),
			invoice_data.get_description_total(),
			invoice_data.get_grand_total()
		};

		success = database.concurrently([&] () {
			if (database.usert(sql::query::statement_usert, statement_params) == false
					|| database.usert(sql::query::labor_delete_all_for_invoice, labor_delete_all_params) == false
					|| database.usert(sql::query::invoice_usert, invoice_params) == false)
			{
				syslog(LOG_CRIT, "INVOICE_MODEL: failed to save the invoice - "
						 "filename %s, line number %d", __FILE__, __LINE__);
				return false;
			}

			for (const data::column& column_data : invoice_data.get_description_column())
			{
				storage::database::sql_parameters description_params{
					invoice_data.get_name(),
					invoice_data.get_order_number(),
					invoice_data.get_job_card_number(),
					column_data.get_row_number(),
					column_data.get_is_description(),
					column_data.get_quantity(),
					column_data.get_description(),
					column_data.get_amount(),
				};
				if (database.usert(sql::query::labor_usert, description_params) == false)
				{
					syslog(LOG_CRIT, "INVOICE_MODEL: failed to save a description line - "
							 "filename %s, line number %d", __FILE__, __LINE__);
					return false;
				}
			}

			for (const data::column& column_data : invoice_data.get_material_column())
			{
				storage::database::sql_parameters material_params{
					invoice_data.get_name(),
					invoice_data.get_order_number(),
					invoice_data.get_job_card_number(),
					column_data.get_row_number(),
					column_data.get_is_description(),
					column_data.get_quantity(),
					column_data.get_description(),
					column_data.get_amount(),
				};
				if (database.usert(sql::query::labor_usert, material_params) == false)
				{
					syslog(LOG_CRIT, "INVOICE_MODEL: failed to save a material line - "
							 "filename %s, line number %d", __FILE__, __LINE__);
					return false;
				}
			}

			return true;
		});
        }

        return success;
}

data::email model::invoice::prepare_for_email(const std::vector<std::any>& _pdf_invoices) const
{
	data::email email_data;
	for (const std::any& pdf_invoice : _pdf_invoices)
	{
		data::pdf_invoice pdf_invoice_data{std::any_cast<data::pdf_invoice> (pdf_invoice)};
		email_data.set_client(pdf_invoice_data.get_client());
		email_data.set_business(pdf_invoice_data.get_business());
		break;
	}

	std::vector<std::future<std::string>> pdf_documents;
	std::transform(_pdf_invoices.cbegin(),
			_pdf_invoices.cend(),
			std::back_inserter(pdf_documents),
			[] (const std::any& _pdf_invoice) {
				return std::async(std::launch::async, [&_pdf_invoice] {
					feature::invoice_pdf pdf;
					return pdf.generate(_pdf_invoice);
				});
			});


	std::vector<std::string> pdfs;
	std::transform(
		std::make_move_iterator(pdf_documents.begin()),
		std::make_move_iterator(pdf_documents.end()),
		std::back_inserter(pdfs),
		[](std::future<std::string> _pdf) {
			return _pdf.get();
		});


	email_data.set_subject("Invoice");
	email_data.set_attachments(pdfs);

	return email_data;
}

std::vector<std::string> model::invoice::prepare_for_print(const std::vector<std::any>& _pdf_invoice) const
{
	std::vector<std::future<std::string>> pdf_documents;
	std::transform(_pdf_invoice.cbegin(),
			_pdf_invoice.cend(),
			std::back_inserter(pdf_documents),
			[] (const std::any& _pdf_invoice) {
				return std::async(std::launch::async, [&_pdf_invoice] {
					feature::invoice_pdf pdf;
					return pdf.generate(_pdf_invoice);
				});
			});


	std::vector<std::string> pdfs;
	std::transform(
		std::make_move_iterator(pdf_documents.begin()),
		std::make_move_iterator(pdf_documents.end()),
		std::back_inserter(pdfs),
		[](std::future<std::string> _pdf) {
			return _pdf.get();
		});

	return pdfs;
}

/*******************************************************************************
 * @file invoice_model_test.cpp
 *
 * @brief Unit tests for the model::invoice class.
 *
 * @details
 * This test suite verifies the invoice model’s interaction with the plain Turso
 * database and its composition of higher-level PDF invoice data. It covers:
 *
 *   • Rejecting invalid invoice data on save.
 *   • Persisting valid invoices, including multiple invoices.
 *   • Loading invoices by client/business name and reconstructing:
 *       - Business (admin) details
 *       - Client details
 *       - Invoice line and total information
 *       - Combined pdf_invoice aggregates
 *
 * The tests use a prepared test database and real SQL queries to provide
 * end-to-end coverage of the model behavior.
 *******************************************************************************/
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"


#include <sqlite.h>
#include <generate_pdf.h>
#include <invoice_model.h>
#include <statement_model.h>
#include <admin_serialize.h>
#include <client_serialize.h>
#include <invoice_serialize.h>
#include <business_serialize.h>
extern "C"
{

}


/**********************************TEST LIST************************************
 * 1) Load the data from a database. (Done)
 * 2) Save the data into a database. (Done)
 ******************************************************************************/
TEST_GROUP(invoice_model_test)
{
	const std::string db_file{"../storage/tests/model_test.db"};
        model::invoice invoice_model{db_file};
	storage::database::sqlite database{db_file};
	void setup()
	{
		data::client client_data{test::generate_client_data()};
		data::business client_business_data{client_data};
		data::admin admin_data{test::generate_business_data()};
		data::business admin_business_data{admin_data};
		serialize::admin admin_serialize{};
		serialize::client client_serialize{};
		serialize::invoice invoice_serialize{};
		serialize::business business_serialize{};
		storage::database::sql_parameters admin_sql_parameters{
			admin_data.get_name(),
			admin_data.get_bank(),
			admin_data.get_branch_code(),
			admin_data.get_account_number(),
			admin_data.get_password(),
			admin_data.get_client_message()
		};
		storage::database::sql_parameters admin_business_sql_parameters{
			admin_business_data.get_name(),
			admin_business_data.get_address(),
			admin_business_data.get_area_code(),
			admin_business_data.get_town(),
			admin_business_data.get_cellphone(),
			admin_business_data.get_email(),
		};
		storage::database::sql_parameters client_business_sql_parameters{
			client_business_data.get_name(),
			client_business_data.get_address(),
			client_business_data.get_area_code(),
			client_business_data.get_town(),
			client_business_data.get_cellphone(),
			client_business_data.get_email(),
		};
		storage::database::sql_parameters client_sql_parameters{
			client_data.get_email(),
			client_data.get_vat_number(),
			client_data.get_statement_schedule()
		};
		(void)database.usert(sql::query::business_details_usert, admin_business_sql_parameters);
		(void)database.usert(sql::query::admin_usert, admin_sql_parameters);
		(void)database.usert(sql::query::business_details_usert, client_business_sql_parameters);
		(void)database.usert(sql::query::client_usert, client_sql_parameters);
	}

	void teardown()
	{
	}
};

TEST(invoice_model_test, save_data_to_database_failure)
{
        data::invoice invoice_data{};

        CHECK_EQUAL(false, invoice_model.save(invoice_data));
}

TEST(invoice_model_test, save_data_to_database_successfully)
{
        data::invoice invoice_data{test::generate_invoice_data("invoice model machining")};

        CHECK_EQUAL(true, invoice_model.save(invoice_data));
}

TEST(invoice_model_test, save_second_invoice_to_database_successfully)
{
        data::invoice invoice_data{test::generate_invoice_data("invoice model testing")};

        CHECK_EQUAL(true, invoice_model.save(invoice_data));
}

TEST(invoice_model_test, load_data_from_database_unsuccessfully)
{
	for (const std::any pdf_invoice_data : invoice_model.load(""))
	{
		data::pdf_invoice data{std::any_cast<data::pdf_invoice> (pdf_invoice_data)};

		CHECK_EQUAL(false, data.is_valid());
	}
}

TEST(invoice_model_test, load_data_from_database_successfully)
{
        data::invoice invoice_data{test::generate_invoice_data("invoice model machining")};
	for (const std::any pdf_invoice_data : invoice_model.load(invoice_data.get_name()))
	{
		data::pdf_invoice data{std::any_cast<data::pdf_invoice> (pdf_invoice_data)};
		data::admin admin_data{data.get_business()};
		data::client client_data{data.get_client()};
		data::invoice invoice_data{data.get_invoice()};

		CHECK_EQUAL(true, admin_data.is_valid());
		CHECK_EQUAL(true, client_data.is_valid());
		CHECK_EQUAL(true, invoice_data.is_valid());
		CHECK_EQUAL(true, data.is_valid());
	}
}

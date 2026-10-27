/*******************************************************************************
 * @file statement_model_test.cpp
 *
 * @brief Unit tests for the statement model’s database integration and
 *        PDF-oriented data preparation.
 *
 * @details
 * This test group validates the behavior of model::statement when interacting
 * with the plain Turso database and related models:
 *
 *   • Setup:
 *       - Initializes the shared test database file.
 *       - Persists a generated client (with multiple emails) via model::client
 *         to ensure required foreign-key/business data exists.
 *
 *   • save_first_statement_data_to_db
 *       - Generates a data::statement instance using helper test utilities.
 *       - Persists the statement via model::statement::save and verifies that
 *         the call reports success.
 *
 *   • save_second_statement_data_to_db
 *       - Repeats statement creation and persistence to ensure that multiple
 *         statements can be stored without conflicts in the same database.
 *
 *   • load_data_from_db
 *       - Persists an invoice using model::invoice, then saves a statement
 *         associated with that client.
 *       - Calls model::statement::load with the invoice’s business name,
 *         deserializes each std::any into data::pdf_statement, and confirms
 *         that the resulting PDF-ready statement objects are valid.
 *
 * Together these tests confirm that statements can be saved, retrieved, and
 * transformed into PDF-facing aggregates in coordination with client and
 * invoice models.
 *******************************************************************************/
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"


#include <vector>
#include <iostream>
#include <sqlite.h>
#include <pdf_statement_data.h>
#include <statement_model.h>
#include <client_model.h>
#include <invoice_model.h>
#include <client_data.h>
#include <statement_data.h>
#include <generate_pdf.h>

extern "C"
{

}


/**********************************TEST LIST************************************
 * 1) Load the data from a database. (Done)
 * 2) Save the data into a database. (Done)
 ******************************************************************************/
TEST_GROUP(statement_model_test)
{
	const std::string db_file{"../storage/tests/model_test.db"};
	storage::database::sqlite database{db_file};
        data::client client_data{test::generate_client_data_multiple_emails()};
	model::statement statement{db_file};
	void setup()
	{
		model::client client_model{db_file};
		(void) client_model.save(client_data);
	}

	void teardown()
	{
	}
};

TEST(statement_model_test, save_first_statement_data_to_db)
{
	std::vector<data::pdf_statement> result{};
	data::statement statement_data{test::generate_statement_data()};

	CHECK_EQUAL(true, statement.save(statement_data));
}

TEST(statement_model_test, save_second_statement_data_to_db)
{
	std::vector<data::pdf_statement> result{};
	data::statement statement_data{test::generate_statement_data()};

	CHECK_EQUAL(true, statement.save(statement_data));
}

TEST(statement_model_test, load_data_from_db)
{
	data::invoice invoice_data{test::generate_invoice_data("model testing")};
	data::statement statement_data{test::generate_statement_data()};
	model::invoice invoice_model{db_file};
	(void) invoice_model.save(invoice_data);
	(void) statement.save(statement_data);
	for (const std::any& data : statement.load(invoice_data.get_name()))
	{
		data::pdf_statement pdf_statement_data{std::any_cast<data::pdf_statement>(data)};

		CHECK_EQUAL(true, pdf_statement_data.is_valid());
	}
}

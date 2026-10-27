/*******************************************************************************
 * @file client_model_test.cpp
 *
 * @brief Unit tests for the model::client class.
 *
 * @details
 * This test suite verifies correctness of the client model’s interactions with
 * the underlying plain Turso database. It focuses on:
 *
 *   • Validating and saving client records.
 *   • Updating existing client information.
 *   • Rejecting invalid or incomplete client data.
 *   • Loading client entries by business name.
 *
 * These tests rely on a controlled test database
 * (storage/tests/model_test.db) to ensure predictable behavior and isolation
 * between test runs. Each case exercises real model logic without mocking the
 * storage layer to provide end-to-end verification.
 *******************************************************************************/
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"


#include <client_data.h>
#include <client_model.h>
#include <generate_pdf.h>

extern "C"
{

}


/**********************************TEST LIST************************************
 * 1) Load data from a db. (Done)
 * 2) Save data to a db. (Done)
 ******************************************************************************/
TEST_GROUP(client_model_test)
{
	const std::string db_file{"../storage/tests/model_test.db"};
        model::client client_model{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(client_model_test, unsuccessfully_save_data_to_database)
{
        data::client client_data{};
        client_data.set_name("Dummy");
        client_data.set_address("Geelsterd 12");
        client_data.set_area_code("");
        client_data.set_town("George");
        client_data.set_cellphone("0711422488");
        client_data.set_email("client@gmail.com");
        client_data.set_vat_number("425435");
        client_data.set_statement_schedule("4,4");

        CHECK_EQUAL(false, client_model.save(client_data));
}

TEST(client_model_test, successfully_save_data_to_database)
{
        data::client client_data{test::generate_client_data_multiple_emails()};

        CHECK_EQUAL(true, client_model.save(client_data));
}

TEST(client_model_test, successfully_update_data_in_database)
{
        data::client client_data{test::generate_client_data_multiple_emails()};
	client_data.set_statement_schedule("2/4");

        CHECK_EQUAL(true, client_model.save(client_data));
}

TEST(client_model_test, unsuccessfully_load_data_from_database)
{
        data::client tmp_data{};
        tmp_data = std::any_cast<data::client> (client_model.load(""));

        CHECK_EQUAL(false, tmp_data.is_valid());
}

TEST(client_model_test, successfully_load_data_from_database)
{
        data::client result{};
        data::client data{};
        data.set_name("Tests");
        data.set_address("Geelsterd 12");
        data.set_area_code("05693");
        data.set_town("George");
        data.set_cellphone("0711422488");
        data.set_email("client@gmail.com");
        data.set_vat_number("42543509");
        data.set_statement_schedule("1,4");

	(void) client_model.save(data);
        result = std::any_cast<data::client> (client_model.load("Tests"));

        CHECK_EQUAL(true, result.is_valid());
}

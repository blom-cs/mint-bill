/*******************************************************************************
 * @file admin_model_test.cpp
 *
 * @brief Unit tests for the model::admin class.
 *
 * @details
 * This test suite validates the core functionality of the admin model,
 * including:
 *
 *   • Saving admin (business) details to the database.
 *   • Rejecting invalid or incomplete data during save operations.
 *   • Loading admin records from the database based on business name.
 *   • Ensuring that validation rules are correctly enforced.
 *
 * The test database is the plain file storage/tests/model_test.db. Each test verifies
 * the model’s behavior using lightweight data fixtures and helper utilities.
 *******************************************************************************/
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <admin_model.h>
#include <admin_data.h>
#include <generate_pdf.h>

extern "C"
{

}


/**********************************TEST LIST************************************
 * 1) Load the data from the database. (Done)
 * 2) Save the data to the database. (Done)
 ******************************************************************************/
TEST_GROUP(admin_model_test)
{
	const std::string db_file{"../storage/tests/model_test.db"};
        model::admin admin_model{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(admin_model_test, unsuccessfully_save_the_data)
{
        data::admin admin_data;
        admin_data.set_name("TCP");
        admin_data.set_address("geelsterd 8");
        admin_data.set_area_code("5432");
        admin_data.set_town("george");
        admin_data.set_cellphone("");
        admin_data.set_email("odngmail.com");
        admin_data.set_bank("Standard Bank");
        admin_data.set_branch_code("043232");
        admin_data.set_account_number("0932443824");
        admin_data.set_password("fdasfdsafdsf");
        admin_data.set_client_message("Thank you for your support");

        CHECK_EQUAL(false, admin_model.save(admin_data));
}

TEST(admin_model_test, successfully_save_the_data)
{
        data::admin admin_data{test::generate_business_data()};

        CHECK_EQUAL(true, admin_model.save(admin_data));
}

TEST(admin_model_test, unsuccessfully_load_data_from_database)
{
        data::admin data = std::any_cast<data::admin> (admin_model.load(""));

        CHECK_EQUAL(false, data.is_valid());
}

TEST(admin_model_test, successfully_load_data_from_database)
{
        data::admin admin_data{test::generate_business_data()};
        (void) admin_model.save(admin_data);
        data::admin result{std::any_cast<data::admin> (admin_model.load(admin_data.get_name()))};

        CHECK_EQUAL(true, result.is_valid());
}

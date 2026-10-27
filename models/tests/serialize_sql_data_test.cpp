/*******************************************************************************
 * @file serialize_sql_data_test.cpp
 *
 * @brief Unit tests for serialization and persistence of business, admin,
 *        client, labor/column, statement, and invoice data.
 *
 * @details
 * This test suite verifies the end-to-end behavior of the serialization layer
 * that bridges in-memory data structures and the SQLite database schema. The
 * following aspects are covered:
 *
 *   • business_serialize_test
 *       - Converting business/admin data into SQL parameter lists.
 *       - Reading business rows from the database into data::business objects.
 *
 *   • admin_serialize_test
 *       - Mapping admin data to SQL arguments for INSERT/UPDATE.
 *       - Reconstructing data::admin from admin_* SELECT queries
 *         (with and without explicit business name).
 *
 *   • client_serialize_test
 *       - Converting client data into SQL parameters.
 *       - Rebuilding data::client from client SELECT results.
 *
 *   • column_serialize_test (labor)
 *       - Writing invoice labor/line-item data as SQL parameters.
 *       - Extracting data::column vectors from labor SELECT queries.
 *
 *   • statement_serialize_test
 *       - Serializing statement metadata into SQL arguments.
 *       - Deserializing rows into data::statement objects, including
 *         validation of failure/success paths.
 *
 *   • invoice_serialize_test
 *       - Serializing full invoice records and associated labor lines.
 *       - Reconstructing data::invoice objects (including material and
 *         description columns) from joined invoice/labor queries.
 *
 * Together these tests ensure that all serialize::* classes correctly translate
 * between the database representation and the strongly typed domain model.
 *******************************************************************************/
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <sqlite.h>
#include <generate_pdf.h>
#include <business_serialize.h>
#include <admin_serialize.h>
#include <client_serialize.h>
#include <invoice_serialize.h>
#include <statement_serialize.h>
#include <date_manager.h>


extern "C"
{

}

const std::string db_file{"../storage/tests/model_test.db"};

/**********************************TEST LIST************************************
 * 1) Ensure the ability to convert data to SQL arguments. (Done)
 * 2) Ensure the ability to convert SQL data to admin struct data. (Done)
 ******************************************************************************/
TEST_GROUP(business_serialize_test)
{
	storage::database::sqlite database{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(business_serialize_test, convert_data_to_sql_arguments_successfully)
{
	data::admin admin_data{test::generate_business_data()};
	data::business business_data{};
	business_data.set_name(admin_data.get_name());
	business_data.set_address(admin_data.get_address());
	business_data.set_area_code(admin_data.get_area_code());
	business_data.set_town(admin_data.get_town());
	business_data.set_cellphone(admin_data.get_cellphone());
	business_data.set_email(admin_data.get_email());
	serialize::business business_serialize{};
	storage::database::sql_parameters sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	(void)database.select(sql::query::business_details_usert, sql_parameters);

	CHECK_EQUAL(false, sql_parameters.empty());
}

TEST(business_serialize_test, convert_sql_to_business_data_unsuccessfully)
{
	serialize::business business_serialize{};
	std::vector<storage::database::param_values> params = {};
	data::business business_data{
		std::any_cast<data::business>(
			business_serialize.extract_data(
				database.select(sql::query::business_details_select, params)
			)
		)
	};

	CHECK_EQUAL(false, business_data.is_valid());
}

TEST(business_serialize_test, convert_sql_business_data_successfully)
{
	serialize::business business_serialize{};
	data::admin tmp_admin_data{test::generate_business_data()};
	std::string business_name{tmp_admin_data.get_name()};
	data::business tmp_business_data{tmp_admin_data};
	storage::database::sql_parameters sql_parameters{
		tmp_business_data.get_name(),
		tmp_business_data.get_address(),
		tmp_business_data.get_area_code(),
		tmp_business_data.get_town(),
		tmp_business_data.get_cellphone(),
		tmp_business_data.get_email(),
	};
	(void)database.select(sql::query::business_details_usert, sql_parameters);
	std::vector<storage::database::param_values> params = {business_name};
	data::business business_data{
		std::any_cast<data::business>(
			business_serialize.extract_data(
				database.select(sql::query::business_details_select, params)
			)
		)
	};

	CHECK_EQUAL(true, business_data.is_valid());
}






/**********************************TEST LIST************************************
 * 1) Ensure the ability to convert data to SQL arguments. (Done)
 * 2) Ensure the ability to convert SQL data to admin struct data. (Done)
 ******************************************************************************/
TEST_GROUP(admin_serialize_test)
{
	storage::database::sqlite database{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(admin_serialize_test, convert_data_to_sql_arguments_successfully)
{
	serialize::admin admin_serialize{};
	serialize::business business_serialize{};
	data::admin admin_data{test::generate_business_data()};
	data::business business_data{admin_data};
	storage::database::sql_parameters admin_sql_parameters{
		admin_data.get_name(),
		admin_data.get_bank(),
		admin_data.get_branch_code(),
		admin_data.get_account_number(),
		admin_data.get_password(),
		admin_data.get_client_message()
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	(void)database.select(sql::query::business_details_usert, business_sql_parameters);
	(void)database.select(sql::query::admin_usert, admin_sql_parameters);

	CHECK_EQUAL(false, admin_sql_parameters.empty());
}

TEST(admin_serialize_test, convert_sql_to_admin_data_unsuccessfully)
{
	serialize::admin admin_serialize{};
	std::vector<storage::database::param_values> params = {};
	data::admin admin_data{
		std::any_cast<data::admin>(
			admin_serialize.extract_data(
				database.select(sql::query::admin_select, params)
			)
		)
	};

	CHECK_EQUAL(false, admin_data.is_valid());
}

TEST(admin_serialize_test, convert_sql_admin_data_successfully)
{
	serialize::admin admin_serialize{};
	serialize::business business_serialize{};
	data::admin tmp_admin_data{test::generate_business_data()};
	data::business business_data{tmp_admin_data};
	storage::database::sql_parameters admin_sql_parameters{
		tmp_admin_data.get_name(),
		tmp_admin_data.get_bank(),
		tmp_admin_data.get_branch_code(),
		tmp_admin_data.get_account_number(),
		tmp_admin_data.get_password(),
		tmp_admin_data.get_client_message()
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email()
	};
	(void)database.select(sql::query::business_details_usert, business_sql_parameters);
	(void)database.select(sql::query::admin_usert, admin_sql_parameters);
	std::vector<storage::database::param_values> params = {tmp_admin_data.get_name()};
	data::admin admin_data{
		std::any_cast<data::admin>(
			admin_serialize.extract_data(
				database.select(sql::query::admin_select, params)
			)
		)
	};

	CHECK_EQUAL(true, admin_data.is_valid());
}

TEST(admin_serialize_test, convert_sql_admin_data_successfully_without_name)
{
	serialize::admin admin_serialize{};
	serialize::business business_serialize{};
	data::admin tmp_admin_data{test::generate_business_data()};
	data::business business_data{tmp_admin_data};
	storage::database::sql_parameters admin_sql_parameters{
		tmp_admin_data.get_name(),
		tmp_admin_data.get_bank(),
		tmp_admin_data.get_branch_code(),
		tmp_admin_data.get_account_number(),
		tmp_admin_data.get_password(),
		tmp_admin_data.get_client_message()
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email()
	};
	(void)database.select(sql::query::business_details_usert, business_sql_parameters);
	(void)database.select(sql::query::admin_usert, admin_sql_parameters);
	std::vector<storage::database::param_values> params = {tmp_admin_data.get_name()};
	data::admin admin_data{
		std::any_cast<data::admin>(
			admin_serialize.extract_data(
				database.select(sql::query::admin_no_name_select)
			)
		)
	};

	CHECK_EQUAL(true, admin_data.is_valid());
}





/**********************************TEST LIST************************************
 * 1) Ensure the ability to convert data to SQL arguments. (Done)
 * 2) Ensure the ability to convert SQL data to admin struct data. (Done)
 ******************************************************************************/
TEST_GROUP(client_serialize_test)
{
	storage::database::sqlite database{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(client_serialize_test, convert_data_to_sql_arguments_successfully)
{
	serialize::client client_serialize{};
	serialize::business business_serialize{};
	data::client client_data{test::generate_client_data()};
	data::business business_data{client_data};
	storage::database::sql_parameters client_sql_parameters{
		client_data.get_email(),
		client_data.get_vat_number(),
		client_data.get_statement_schedule()
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	(void)database.select(sql::query::business_details_usert, business_sql_parameters);
	(void)database.select(sql::query::client_usert, client_sql_parameters);

	CHECK_EQUAL(false, client_sql_parameters.empty());
}

TEST(client_serialize_test, convert_sql_to_business_data_unsuccessfully)
{
	serialize::client client_serialize{};
	std::vector<storage::database::param_values> params = {};
	data::client client_data{
		std::any_cast<data::client>(
			client_serialize.extract_data(
				database.select(sql::query::client_select, params)
			)
		)
	};

	CHECK_EQUAL(false, client_data.is_valid());
}

TEST(client_serialize_test, convert_sql_business_data_successfully)
{
	serialize::client client_serialize{};
	serialize::business business_serialize{};
	data::client tmp_client_data{test::generate_client_data()};
	data::business business_data{tmp_client_data};
	storage::database::sql_parameters client_sql_parameters{
		tmp_client_data.get_email(),
		tmp_client_data.get_vat_number(),
		tmp_client_data.get_statement_schedule()
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	(void)database.select(sql::query::business_details_usert, business_sql_parameters);
	(void)database.select(sql::query::client_usert, client_sql_parameters);
	std::vector<storage::database::param_values> params = {tmp_client_data.get_name()};
	data::client client_data{
		std::any_cast<data::client>(
			client_serialize.extract_data(
				database.select(sql::query::client_select, params)
			)
		)
	};

	CHECK_EQUAL(true, client_data.is_valid());
}







/**********************************TEST LIST************************************
 * 1) Ensure the ability to convert data to SQL arguments. (Done)
 * 2) Ensure the ability to convert SQL data to admin struct data. (Done)
 ******************************************************************************/
TEST_GROUP(column_serialize_test)
{
	storage::database::sqlite database{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(column_serialize_test, convert_data_to_sql_arguments_successfully)
{
	serialize::labor labor_serialize{};
	data::invoice invoice_data{test::generate_invoice_data("Serialization test")};
	for (const data::column& column_data : invoice_data.get_description_column())
	{
		data::invoice invoice_data{test::generate_invoice_data("Serialization test")};
		data::client client_data{test::generate_client_data()};
		data::business business_data{client_data};
		serialize::client client_serialize{};
		serialize::business business_serialize{};
		serialize::invoice invoice_serialize{};
		storage::database::sql_parameters business_sql_parameters{
			business_data.get_name(),
			business_data.get_address(),
			business_data.get_area_code(),
			business_data.get_town(),
			business_data.get_cellphone(),
			business_data.get_email(),
		};
		storage::database::sql_parameters client_sql_parameters{
			client_data.get_email(),
			client_data.get_vat_number(),
			client_data.get_statement_schedule()
		};
		utility::date_manager date_manager{};
		utility::period_bounds pb{date_manager.compute_period_bounds(client_data.get_statement_schedule())};
		storage::database::sql_parameters invoice_sql_parameters{
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
		(void)database.select(sql::query::business_details_usert, business_sql_parameters);
		(void)database.select(sql::query::client_usert, client_sql_parameters);
		(void)database.select(sql::query::invoice_usert, invoice_sql_parameters);

		for (const data::column& column_data : invoice_data.get_description_column())
		{
			storage::database::sql_parameters labor_sql_parameters{
				invoice_data.get_name(),
				invoice_data.get_order_number(),
				invoice_data.get_job_card_number(),
				column_data.get_row_number(),
				column_data.get_is_description(),
				column_data.get_quantity(),
				column_data.get_description(),
				column_data.get_amount(),
			};
			(void)database.select(sql::query::labor_usert, labor_sql_parameters);
		}

		for (const data::column& column_data : invoice_data.get_material_column())
		{
			storage::database::sql_parameters labor_sql_parameters{
				invoice_data.get_name(),
				invoice_data.get_order_number(),
				invoice_data.get_job_card_number(),
				column_data.get_row_number(),
				column_data.get_is_description(),
				column_data.get_quantity(),
				column_data.get_description(),
				column_data.get_amount(),
			};
			(void)database.select(sql::query::labor_usert, labor_sql_parameters);
		}

		storage::database::sql_parameters sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};

		CHECK_EQUAL(false, sql_parameters.empty());
	}
}

TEST(column_serialize_test, convert_sql_to_business_data_unsuccessfully)
{
	serialize::labor labor_serialize{};
	std::vector<storage::database::param_values> params = {};

	for (const data::column& column_data : labor_serialize.extract_data(
				database.select(sql::query::description_labor_select, params)
			))
	{
		CHECK_EQUAL(false, column_data.is_valid());
	}
}

TEST(column_serialize_test, convert_sql_to_business_data_successfully)
{
	serialize::labor labor_serialize{};
	data::invoice invoice_data{test::generate_invoice_data("Serialization test")};
	data::client client_data{test::generate_client_data()};
	data::business business_data{client_data};
	serialize::client client_serialize{};
	serialize::business business_serialize{};
	serialize::invoice invoice_serialize{};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	storage::database::sql_parameters client_sql_parameters{
		client_data.get_email(),
		client_data.get_vat_number(),
		client_data.get_statement_schedule()
	};
	utility::date_manager date_manager{};
	utility::period_bounds pb{date_manager.compute_period_bounds(client_data.get_statement_schedule())};
	storage::database::sql_parameters invoice_sql_parameters{
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
	(void)database.select(sql::query::business_details_usert, business_sql_parameters);
	(void)database.select(sql::query::client_usert, client_sql_parameters);
	(void)database.select(sql::query::invoice_usert, invoice_sql_parameters);

	for (const data::column& column_data : invoice_data.get_description_column())
	{
		storage::database::sql_parameters labor_sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};
		(void)database.select(sql::query::labor_usert, labor_sql_parameters);
	}

	for (const data::column& column_data : invoice_data.get_material_column())
	{
		storage::database::sql_parameters labor_sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};
		(void)database.select(sql::query::labor_usert, labor_sql_parameters);
	}
	std::vector<storage::database::param_values> params = {
		static_cast<long long> (
				std::stoi(invoice_data.get_id())
				)
	};

	for (const data::column& column_data : labor_serialize.extract_data(
				database.select(sql::query::description_labor_select, params)))
	{
		CHECK_EQUAL(true, column_data.is_valid());
	}
}








/**********************************TEST LIST************************************
 * 1) Ensure the ability to convert data to SQL arguments. (Done)
 * 2) Ensure the ability to convert SQL data to statement data struct. (Done)
 ******************************************************************************/
TEST_GROUP(statement_serialize_test)
{
	storage::database::sqlite database{db_file};
	void setup()
	{
		data::client client_data{test::generate_client_data()};
		data::business business_data{client_data};
		serialize::client client_serialize{};
		serialize::business business_serialize{};
		serialize::statement statement_serialize{};
		storage::database::sql_parameters business_sql_parameters{
			business_data.get_name(),
			business_data.get_address(),
			business_data.get_area_code(),
			business_data.get_town(),
			business_data.get_cellphone(),
			business_data.get_email(),
		};
		storage::database::sql_parameters client_sql_parameters{
			client_data.get_email(),
			client_data.get_vat_number(),
			client_data.get_statement_schedule()
		};
		(void)database.usert(sql::query::business_details_usert, business_sql_parameters);
		(void)database.usert(sql::query::client_usert, client_sql_parameters);
	}

	void teardown()
	{
	}
};

TEST(statement_serialize_test, convert_data_to_sql_arguments_successfully)
{
	data::statement statement_data{test::generate_statement_data()};
	storage::database::sql_parameters statement_sql_parameters{
		statement_data.get_name(),
		statement_data.get_period_start(),
		statement_data.get_period_end(),
		statement_data.get_date(),
		statement_data.get_paid_status(),
	};
	(void)database.usert(sql::query::statement_usert, statement_sql_parameters);

	CHECK_EQUAL(false, statement_sql_parameters.empty());
}

TEST(statement_serialize_test, convert_sql_to_business_data_unsuccessfully)
{
	serialize::statement statement_serialize{};
	storage::database::sql_parameters params{};

	for (const std::any& data : statement_serialize.extract_data(
			database.select(sql::query::statement_select, params)))
	{
		data::statement statement_data{std::any_cast<data::statement> (data)};

		CHECK_EQUAL(false, statement_data.is_valid());
	}
}

TEST(statement_serialize_test, convert_sql_to_business_data_successfully)
{
	data::statement statement_data{test::generate_statement_data()};
	serialize::statement statement_serialize{};
	storage::database::sql_parameters usert_params{
		statement_data.get_name(),
		statement_data.get_period_start(),
		statement_data.get_period_end(),
		statement_data.get_date(),
		statement_data.get_paid_status(),
	};
	(void)database.usert(sql::query::statement_usert, usert_params);

	storage::database::sql_parameters select_params{
		statement_data.get_name()
	};

	for (const std::any& data : statement_serialize.extract_data(
			database.select(sql::query::statement_select, select_params)))
	{
		data::statement statement_data{std::any_cast<data::statement> (data)};

		CHECK_EQUAL(true, statement_data.is_valid());
	}
}






/**********************************TEST LIST************************************
 * 1) Ensure the ability to convert data to SQL arguments. (Done)
 * 2) Ensure the ability to convert SQL data to invoice data struct. (Done)
 ******************************************************************************/
TEST_GROUP(invoice_serialize_test)
{
	storage::database::sqlite database{db_file};
	void setup()
	{
	}

	void teardown()
	{
	}
};

TEST(invoice_serialize_test, convert_data_to_sql_arguments_successfully)
{
	data::invoice invoice_data{test::generate_invoice_data("Serialization test")};
	data::client client_data{test::generate_client_data()};
	data::business business_data{client_data};
	data::statement statement_data{test::generate_statement_data()};
	serialize::client client_serialize{};
	serialize::business business_serialize{};
	serialize::invoice invoice_serialize{};
	serialize::labor labor_serialize{};
	serialize::statement statement_serialize{};
	storage::database::sql_parameters statement_sql_parameters{
		statement_data.get_name(),
		statement_data.get_period_start(),
		statement_data.get_period_end(),
		statement_data.get_date(),
		statement_data.get_paid_status(),
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	storage::database::sql_parameters client_sql_parameters{
		client_data.get_email(),
		client_data.get_vat_number(),
		client_data.get_statement_schedule()
	};
	utility::date_manager date_manager{};
	utility::period_bounds pb{date_manager.compute_period_bounds(client_data.get_statement_schedule())};
	storage::database::sql_parameters invoice_sql_parameters{
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
	(void)database.usert(sql::query::business_details_usert, business_sql_parameters);
	(void)database.usert(sql::query::client_usert, client_sql_parameters);
	(void)database.usert(sql::query::invoice_usert, invoice_sql_parameters);
	(void)database.usert(sql::query::statement_usert, statement_sql_parameters);

	for (const data::column& column_data : invoice_data.get_description_column())
	{
		storage::database::sql_parameters labor_sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};
		(void)database.usert(sql::query::labor_usert, labor_sql_parameters);
	}

	for (const data::column& column_data : invoice_data.get_material_column())
	{
		storage::database::sql_parameters labor_sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};
		(void)database.usert(sql::query::labor_usert, labor_sql_parameters);
	}

	CHECK_EQUAL(false, invoice_sql_parameters.empty());
}

TEST(invoice_serialize_test, convert_sql_to_business_data_unsuccessfully)
{
	serialize::invoice invoice_serialize{};
	std::vector<storage::database::param_values> params = {};

	for (const std::any& data : invoice_serialize.extract_data(
			database.select(sql::query::invoice_select, params)))
	{
		data::invoice invoice_data{std::any_cast<data::invoice> (data)};

		CHECK_EQUAL(false, invoice_data.is_valid());
	}
}

TEST(invoice_serialize_test, convert_sql_to_business_data_successfully)
{
	data::invoice invoice_data{test::generate_invoice_data("Serialization test")};
	data::client client_data{test::generate_client_data()};
	data::business business_data{client_data};
	data::statement statement_data{test::generate_statement_data()};
	serialize::client client_serialize{};
	serialize::business business_serialize{};
	serialize::invoice invoice_serialize{};
	serialize::labor labor_serialize{};
	serialize::statement statement_serialize{};
	storage::database::sql_parameters statement_sql_parameters{
		statement_data.get_name(),
		statement_data.get_period_start(),
		statement_data.get_period_end(),
		statement_data.get_date(),
		statement_data.get_paid_status(),
	};
	storage::database::sql_parameters business_sql_parameters{
		business_data.get_name(),
		business_data.get_address(),
		business_data.get_area_code(),
		business_data.get_town(),
		business_data.get_cellphone(),
		business_data.get_email(),
	};
	storage::database::sql_parameters client_sql_parameters{
		client_data.get_email(),
		client_data.get_vat_number(),
		client_data.get_statement_schedule()
	};
	utility::date_manager date_manager{};
	utility::period_bounds pb{date_manager.compute_period_bounds(client_data.get_statement_schedule())};
	storage::database::sql_parameters invoice_sql_parameters{
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
	(void)database.usert(sql::query::business_details_usert, business_sql_parameters);
	(void)database.usert(sql::query::client_usert, client_sql_parameters);
	(void)database.usert(sql::query::invoice_usert, invoice_sql_parameters);
	(void)database.usert(sql::query::statement_usert, statement_sql_parameters);

	for (const data::column& column_data : invoice_data.get_description_column())
	{
		storage::database::sql_parameters labor_sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};
		(void)database.usert(sql::query::labor_usert, labor_sql_parameters);
	}

	for (const data::column& column_data : invoice_data.get_material_column())
	{
		storage::database::sql_parameters labor_sql_parameters{
			invoice_data.get_name(),
			invoice_data.get_order_number(),
			invoice_data.get_job_card_number(),
			column_data.get_row_number(),
			column_data.get_is_description(),
			column_data.get_quantity(),
			column_data.get_description(),
			column_data.get_amount(),
		};
		(void)database.usert(sql::query::labor_usert, labor_sql_parameters);
	}

	storage::database::sql_parameters invoice_params = {invoice_data.get_name()};
	for (const std::any& data : invoice_serialize.extract_data(
			database.select(sql::query::invoice_select, invoice_params)))
	{
		data::invoice tmp_invoice_data{std::any_cast<data::invoice> (data)};
		storage::database::sql_parameters column_params = {std::stoi(tmp_invoice_data.get_id())};
		std::vector<data::column> description_column_data{labor_serialize.extract_data(
					database.select(sql::query::description_labor_select, column_params)
				)};
		std::vector<data::column> material_column_data{labor_serialize.extract_data(
					database.select(sql::query::material_labor_select, column_params)
				)};
		tmp_invoice_data.set_description_column(description_column_data);
		tmp_invoice_data.set_material_column(material_column_data);

		CHECK_EQUAL(true, tmp_invoice_data.is_valid());
	}
}

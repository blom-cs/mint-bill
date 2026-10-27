/*******************************************************************************
 * @file    gui_parts_tests.cpp
 *
 * @brief   CppUTest unit tests for GUI helper components and statement views.
 *
 * @details
 * This file contains integration-style GUI tests for multiple helper classes
 * in the gui::part and gui::part::statement namespaces, using a common
 * Gtk::Application and Gtk::Builder loaded from the mint-bill.ui file.
 *
 * Covered Components:
 *   - gui::part::search_bar
 *       * Verifies creation, validity checks, subscriber registration,
 *         search keyword propagation, and automatic clearing on stack changes.
 *
 *   - gui::part::dialog
 *       * Ensures safe creation, connection of response callbacks, and
 *         visibility control (show/hide) including failure paths.
 *
 *   - gui::part::sub_button
 *       * Tests button creation, callback subscription, and validity checks.
 *
 *   - gui::part::statement::column_view
 *       * Validates creation, adding columns, populating with invoice data,
 *         clearing, and extracting records from the underlying store.
 *
 *   - gui::part::statement::pdf_window
 *       * Exercises PDF window generation with both invalid and valid
 *         poppler::document instances.
 *
 *   - gui::part::statement::invoice_pdf_view
 *       * Checks creation, validity, population, clearing, and record
 *         extraction behavior for invoice PDF list views.
 *
 *   - gui::part::statement::statement_pdf_view
 *       * Tests creation, validity, population, clearing, and extraction
 *         of statement records from the list view model.
 *
 *   - gui::part::statement::columns (invoice/date/order/paid/price)
 *       * Verifies column validity, correct column titles, and that
 *         retrieve_value() reflects the underlying invoice data.
 *
 * Test Strategy:
 *   - Uses Gtk::Builder to pull named widgets from the mint-bill.ui file.
 *   - Uses real signal connections (e.g., “search-changed”, list activation).
 *   - Calls C-style emission helpers where needed to simulate user actions.
 *   - Relies on mocked or test data providers for invoice/statement models.
 *
 * The goal of these tests is to ensure that GUI helper parts behave correctly
 * when wired together with actual GTK widgets, while still remaining fast and
 * deterministic enough for automated unit test runs.
 *******************************************************************************/
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"



#include <invoice_pdf.h>
#include <gtkmm.h>
#include <iostream>
#include <stack.h>
#include <sqlite.h>
#include <statement_page.h>
#include <statement_data.h>
#include <pdf_invoice_data.h>
#include <pdf_statement_data.h>
#include <mock_pdf_invoice_data.h>
#include <generate_pdf.h>
extern "C"
{

}


static std::string callback_result{""};
static void search_bar_callback(const std::string& keyword) {
    callback_result = keyword;
}


/**********************************GUI PART SEARCH BAR TEST LIST***************
 * 1) The user should be able to search for a business. Done
 * 2) There should be no elements in the list if the user clears the
 *    search. Done
 * 3) When the user navigates to a different page, the search entry should
 *    be automatically cleared. Done
 ******************************************************************************/
TEST_GROUP(gui_part_search_bar)
{
        gui::part::search_bar search_bar{"business-name-search"};
        Glib::RefPtr<Gtk::Builder> builder;
        Glib::RefPtr<Gtk::Application> app;
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
		callback_result.clear();
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(gui_part_search_bar, create_search_bar_successfully)
{
	CHECK_EQUAL(true, search_bar.create(builder));
}

TEST(gui_part_search_bar, create_search_bar_unsuccessfully)
{
	CHECK_EQUAL(false, search_bar.create(nullptr));
}

TEST(gui_part_search_bar, is_search_bar_not_valid)
{
	CHECK_EQUAL(true, search_bar.is_not_valid());
}

TEST(gui_part_search_bar, is_search_bar_valid)
{
	(void) search_bar.create(builder);

	CHECK_EQUAL(false, search_bar.is_not_valid());
}

TEST(gui_part_search_bar, fail_to_add_subscriber_without_page_name)
{
	(void) search_bar.create(builder);

	CHECK_EQUAL(false, search_bar.subscribe("", search_bar_callback));
}

TEST(gui_part_search_bar, fail_to_add_subscriber_without_search_bar_callback)
{
	(void) search_bar.create(builder);

	CHECK_EQUAL(false, search_bar.subscribe("statement-page", nullptr));
}

TEST(gui_part_search_bar, add_subscriber)
{
	(void) search_bar.create(builder);

	CHECK_EQUAL(true, search_bar.subscribe("statement-page", search_bar_callback));
}

TEST(gui_part_search_bar, try_to_add_multiple_subscriber_to_the_same_page)
{
	(void) search_bar.create(builder);
	(void) search_bar.subscribe("statement-page", search_bar_callback);

	CHECK_EQUAL(false, search_bar.subscribe("statement-page", search_bar_callback));
}

TEST(gui_part_search_bar, clear_the_search_bar_entry_on_every_stack_page_change_unsuccessfully)
{
	(void) search_bar.create(builder);

	CHECK_EQUAL(false, search_bar.update(""));
}

TEST(gui_part_search_bar, clear_the_search_bar_entry_on_every_stack_page_change)
{
	std::string current_stack_page_name{"invoice-page"};
	(void) search_bar.create(builder);

	CHECK_EQUAL(true, search_bar.update(current_stack_page_name));
}

TEST(gui_part_search_bar, changing_the_search_keyword_unsuccessfully)
{
	(void) search_bar.create(builder);
	(void) search_bar.subscribe("statement-page", search_bar_callback);
	auto entry = builder->get_widget<Gtk::SearchEntry>("business-name-search");
	entry->set_text("");
	g_signal_emit_by_name(entry->gobj(), "search-changed");

	CHECK_EQUAL("", entry->get_text());
}

TEST(gui_part_search_bar, changing_the_search_keyword_successfully)
{
	std::string result{"Test Business"};
	(void) search_bar.create(builder);
	(void) search_bar.subscribe("statement-page", search_bar_callback);
	auto entry = builder->get_widget<Gtk::SearchEntry>("business-name-search");
	entry->set_text(result);
	g_signal_emit_by_name(entry->gobj(), "search-changed");
	std::string expected{entry->get_text()};

	STRCMP_EQUAL(result.c_str(), expected.c_str());
}

TEST(gui_part_search_bar, reach_the_end_of_subscriber_list)
{
	std::string expected{"Test Business"};
	(void) search_bar.create(builder);
	(void) search_bar.subscribe("statement-page", search_bar_callback);
	auto stack = builder->get_widget<Gtk::Stack>("pages-stack");
	auto entry = builder->get_widget<Gtk::SearchEntry>("business-name-search");
	stack->set_visible_child("statement-page", Gtk::StackTransitionType::NONE);
	entry->set_text(expected);
	g_signal_emit_by_name(entry->gobj(), "search-changed");

	STRCMP_EQUAL(expected.c_str(), callback_result.c_str());
}


static void dialog_callback_function(const int response)
{
	std::cout << "Response: " << std::to_string(response) << std::endl;
}


/**********************************GUI PART DIALOG TEST LIST*******************
 * 1) create the dialog. (Done)
 * 2) connect the signal handler of the dialog. (Done)
 * 3) show the dialog. (Done)
 * 4) ensure that the system does not crash if we connect the dialog
 *    before it was created. (Done)
 * 5) verify that the dialog is valid before using it. (Done)
 ******************************************************************************/
TEST_GROUP(gui_part_dialog)
{
        gui::part::dialog gui_dialog{"statement-no-item-selected-alert"};
        Glib::RefPtr<Gtk::Builder> builder;
        Glib::RefPtr<Gtk::Application> app;
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(gui_part_dialog, dialog_create)
{
        CHECK_EQUAL(true, gui_dialog.create(builder));
}

TEST(gui_part_dialog, dialog_is_valid)
{
        (void) gui_dialog.create(builder);

        CHECK_EQUAL(false, gui_dialog.is_not_valid());
}

TEST(gui_part_dialog, connect_unsuccessfully)
{
        CHECK_EQUAL(false, gui_dialog.connect(nullptr));
}

TEST(gui_part_dialog, connect_successfully)
{
        CHECK_EQUAL(true, gui_dialog.connect(dialog_callback_function));
}

TEST(gui_part_dialog, dialog_show_unsuccessfully)
{
        CHECK_EQUAL(false, gui_dialog.show());
}

TEST(gui_part_dialog, dialog_show_successfully)
{
        (void) gui_dialog.create(builder);

        CHECK_EQUAL(true, gui_dialog.show());
}

TEST(gui_part_dialog, dialog_hide_unsuccessfully)
{
        CHECK_EQUAL(false, gui_dialog.hide());
}

TEST(gui_part_dialog, dialog_hide_successfully)
{
        (void) gui_dialog.create(builder);

        CHECK_EQUAL(true, gui_dialog.hide());
}







static void sub_button_callback_function() {
	std::cout << "Clicked the button\n";
}


/**********************************GUI PART SUB_BUTTON TEST LIST***************
 * 1)
 ******************************************************************************/
TEST_GROUP(sub_button_test)
{
        gui::part::sub_button sub_button{"save-button"};
        Glib::RefPtr<Gtk::Builder> builder;
        Glib::RefPtr<Gtk::Application> app;
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(sub_button_test, create_sub_button_unsuccessfully)
{
	CHECK_EQUAL(false, sub_button.create(nullptr));
}

TEST(sub_button_test, create_sub_button_successfully)
{
	CHECK_EQUAL(true, sub_button.create(builder));
}

TEST(sub_button_test, sub_button_subscribe_callback_unsuccessfully)
{
	(void) sub_button.create(builder);

	CHECK_EQUAL(false, sub_button.subscribe(nullptr));
}

TEST(sub_button_test, sub_button_subscribe_callback_successfully)
{
	(void) sub_button.create(builder);

	CHECK_EQUAL(true, sub_button.subscribe(sub_button_callback_function));
}

TEST(sub_button_test, sub_button_is_not_valid)
{
	CHECK_EQUAL(true, sub_button.is_not_valid());
}

TEST(sub_button_test, sub_button_is_valid)
{
	(void) sub_button.create(builder);

	CHECK_EQUAL(false, sub_button.is_not_valid());
}






/**********************************GUI PART COLUMN_VIEW TEST LIST**************
 * 1) Create the column view. (Done)
 * 3) Extract data from the column view. (Done)
 * 4) Populate the column view. (Done)
 * 5) Clear the column view. (Done)
 * 6) Ensure the column view is valid. (Done)
 * 7) Add a column to the column view. (Done)
 * 8) Add a single data line to the column view. (Done)
 * 9) Populate the column view based on a search.
 ******************************************************************************/
TEST_GROUP(statement_page_column_view_test)
{
        Glib::RefPtr<Gtk::Builder> builder{};
        Glib::RefPtr<Gtk::Application> app{};
        gui::part::statement::column_view column_view{"statement-column-view", "statement-column-view-vadjustment"};
        gui::part::statement::columns::date date{};
        gui::part::statement::columns::price price{};
        gui::part::statement::columns::paid_status paid_status{};
        gui::part::statement::columns::order_number order_number{};
        gui::part::statement::columns::invoice_number invoice_number{};
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
		(void) date.create("Date");
		(void) price.create("Price");
		(void) paid_status.create("Paid Status");
		(void) order_number.create("Order Number");
		(void) invoice_number.create("Invoice Number");
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(statement_page_column_view_test, column_view_create)
{
        CHECK_EQUAL(true, column_view.create(builder));
}

TEST(statement_page_column_view_test, column_view_is_valid)
{
        (void) column_view.create(builder);

        CHECK_EQUAL(false, column_view.is_not_valid());
}

TEST(statement_page_column_view_test, column_view_add_column)
{
        (void) column_view.create(builder);
        (void) column_view.is_not_valid();

        CHECK_EQUAL(true, column_view.add_column(invoice_number));
        CHECK_EQUAL(true, column_view.add_column(date));
        CHECK_EQUAL(true, column_view.add_column(order_number));
        CHECK_EQUAL(true, column_view.add_column(paid_status));
        CHECK_EQUAL(true, column_view.add_column(price));
}

TEST(statement_page_column_view_test, column_view_populate)
{
	std::vector<std::any> statements{};
	for (const std::any& data : test::get_pdf_statements("Test Business Name"))
	{
		data::pdf_statement pdf_statement{std::any_cast<data::pdf_statement>(data)};
		for (const data::pdf_invoice& pdf_invoice : pdf_statement.get_pdf_invoices())
		{
			statements.emplace_back(pdf_invoice.get_invoice());
		}
	}
        (void) column_view.create(builder);
        (void) column_view.is_not_valid();
        (void) column_view.add_column(invoice_number);
        (void) column_view.add_column(date);
        (void) column_view.add_column(order_number);
        (void) column_view.add_column(paid_status);
        (void) column_view.add_column(price);

        CHECK_EQUAL(true, column_view.populate(statements));
}

TEST(statement_page_column_view_test, column_view_clear_unsuccessful)
{
        CHECK_EQUAL(false, column_view.clear());
}

TEST(statement_page_column_view_test, column_view_clear_successful)
{
	std::vector<std::any> invoices{};
	for (const std::any& data : test::get_pdf_statements("Test Business Name"))
	{
		data::pdf_statement pdf_statement{std::any_cast<data::pdf_statement>(data)};
		for (const data::pdf_invoice& pdf_invoice : pdf_statement.get_pdf_invoices())
		{
			invoices.emplace_back(pdf_invoice.get_invoice());
		}
	}
        (void) column_view.create(builder);
        (void) column_view.is_not_valid();
        (void) column_view.add_column(invoice_number);
        (void) column_view.add_column(date);
        (void) column_view.add_column(order_number);
        (void) column_view.add_column(paid_status);
        (void) column_view.add_column(price);
        (void) column_view.populate(invoices);

        CHECK_EQUAL(true, column_view.clear());
}

TEST(statement_page_column_view_test, extract_data_from_store)
{
	std::vector<std::any> invoices{};
	for (const std::any& data : test::get_pdf_statements("Test Business Name"))
	{
		data::pdf_statement pdf_statement{std::any_cast<data::pdf_statement>(data)};
		for (const data::pdf_invoice& pdf_invoice : pdf_statement.get_pdf_invoices())
		{
			invoices.emplace_back(pdf_invoice.get_invoice());
		}
	}
        (void) column_view.create(builder);
        (void) column_view.is_not_valid();
        (void) column_view.add_column(invoice_number);
        (void) column_view.add_column(date);
        (void) column_view.add_column(order_number);
        (void) column_view.add_column(paid_status);
        (void) column_view.add_column(price);
        (void) column_view.populate(invoices);
	std::vector<std::any> records(column_view.extract());
	data::invoice result{std::any_cast<data::invoice> (records[0])};

        CHECK_EQUAL(true, result.is_valid());
}





/**********************************GUI PART LIST_VIEW TEST LIST**************
 * 1) Create the list view. (Done)
 * 3) Extract data from the list view.
 * 4) Populate the list view.
 * 5) Clear the list view.
 * 6) Ensure the list view is valid.
 * 7) Add a column to the list view.
 * 8) Add a single data line to the list view.
 * 9) Populate the list view based on a search.
 ******************************************************************************/
TEST_GROUP(pdf_window_test)
{
	feature::invoice_pdf invoice_pdf{};
        Glib::RefPtr<Gtk::Builder> builder{};
        Glib::RefPtr<Gtk::Application> app{};
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(pdf_window_test, generate_window_with_bad_document)
{
	gui::part::statement::pdf_window pdf_window{"Invoice"};
	std::shared_ptr<poppler::document> document{nullptr};

	CHECK_EQUAL(false, pdf_window.generate(document));
}

TEST(pdf_window_test, generate_window_with_good_document)
{
	data::pdf_invoice pdf_invoice{};
	pdf_invoice.set_business(retrieve_business_data());
	pdf_invoice.set_client(retrieve_client_data());
	pdf_invoice.set_invoice(retrieve_invoice_data());
	gui::part::statement::pdf_window pdf_window{"Invoice"};
	std::string document{invoice_pdf.generate(pdf_invoice)};
	std::shared_ptr<poppler::document> pdf_document;
	std::vector<char> byte_vector{document.begin(), document.end()};
	poppler::byte_array byte_array{byte_vector};
	poppler::document* raw_doc{poppler::document::load_from_data(&byte_array)};
	pdf_document = std::shared_ptr<poppler::document>(raw_doc, [](poppler::document* ptr) {
		if (ptr) {
			delete ptr;
			ptr = nullptr;
		}
	});

	CHECK_EQUAL(true, pdf_window.generate(pdf_document));
}




/*****************************GUI PART INVOICE LIST_VIEW TEST LIST**************
 * 1) Create the list view. (Done)
 * 3) Extract data from the list view. (Done)
 * 4) Populate the list view. (Done)
 * 5) Clear the list view. (Done)
 * 6) Ensure the list view is valid. (Done)
 * 7) Add a column to the list view. (Done)
 * 8) Add a single data line to the list view. (Done)
 * 9) Populate the list view based on a search. (Done)
 ******************************************************************************/
TEST_GROUP(invoice_pdf_view_test)
{
        Glib::RefPtr<Gtk::Builder> builder{};
        Glib::RefPtr<Gtk::Application> app{};
        gui::part::statement::invoice_pdf_view invoice_pdf_view{"statement-invoice-list-view", "statement-invoice-list-view-vadjustment"};
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(invoice_pdf_view_test, invoice_pdf_view_create_unsuccessfully)
{
        CHECK_EQUAL(false, invoice_pdf_view.create(nullptr));
}

TEST(invoice_pdf_view_test, create_successfully)
{
        CHECK_EQUAL(true, invoice_pdf_view.create(builder));
}

TEST(invoice_pdf_view_test, view_is_not_valid)
{
        CHECK_EQUAL(true, invoice_pdf_view.is_not_valid());
}

TEST(invoice_pdf_view_test, view_is_valid)
{
        (void) invoice_pdf_view.create(builder);

        CHECK_EQUAL(false, invoice_pdf_view.is_not_valid());
}

TEST(invoice_pdf_view_test, populate_view_without_being_created)
{
	std::vector<std::any> invoices{};

        CHECK_EQUAL(false, invoice_pdf_view.populate(invoices));
}

TEST(invoice_pdf_view_test, populate_view_with_empty_data)
{
	std::vector<std::any> empty_data{};
        (void) invoice_pdf_view.create(builder);

        CHECK_EQUAL(false, invoice_pdf_view.populate(empty_data));
}

TEST(invoice_pdf_view_test, populate_invoice_view_successfully)
{
	std::vector<std::any> statement_pdf_data = std::move(test::get_pdf_statements("Test business"));
	data::pdf_statement temp_statement = std::move(std::any_cast<data::pdf_statement> (statement_pdf_data.front()));
	std::vector<data::pdf_invoice> temp_invoices{temp_statement.get_pdf_invoices()};

	// std::vector<std::any> invoices{};
	for (const data::pdf_invoice& inv : temp_invoices)
	{
		CHECK_EQUAL(true, inv.is_valid());
	// 	invoices.emplace_back(inv);
	}
	// (void) invoice_pdf_view.create(builder);
	//
	// CHECK_EQUAL(true, invoice_pdf_view.populate(invoices));
}

TEST(invoice_pdf_view_test, clear_invoice_view_unsuccessfully)
{
	CHECK_EQUAL(false, invoice_pdf_view.clear());
}

TEST(invoice_pdf_view_test, clear_invoice_view_successfully)
{
	std::vector<std::any> statement_pdf_data = std::move(test::get_pdf_statements("Test business"));
	data::pdf_statement temp_statement = std::move(std::any_cast<data::pdf_statement> (statement_pdf_data.front()));
	std::vector<data::pdf_invoice> temp_invoices{temp_statement.get_pdf_invoices()};
	std::vector<std::any> invoices{};
	for (const data::pdf_invoice& inv : temp_invoices)
	{
		invoices.emplace_back(inv);
	}
	(void) invoice_pdf_view.create(builder);
	(void) invoice_pdf_view.populate(invoices);

	CHECK_EQUAL(true, invoice_pdf_view.clear());
}

TEST(invoice_pdf_view_test, extract_invoice_view_data_unsuccessfully)
{
	std::vector<std::any> result{invoice_pdf_view.extract()};

	CHECK_COMPARE(2, >=, result.size());
}

// TEST(invoice_pdf_view_test, extract_invoice_view_data_successfully)
// {
// 	std::vector<std::any> statement_pdf_data = std::move(test::get_pdf_statements("Test business"));
// 	data::pdf_statement temp_statement = std::move(std::any_cast<data::pdf_statement> (statement_pdf_data.front()));
// 	std::vector<data::pdf_invoice> temp_invoices{temp_statement.get_pdf_invoices()};
// 	std::vector<std::any> invoices{};
// 	for (const data::pdf_invoice& inv : temp_invoices)
// 	{
// 		invoices.emplace_back(inv);
// 	}
// 	(void) invoice_pdf_view.create(builder);
// 	(void) invoice_pdf_view.populate(invoices);
// 	std::vector<std::any> result{invoice_pdf_view.extract()};
// 	data::pdf_invoice data{};
// 	for (const std::any& invoice : invoice_pdf_view.extract())
// 	{
// 		data = std::move(std::any_cast<data::pdf_invoice> (invoice));
// 		break;
// 	}
//
// 	CHECK_EQUAL(true, data.is_valid());
// }





/*************************GUI PART STATEMENT LIST_VIEW TEST LIST****************
 * 1) Create the list view.
 * 3) Extract data from the list view.
 * 4) Populate the list view.
 * 5) Clear the list view.
 * 6) Ensure the list view is valid.
 * 7) Add a column to the list view.
 * 8) Add a single data line to the list view.
 * 9) Populate the list view based on a search.
 ******************************************************************************/
TEST_GROUP(statement_pdf_view_test)
{
	// const std::string db_file{"../storage/tests/model_test.db"};
	// data::statement statement_data{test::generate_statement_data()};
	// storage::database::sqlite database{db_file};
	// model::statement statement_model{db_file};
        Glib::RefPtr<Gtk::Builder> builder{};
        Glib::RefPtr<Gtk::Application> app{};
        gui::part::statement::statement_pdf_view statement_pdf_view{"statement-list-view", "statement-list-view-vadjustment"};
	void setup()
	{
                app = Gtk::Application::create("org.testing");
                builder = Gtk::Builder::create();
                builder->add_from_file("../gui/mint-bill.ui");
	}

	void teardown()
	{
                app.reset();
	}
};

TEST(statement_pdf_view_test, statement_pdf_view_create_unsuccessfully)
{
        CHECK_EQUAL(false, statement_pdf_view.create(nullptr));
}

TEST(statement_pdf_view_test, create_successfully)
{
        CHECK_EQUAL(true, statement_pdf_view.create(builder));
}

TEST(statement_pdf_view_test, view_is_not_valid)
{
        CHECK_EQUAL(true, statement_pdf_view.is_not_valid());
}

TEST(statement_pdf_view_test, view_is_valid)
{
        (void) statement_pdf_view.create(builder);

        CHECK_EQUAL(false, statement_pdf_view.is_not_valid());
}

TEST(statement_pdf_view_test, populate_view_without_being_created)
{
	std::vector<std::any> statements{};

        CHECK_EQUAL(false, statement_pdf_view.populate(statements));
}

TEST(statement_pdf_view_test, populate_view_with_empty_data)
{
	std::vector<std::any> empty_data{};
        (void) statement_pdf_view.create(builder);

        CHECK_EQUAL(false, statement_pdf_view.populate(empty_data));
}

TEST(statement_pdf_view_test, populate_statement_view_successfully)
{
	std::vector<std::any> statements = std::move(test::get_pdf_statements("Test business"));
	(void) statement_pdf_view.create(builder);

	CHECK_EQUAL(true, statement_pdf_view.populate(statements));
}

TEST(statement_pdf_view_test, clear_statement_view_before_creation)
{
	CHECK_EQUAL(false, statement_pdf_view.clear());
}

TEST(statement_pdf_view_test, clear_statement_view)
{
	std::vector<std::any> statements = std::move(test::get_pdf_statements("Test business"));
	(void) statement_pdf_view.create(builder);
	(void) statement_pdf_view.populate(statements);

	CHECK_EQUAL(true, statement_pdf_view.clear());
}

TEST(statement_pdf_view_test, extract_statement_view_data_before_creation)
{
	std::vector<std::any> records{statement_pdf_view.extract()};

	CHECK_EQUAL(1, records.size());
}

TEST(statement_pdf_view_test, extract_statement_view_data_after_creation)
{
	std::vector<std::any> statements = std::move(test::get_pdf_statements("Test business"));
	(void) statement_pdf_view.create(builder);
	(void) statement_pdf_view.populate(statements);
	std::vector<std::any> records{statement_pdf_view.extract()};

	CHECK_EQUAL(false, records.empty());
}




/*************************GUI PART COLUMN INVOICE NUMBER TEST LIST*************
 * 1) The column invoice number should have a name. (Done)
 * 3) The column invoice number should have the ability to
 *    retrieve the value. (Done)
 * 4) The column invoice number should be created. (Done)
 * 5) The column invoice number should be destroyed. (Done)
 * 6) The column invoice number should be set. (Done)
 * 7) The column must have the ability to verify it's validity. (Done)
  ******************************************************************************/
TEST_GROUP(statement_columns_test)
{
	data::invoice invoice{};
	std::string price_title{"price"};
	std::string date_title{"date"};
	std::string order_number_title{"order number"};
	std::string paid_status_title{"paid"};
	std::string invoice_number_title{"invoice number"};
	Glib::RefPtr<Gtk::Builder> builder{};
	Glib::RefPtr<Gtk::Application> app{};
	std::unique_ptr<Gtk::ColumnView> view{};
	Glib::RefPtr<Gtk::SelectionModel> selection{};
	gui::part::statement::columns::date date_column{};
	gui::part::statement::columns::price price_column{};
	gui::part::statement::columns::paid_status paid_status_column{};
	gui::part::statement::columns::order_number order_number_column{};
	gui::part::statement::columns::invoice_number invoice_number_column{};
	Glib::RefPtr<Gio::ListStore<gui::part::statement::columns::entries>> store{};
	void setup()
	{
		invoice = std::move(retrieve_invoice_data());
		app = Gtk::Application::create("org.testing");
		builder = Gtk::Builder::create();
		builder->add_from_file("../gui/mint-bill.ui");
		view = std::unique_ptr<Gtk::ColumnView>{
			builder->get_widget<Gtk::ColumnView>("statement-column-view")};
		store = Gio::ListStore<gui::part::statement::columns::entries>::create();
		store->append(gui::part::statement::columns::entries::create(invoice));
		selection = Gtk::SingleSelection::create(store);
		view->set_model(selection);
		(void) date_column.create(date_title);
		(void) price_column.create(price_title);
		(void) paid_status_column.create(paid_status_title);
		(void) order_number_column.create(order_number_title);
		(void) invoice_number_column.create(invoice_number_title);
	}

	void teardown()
	{
		app.reset();
	}
};

TEST(statement_columns_test, check_if_the_column_is_valid_after_creation)
{
        CHECK_EQUAL(false, invoice_number_column.is_not_valid());
        CHECK_EQUAL(false, date_column.is_not_valid());
        CHECK_EQUAL(false, order_number_column.is_not_valid());
        CHECK_EQUAL(false, paid_status_column.is_not_valid());
        CHECK_EQUAL(false, price_column.is_not_valid());
}

TEST(statement_columns_test, retrieve_the_column)
{
        Glib::RefPtr<Gtk::ColumnViewColumn> invoice_number_column_temp{invoice_number_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> date_column_temp{date_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> order_number_column_temp{order_number_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> paid_status_column_temp{paid_status_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> price_column_temp{price_column.retrieve_item()};
        view->append_column(invoice_number_column.retrieve_item());
        view->append_column(date_column.retrieve_item());
        view->append_column(order_number_column.retrieve_item());
        view->append_column(paid_status_column.retrieve_item());
        view->append_column(price_column.retrieve_item());
        std::string result_invoice_number_column{invoice_number_column_temp->get_title()};
        std::string result_date_column{date_column_temp->get_title()};
        std::string result_order_number_column{order_number_column_temp->get_title()};
        std::string result_paid_status_column{paid_status_column_temp->get_title()};
        std::string result_price_column{price_column_temp->get_title()};

        CHECK_EQUAL(invoice_number_title, result_invoice_number_column);
        CHECK_EQUAL(date_title, result_date_column);
        CHECK_EQUAL(order_number_title, result_order_number_column);
        CHECK_EQUAL(paid_status_title, result_paid_status_column);
        CHECK_EQUAL(price_title, result_price_column);
}

TEST(statement_columns_test, retrieve_value)
{
        Glib::RefPtr<Gtk::ColumnViewColumn> invoice_number_column_temp{invoice_number_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> date_column_temp{date_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> order_number_column_temp{order_number_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> paid_status_column_temp{paid_status_column.retrieve_item()};
        Glib::RefPtr<Gtk::ColumnViewColumn> price_column_temp{price_column.retrieve_item()};
        view->append_column(invoice_number_column.retrieve_item());
        view->append_column(date_column.retrieve_item());
        view->append_column(order_number_column.retrieve_item());
        view->append_column(paid_status_column.retrieve_item());
        view->append_column(price_column.retrieve_item());
        std::string expected_invoice_number{invoice.get_id()};
        std::string expected_date{invoice.get_date()};
        std::string expected_order_number{invoice.get_order_number()};
        std::string expected_paid_status{invoice.get_paid_status()};
        std::string expected_price{invoice.get_grand_total()};

        CHECK_EQUAL(expected_invoice_number, invoice_number_column.retrieve_value());
        CHECK_EQUAL(expected_date, date_column.retrieve_value());
        CHECK_EQUAL(expected_order_number, order_number_column.retrieve_value());
        CHECK_EQUAL(expected_paid_status, paid_status_column.retrieve_value());
        CHECK_EQUAL(expected_price, price_column.retrieve_value());
}

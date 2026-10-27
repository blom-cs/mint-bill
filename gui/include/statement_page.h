/*******************************************************************************
 * @file statement_page.h
 *
 * @brief Declaration of the gui::statement_page class for statement operations
 *        such as viewing, saving, printing, and emailing account statements.
 *
 * @details
 * The statement_page class implements interface::operations_page and provides
 * the GUI workflow for interacting with customer account statements. It:
 *
 *   - Binds to Gtk widgets created via Gtk::Builder (labels, list views,
 *     dialogs, etc.) and wires them into higher-level GUI components under
 *     part::statement (column views and PDF views).
 *   - Loads statement data from the database via model::statement and presents
 *     it as lists of data::pdf_statement and data::invoice instances.
 *   - Handles user selection of one or more statements and associated invoices,
 *     storing them in internal containers for later actions.
 *   - Coordinates printing through gui::part::printer and emailing through
 *     feature::email, including asynchronous email sending with a
 *     Glib::Dispatcher callback (email_sent).
 *   - Supports saving statement and invoice changes back to the database via
 *     model::statement and model::invoice after user confirmation.
 *   - Manages dialogs for save/print/email operations, no selection, and
 *     no-internet conditions using part::dialog helpers.
 *
 * All stateful GUI and model-related members are kept in a single instance,
 * and the class is explicitly non-copyable and non-movable to avoid accidental
 * duplication of UI or data ownership.
 *******************************************************************************/
#ifndef _STATEMENT_PAGE_H_
#define _STATEMENT_PAGE_H_
#include <gui.h>
#include <thread>
#include <future>
#include <syslog.h>
#include <gui_parts.h>
#include <email_data.h>
#include <statement_pdf.h>
#include <statement_model.h>


namespace gui {
class statement_page : public interface::operations_page {
public:
        statement_page();
        statement_page(const statement_page&) = delete;
        statement_page(statement_page&&) = delete;
        statement_page& operator= (const statement_page&) = delete;
        statement_page& operator= (statement_page&&) = delete;
        ~statement_page() override;

	[[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&,
					  const std::shared_ptr<Gtk::Window>& _main_window) override;
	[[nodiscard]] virtual bool search(const std::string&) override;
	[[nodiscard]] virtual bool print() override;
	[[nodiscard]] virtual bool email() override;
	[[nodiscard]] virtual bool save() override;

protected:
	void email_sent();
	bool email_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool print_setup(const Glib::RefPtr<Gtk::Builder>&,
			 const std::shared_ptr<Gtk::Window>&);
	[[nodiscard]] bool save_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool populate(const std::string&);
	[[nodiscard]] bool no_item_selected_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool no_internet_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool statement_column_view_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool invoice_pdf_view_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool statement_pdf_view_setup(const Glib::RefPtr<Gtk::Builder>&);

private:
	std::future<bool> email_future;
	Glib::Dispatcher email_dispatcher{};
	bool on_double_click();
	bool on_single_click();
	void clear();


private:
	std::vector<std::any> documents{};
	std::vector<std::any> invoice_data{};
	data::pdf_statement selected_pdf_statement{};
	std::unique_ptr<Gtk::Label> total_label{};
        part::dialog no_item_selected{"statement-no-item-selected-alert"};
        part::dialog email_alert{"statement-email-button-alert"};
        part::dialog print_alert{"statement-print-button-alert"};
        part::dialog no_internet_alert{"statement-email-no-internet-alert"};
        part::dialog save_alert{"statement-save-button-alert"};
	part::statement::columns::date date{};
	part::statement::columns::price price{};
	part::statement::columns::paid_status paid_status{};
	part::statement::columns::order_number order_number{};
	part::statement::columns::invoice_number invoice_number{};
	part::statement::column_view statement_view{"statement-column-view",
		"statement-column-view-vadjustment"};
	part::statement::invoice_pdf_view invoice_pdf_view{"statement-invoice-list-view",
		"statement-invoice-list-view-vadjustment"};
	part::statement::statement_pdf_view statement_pdf_view{"statement-list-view",
		"statement-list-view-vadjustment"};
};
}


#endif

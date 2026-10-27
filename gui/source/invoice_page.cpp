/*******************************************************************************
 * @file invoice_page.cpp
 *
 * @brief Implementation of the gui::invoice_page class and helper wrappers
 *        used to drive invoice creation, editing, searching, printing, and
 *        emailing from the GUI.
 *
 * @details
 * This implementation:
 *   - Wires up Gtk::ColumnView and Gtk::ListView instances for description,
 *     material, and known-invoice views, including their selection models and
 *     SignalListItemFactory setup/bind/teardown handlers.
 *   - Manages the lifecycle of line-item stores (Gio::ListStore<column_entries>)
 *     and invoice stores (Gio::ListStore<invoice_entries>), updating totals
 *     whenever items change.
 *   - Validates user input for quantity (integer) and amount (double) columns,
 *     showing error dialogs when formats are incorrect and resetting values
 *     to safe defaults.
 *   - Loads invoice data from the database via model::invoice, converts it
 *     into data::pdf_invoice and data::invoice objects, and populates both the
 *     “known invoices” list and the editable invoice fields.
 *   - Extracts the current GUI state into a data::invoice instance, including
 *     description and material columns tagged with is_description flags, ready
 *     for persistence or further processing.
 *   - Coordinates confirmation and error dialogs for save, email, print, and
 *     data/selection problems through part::dialog helpers.
 *   - Launches asynchronous email sending using std::async and notifies the
 *     main thread via a Glib::Dispatcher callback (email_sent), handling
 *     success and failure (e.g., no internet) with logging and dialogs.
 *
 * Throughout, syslog is used to record both normal flow (user actions) and
 * error conditions, aiding diagnostics and operational observability.
 *******************************************************************************/
#include <chrono>
#include <ostream>
#include <syslog.h>
#include <paths.h>
#include <invoice_page.h>
#include <date_manager.h>


//GCOVR_EXCL_START
namespace limit {
        constexpr std::uint8_t MAX_QUANTITY{9};
        constexpr std::uint8_t MAX_AMOUNT{15};
        constexpr std::uint16_t MAX_DESCRIPTION{500};
}

gui::invoice_page::invoice_page()
{
        this->email_dispatcher.connect(sigc::mem_fun(*this, &invoice_page::email_sent));
}

gui::invoice_page::~invoice_page()
{

}

bool gui::invoice_page::create(const Glib::RefPtr<Gtk::Builder>& _ui_builder,
			       const std::shared_ptr<Gtk::Window>& _main_window)
{
        if (_ui_builder == nullptr || _main_window == nullptr)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: Invalid arguments - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
        }
        else
        {
                create_views(_ui_builder);
                create_entries(_ui_builder);
                create_buttons(_ui_builder);
                connect_material_view();
                connect_description_view();
                connect_invoice_view();

		if (this->save_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup save - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->email_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup email - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->print_setup(_ui_builder, _main_window) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup print - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->wrong_info_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup wrong_info - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->no_internet_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup no_internet - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->no_item_selected_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup no_item_selected - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->wrong_data_in_amount_column_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup wrong_data_in_amount_column_setup - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}

		if (this->wrong_data_in_quantity_column_setup(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to setup wrong_data_in_quantity_column_setup - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return false;
		}
        }

        return true;
}

bool gui::invoice_page::search(const std::string& _business_name)
{
        bool searched{false};
	this->clear();
        if (_business_name.empty() == true)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The _keyword is empty - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
        }
        else
        {
		searched = true;
		this->populate(_business_name);
        }

        return searched;
}

bool gui::invoice_page::print()
{
	bool success{false};
	if (this->invoices_selected.empty() == true)
	{
		if (this->no_item_selected_alert.show() == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to show no_item_selected_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
	}
	else
	{
		if (this->print_alert.show() == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to show print dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = true;
		}
	}

	return success;
}

bool gui::invoice_page::email()
{
	bool success{false};
	if (this->invoices_selected.empty() == true)
	{
		if (this->no_item_selected_alert.show() == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to show no_item_selected_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
	}
	else
	{
		if (this->email_alert.show() == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to show email dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = true;
		}
	}

	return success;
}

void gui::invoice_page::clear()
{
	this->job_card->set_text("");
	this->order_number->set_text("");
	this->invoice_date->set_text("");
	this->invoice_number->set_text("");
	this->invoice_store->remove_all();
	this->material_store->remove_all();
	this->description_store->remove_all();
}

bool gui::invoice_page::save()
{
	bool success{false};
	if (this->save_alert.is_not_valid())
	{
                syslog(LOG_CRIT, "INVOICE_PAGE: The save_alert is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->save_alert.show() == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: The show save_alert - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = true;
		}
	}

	return success;
}

void gui::invoice_page::create_views(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
        this->description_view = std::unique_ptr<Gtk::ColumnView>{
                _ui_builder->get_widget<Gtk::ColumnView>("new-invoice-description")};
        this->description_adjustment = std::shared_ptr<Gtk::Adjustment>{
                _ui_builder->get_object<Gtk::Adjustment>("new-invoice-description-column-view-vadjustment")};
        this->material_view = std::unique_ptr<Gtk::ColumnView>{
                _ui_builder->get_widget<Gtk::ColumnView>("new-invoice-material")};
        this->material_adjustment = std::shared_ptr<Gtk::Adjustment>{
                _ui_builder->get_object<Gtk::Adjustment>("new-invoice-material-column-view-vadjustment")};
        this->invoice_view = std::unique_ptr<Gtk::ListView>{
                _ui_builder->get_widget<Gtk::ListView>("known-invoice-view")};
        this->invoices_adjustment = std::shared_ptr<Gtk::Adjustment>{
                _ui_builder->get_object<Gtk::Adjustment>("known-invoice-adjustment")};
}

void gui::invoice_page::create_entries(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
        this->job_card = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("new-invoice-job-card-entry")};
        this->order_number = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("new-invoice-order-number-entry")};
        this->description_total_label = std::unique_ptr<Gtk::Label>{
                _ui_builder->get_widget<Gtk::Label>("new-invoice-description-total-label")};
        this->material_total_label = std::unique_ptr<Gtk::Label>{
                _ui_builder->get_widget<Gtk::Label>("new-invoice-material-total-label")};
        this->grand_total_label = std::unique_ptr<Gtk::Label>{
                _ui_builder->get_widget<Gtk::Label>("new-invoice-total-label")};
        this->invoice_date = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("new-invoice-date-entry")};
        this->invoice_number = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("new-invoice-number-entry")};
}

void gui::invoice_page::create_buttons(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
        this->description_add_button = std::unique_ptr<Gtk::Button>{
                _ui_builder->get_widget<Gtk::Button>("invoice-description-add-button")};
        this->material_add_button = std::unique_ptr<Gtk::Button>{
                _ui_builder->get_widget<Gtk::Button>("invoice-material-add-button")};
        this->material_delete_button = std::unique_ptr<Gtk::Button>{
                _ui_builder->get_widget<Gtk::Button>("invoice-material-delete-button")};
        this->description_delete_button = std::unique_ptr<Gtk::Button>{
                _ui_builder->get_widget<Gtk::Button>("invoice-description-delete-button")};
}

bool gui::invoice_page::save_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->save_alert.create(_ui_builder) == false)
		{

			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create save_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->save_alert.connect([this] (const int& _response) {
				model::invoice invoice_model{app::paths::database()};
				data::invoice data{this->extract_invoice_data()};
				switch(_response)
				{
					case GTK_RESPONSE_YES:
						syslog(LOG_INFO, "INVOICE_PAGE: User chose to save the information - "
								 "filename %s, line number %d", __FILE__, __LINE__);
						if (invoice_model.save(data) == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to save the invoice information - "
									 "filename %s, line number %d", __FILE__, __LINE__);
							if (this->save_alert.hide() == false)
							{
								syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide the save dialog - "
										 "filename %s, line number %d", __FILE__, __LINE__);
							}

							if (this->wrong_info_alert.show() == false)
							{
								syslog(LOG_CRIT, "INVOICE_PAGE: Failed to show the wrong data dialog - "
										 "filename %s, line number %d", __FILE__, __LINE__);
							}
						}
						else
						{
							if (this->save_alert.hide() == false)
							{
								syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide the save dialog - "
										 "filename %s, line number %d", __FILE__, __LINE__);
							}

							this->clear();
							this->populate(data.get_name());
						}
						break;
					case GTK_RESPONSE_NO:
						syslog(LOG_INFO, "INVOICE_PAGE: User chose not to save the information - "
								 "filename %s, line number %d", __FILE__, __LINE__);
						if (this->save_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide the save dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->save_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide the save dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

bool gui::invoice_page::email_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->email_alert.create(_ui_builder) == false)
		{

			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create email_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->email_alert.connect([this] (const int& _response) {
				switch (_response)
				{
					case GTK_RESPONSE_YES:
						if (this->email_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide email_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						else
						{
							this->email_future = std::move(std::async(std::launch::async, [this] () {
								model::invoice invoice_model{app::paths::database()};
								data::email data{invoice_model.prepare_for_email(
										 this->invoices_selected)};
								feature::email email;
								bool result{email.send(data)};
								this->email_dispatcher.emit();
								return result;
							}));
						}
						break;
					case GTK_RESPONSE_NO:
						if (this->email_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide email_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->email_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide email_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

bool gui::invoice_page::print_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder,
				      const std::shared_ptr<Gtk::Window>& _main_window)
{
	bool success{false};
	if (_ui_builder == nullptr || _main_window == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder or the _main_window is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->print_alert.create(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create print dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->print_alert.connect([this, _main_window] (const int& response) {
				model::invoice invoice_model{app::paths::database()};
				switch (response)
				{
					case GTK_RESPONSE_YES:
						if (this->print_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide print dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						else
						{
							std::vector<std::string> data{invoice_model.prepare_for_print(this->invoices_selected)};
							gui::part::printer printer{"invoice"};
							if (printer.print(data, _main_window) == false)
							{
								syslog(LOG_CRIT, "INVOICE_PAGE: Failed to print documents - "
										 "filename %s, line number %d", __FILE__, __LINE__);
							}
						}
						break;
					case GTK_RESPONSE_NO:
						if (this->print_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide print dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->print_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide print dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

bool gui::invoice_page::wrong_info_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->wrong_info_alert.create(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create wrong_info_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->wrong_info_alert.connect([this] (const int& _response) {
				switch (_response)
				{
					case GTK_RESPONSE_CLOSE:
						if (this->wrong_info_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide wrong_info_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->wrong_info_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide wrong_info_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

bool gui::invoice_page::no_internet_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->no_internet_alert.create(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create no_internet_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->no_internet_alert.connect([this] (const int& _response) {
				switch (_response)
				{
					case GTK_RESPONSE_CLOSE:
						if (this->no_internet_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide no_internet_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->no_internet_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide no_internet_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

bool gui::invoice_page::no_item_selected_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->no_item_selected_alert.create(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create no_item_selected_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->no_item_selected_alert.connect([this] (const int& _response) {
				switch (_response)
				{
					case GTK_RESPONSE_CLOSE:
						if (this->no_item_selected_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide no_item_selected_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->no_item_selected_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide no_item_selected_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}
bool gui::invoice_page::wrong_data_in_amount_column_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->wrong_data_in_amount_column_alert.create(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create wrong_data_in_amount_column_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->wrong_data_in_amount_column_alert.connect([this] (const int& _response) {
				switch (_response)
				{
					case GTK_RESPONSE_CLOSE:
						if (this->wrong_data_in_amount_column_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide wrong_data_in_amount_column_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->wrong_data_in_amount_column_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide wrong_data_in_amount_column_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

bool gui::invoice_page::wrong_data_in_quantity_column_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (_ui_builder == nullptr)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: The _ui_builder is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->wrong_data_in_quantity_column_alert.create(_ui_builder) == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to create wrong_data_in_quantity_column_alert dialog - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->wrong_data_in_quantity_column_alert.connect([this] (const int& _response) {
				switch (_response)
				{
					case GTK_RESPONSE_CLOSE:
						if (this->wrong_data_in_quantity_column_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide wrong_data_in_quantity_column_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
					default:
						if (this->wrong_data_in_quantity_column_alert.hide() == false)
						{
							syslog(LOG_CRIT, "INVOICE_PAGE: Failed to hide wrong_data_in_quantity_column_alert dialog - "
									 "filename %s, line number %d", __FILE__, __LINE__);
						}
						break;
				}
			});
		}
	}

	return success;
}

void gui::invoice_page::connect_description_view()
{
        this->description_store = std::shared_ptr<Gio::ListStore<column_entries>>{
                Gio::ListStore<column_entries>::create()};
        if (!this->description_store)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The description_store is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::MultiSelection> selection_model = Gtk::MultiSelection::create(this->description_store);
        if (!selection_model)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The description selection_model is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->description_view->set_model(selection_model);
        this->description_view->set_name("description_view");

        connect_description_add_button();
        connect_description_delete_button(selection_model);
        connect_description_list_store();

        quantity_column(this->description_view);
        description_column(this->description_view);
        amount_column(this->description_view);
}

void gui::invoice_page::connect_description_add_button()
{
        if (!this->description_add_button)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The description_add_button is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->description_add_button->signal_clicked().connect([this] () {
                syslog(LOG_INFO, "INVOICE_PAGE: User is adding a description row - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                this->description_store->append(column_entries::create());
                Glib::signal_timeout().connect_once([this]() {
                        this->description_adjustment->set_value(this->description_adjustment->get_upper());
                }, 30);
        });
}

void gui::invoice_page::connect_description_delete_button(const Glib::RefPtr<Gtk::MultiSelection>& selection_model)
{
        if (!this->description_delete_button)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The description_delete_button is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->description_delete_button->signal_clicked().connect([selection_model, this] () {
                syslog(LOG_INFO, "INVOICE_PAGE: User is removing description row - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                Glib::RefPtr<const Gtk::Bitset> indices = selection_model->get_selection();
                while (indices->get_size() != 0)
                {
                        this->description_store->remove(indices->get_minimum());
                }
        });
}

void gui::invoice_page::connect_description_list_store()
{
        this->description_store->signal_items_changed().connect(
                        sigc::mem_fun(*this, &invoice_page::update_description_total));
}

void gui::invoice_page::edit_known_invoice(uint position)
{
        auto item = this->invoice_store->get_item(position);
        if (!item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto data = std::dynamic_pointer_cast<invoice_entries>(item);
        if (!data)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The data is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        data::pdf_invoice pdf_invoice{data->pdf_invoice};
        if (pdf_invoice.is_valid() == false)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The invoice data is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        syslog(LOG_INFO, "INVOICE_PAGE: User is editing an invoice - "
                         "filename %s, line number %d", __FILE__, __LINE__);

	data::invoice invoice{pdf_invoice.get_invoice()};
        this->invoice_number->set_text(invoice.get_id());
        this->invoice_date->set_text(invoice.get_date());
        this->job_card->set_text(invoice.get_job_card_number());
        this->order_number->set_text(invoice.get_order_number());
        this->description_total_label->set_text("Total: R " + invoice.get_description_total());
        this->material_total_label->set_text("Total: R " + invoice.get_material_total());
        this->grand_total_label->set_text("Grand Total: R " + invoice.get_grand_total());
        populate_description_store(invoice);
        populate_material_store(invoice);
}

void gui::invoice_page::update_description_total(uint position, uint removed, uint added)
{
        position++;
        removed++;
        added++;

        std::ostringstream doss{""};
        doss << std::fixed << std::setprecision(2) << compute_total(this->description_store);
        this->description_total_label->set_text("Total: R " + doss.str());
        this->description_total = doss.str();

        std::ostringstream moss{""};
        moss << std::fixed << std::setprecision(2) << compute_total(this->material_store);
        this->material_total_label->set_text("Total: R " + moss.str());
        this->material_total = moss.str();

        std::ostringstream goss{""};
        goss << std::fixed << std::setprecision(2) << this->compute_grand_total();
        this->grand_total_label->set_text("Grand Total: R " + goss.str());
        this->grand_total = goss.str();
}

void gui::invoice_page::connect_material_view()
{
        this->material_store = std::shared_ptr<Gio::ListStore<column_entries>>{
                Gio::ListStore<column_entries>::create()};
        if (!this->material_store)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The material_store is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::MultiSelection> selection_model = Gtk::MultiSelection::create(this->material_store);
        if (!selection_model)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The material selection_model is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->material_view->set_model(selection_model);
        this->material_view->set_name("material_view");

        connect_material_add_button();
        connect_material_delete_button(selection_model);
        connect_material_list_store();

        quantity_column(this->material_view);
        description_column(this->material_view);
        amount_column(this->material_view);
}

void gui::invoice_page::connect_material_add_button()
{
        if (!this->material_add_button)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The material_add_button is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->material_add_button->signal_clicked().connect([this] () {
                syslog(LOG_INFO, "INVOICE_PAGE: User is adding a material row - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                this->material_store->append(column_entries::create());
                Glib::signal_timeout().connect_once([this]() {
                        this->material_adjustment->set_value(this->material_adjustment->get_upper());
                }, 30);
        });
}

void gui::invoice_page::connect_material_delete_button(const Glib::RefPtr<Gtk::MultiSelection>& selection_model)
{
        if (!this->material_delete_button)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The material_delete_button is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->material_delete_button->signal_clicked().connect([selection_model, this] () {
                syslog(LOG_INFO, "INVOICE_PAGE: User is removing a material row - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                Glib::RefPtr<const Gtk::Bitset> indices = selection_model->get_selection();
                while (indices->get_size() != 0)
                {
                        this->material_store->remove(indices->get_minimum());
                }
        });
}

void gui::invoice_page::connect_material_list_store()
{
        this->material_store->signal_items_changed().connect(
                        sigc::mem_fun(*this, &invoice_page::update_material_total));
}

void gui::invoice_page::update_material_total(uint position, uint removed, uint added)
{
        position++;
        removed++;
        added++;

        std::ostringstream doss{""};
        doss << std::fixed << std::setprecision(2) << compute_total(this->description_store);
        this->description_total_label->set_text("Total: R " + doss.str());
        this->description_total = doss.str();

        std::ostringstream moss{""};
        moss << std::fixed << std::setprecision(2) << compute_total(this->material_store);
        this->material_total_label->set_text("Total: R " + moss.str());
        this->material_total = moss.str();

        std::ostringstream goss{""};
        goss << std::fixed << std::setprecision(2) << this->compute_grand_total();
        this->grand_total_label->set_text("Grand Total: R " + goss.str());
        this->grand_total = goss.str();
}

void gui::invoice_page::quantity_column(const std::unique_ptr<Gtk::ColumnView>& view)
{
        if (!view)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The view is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::SignalListItemFactory> factory = Gtk::SignalListItemFactory::create();
        if (!factory)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The factory is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::ColumnViewColumn> column = Gtk::ColumnViewColumn::create("Quantity", factory);
        if (!column)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The column is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        factory->signal_setup().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::setup)));
        factory->signal_bind().connect(sigc::mem_fun(*this, &invoice_page::bind_quantity));
        factory->signal_teardown().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::teardown)));
        column->set_expand(false);
        view->append_column(column);
}

void gui::invoice_page::description_column(const std::unique_ptr<Gtk::ColumnView>& view)
{
        if (!view)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The view is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::SignalListItemFactory> factory = Gtk::SignalListItemFactory::create();
        if (!factory)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The factory is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::ColumnViewColumn> column = Gtk::ColumnViewColumn::create("Description", factory);
        if (!column)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The column is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        factory->signal_setup().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::setup)));
        factory->signal_bind().connect(sigc::mem_fun(*this, &invoice_page::bind_description));
        factory->signal_teardown().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::teardown)));
        column->set_expand(true);
        view->append_column(column);
}

void gui::invoice_page::amount_column(const std::unique_ptr<Gtk::ColumnView>& view)
{
        if (!view)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The view is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::SignalListItemFactory> factory = Gtk::SignalListItemFactory::create();
        if (!factory)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The factory is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::ColumnViewColumn> column = Gtk::ColumnViewColumn::create("Amount", factory);
        if (!column)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The column is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        factory->signal_setup().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::setup)));
        factory->signal_bind().connect(sigc::mem_fun(*this, &invoice_page::bind_amount));
        factory->signal_teardown().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::teardown)));
        column->set_expand(false);
        view->append_column(column);
}

void gui::invoice_page::setup(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
        if (!list_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The list_item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        list_item->set_child(*Gtk::make_managed<Gtk::Entry>());
}

void gui::invoice_page::teardown(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
        if (!list_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The list_item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        list_item->unset_child();
}

void gui::invoice_page::bind_quantity(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
        if (!list_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The list_item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto columns = std::dynamic_pointer_cast<column_entries>(list_item->get_item());
        if (!columns)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The bind quantity columns are not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto entry = dynamic_cast<Gtk::Entry*>(list_item->get_child());
        if (!entry)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The bind quantity entry is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        entry->set_max_length(limit::MAX_QUANTITY);
        entry->set_text(std::to_string(columns->quantity));
        entry->signal_changed().connect([entry, columns, this] () {
                std::string text{entry->get_text()};
                if (text.empty())
                {
                        text = "0";
                }

                std::regex int_regex(R"(^[0-9]+$)");
                bool correct_format{std::regex_search(text, int_regex)};
                if (correct_format)
                {
                        columns->quantity = std::stoi(text);
                }
                else
                {
                        (void) this->wrong_data_in_quantity_column_alert.show();
                        entry->select_region(0, limit::MAX_QUANTITY);
                        columns->quantity = 0;
                }
        });
}

void gui::invoice_page::bind_description(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
        if (!list_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The list_item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto columns = std::dynamic_pointer_cast<column_entries>(list_item->get_item());
        if (!columns)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The bind description columns are not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto entry = dynamic_cast<Gtk::Entry*>(list_item->get_child());
        if (!entry)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The bind description entry is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        entry->set_max_length(limit::MAX_DESCRIPTION);
        entry->set_text(columns->description);
        entry->signal_changed().connect([entry, columns, this] () {
                columns->description = entry->get_text();
        });
}

void gui::invoice_page::bind_amount(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
        if (!list_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The list_item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto columns = std::dynamic_pointer_cast<column_entries>(list_item->get_item());
        if (!columns)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The bind amount columns are not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto entry = dynamic_cast<Gtk::Entry*>(list_item->get_child());
        if (!entry)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The bind amount entry is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        std::ostringstream amount_precision{""};
        amount_precision.imbue(std::locale::classic());
        amount_precision << std::fixed << std::setprecision(2) << columns->amount;
        entry->set_max_length(limit::MAX_AMOUNT);
        entry->set_text(amount_precision.str());
        entry->signal_changed().connect([entry, columns, this] () {
                std::string text{entry->get_text()};
                if (text.empty())
                {
                        text = "0";
                }

                std::regex double_regex(R"(^-?([0-9]+(\.[0-9]*)?|\.[0-9]+)$)");
                bool correct_format{std::regex_search(text, double_regex)};
                if (correct_format)
                {
                        columns->amount = std::stod(text);
                        auto ancestor{entry->get_ancestor(Gtk::ColumnView::get_type())};
                        if (ancestor->get_name() == "description_view")
                        {
                                std::ostringstream doss{""};
                                doss << std::fixed << std::setprecision(2) << compute_total(this->description_store);
                                this->description_total_label->set_text("Total: R " + doss.str());
                                this->description_total = doss.str();
                        }

                        if (ancestor->get_name() == "material_view")
                        {
                                std::ostringstream moss{""};
                                moss << std::fixed << std::setprecision(2) << compute_total(this->material_store);
                                this->material_total_label->set_text("Total: R " + moss.str());
                                this->material_total = moss.str();
                        }

                        std::ostringstream goss{""};
                        goss << std::fixed << std::setprecision(2) << this->compute_grand_total();
                        this->grand_total_label->set_text("Grand Total: R " + goss.str());
                        this->grand_total = goss.str();
                }
                else
                {
                        (void) this->wrong_data_in_amount_column_alert.show();
                        entry->select_region(0, limit::MAX_AMOUNT);
                        columns->amount = 0;
                }
        });
}

double gui::invoice_page::compute_total(const Glib::RefPtr<Gio::ListStore<column_entries>>& store)
{
        double total{0.0};
        if (!store)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The store is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
        }
        else
        {
                for (guint i = 0; i < store->get_n_items(); ++i)
                {
                        auto item = store->get_item(i);
                        if (item)
                        {
                                total += item->amount;
                        }
                }
        }

        return total;
}

double gui::invoice_page::compute_grand_total()
{
        double total{this->compute_total(this->description_store) + this->compute_total(this->material_store)};

        return total;
}

data::invoice gui::invoice_page::extract_invoice_data()
{
        data::invoice data{};
        data.set_name(this->business_name);
        data.set_id(this->invoice_number->get_text());
        data.set_date(this->invoice_date->get_text());
        data.set_paid_status("Not Paid");
        data.set_job_card_number(this->job_card->get_text());
        data.set_order_number(this->order_number->get_text());
        data.set_description_total(this->description_total);
        data.set_material_total(this->material_total);
        data.set_grand_total(this->grand_total);
        std::vector<data::column> description_columns{this->retrieve_column_data(this->description_store, data::LOGICAL_TRUE)};
        data.set_description_column(description_columns);
        std::vector<data::column> material_columns{this->retrieve_column_data(this->material_store, data::LOGICAL_FALSE)};
        data.set_material_column(material_columns);

        return data;
}

std::vector<data::column> gui::invoice_page::retrieve_column_data(const Glib::RefPtr<Gio::ListStore<column_entries>>& store,
								  const long long& _is_description)
{
        std::vector<data::column> columns{};
        if (!store)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The store is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
        }
        else
        {
                for (guint i = 0; i < store->get_n_items(); ++i)
                {
                        auto item = store->get_item(i);
                        if (item)
                        {
                                data::column column;
                                column.set_quantity(item->quantity);
                                column.set_description(item->description);
                                column.set_amount(item->amount);
				column.set_row_number(static_cast<long long> (i));
				column.set_is_description(_is_description);
                                columns.push_back(column);
                        }
                }
        }

        return columns;
}

void gui::invoice_page::populate(const std::string& _business_name)
{
        this->invoice_store->remove_all();
        if (_business_name.empty())
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The _business_name is empty - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
        }
        else
        {
		this->business_name.clear();
		this->business_name = _business_name;
		model::invoice invoice_model{app::paths::database()};
		std::vector<std::any> db_invoices = invoice_model.load(_business_name);
		std::vector<data::pdf_invoice> pdf_invoices;
		std::transform(db_invoices.cbegin(),
				db_invoices.cend(),
				std::back_inserter(pdf_invoices),
				[] (const std::any& _pdf_invoice) {
					return std::any_cast<data::pdf_invoice> (_pdf_invoice);
				});

		data::client db_client_data{};
		data::invoice db_invoice_data{};
		for (const data::pdf_invoice& data : pdf_invoices)
		{
			 db_client_data = data.get_client();
			 db_invoice_data = data.get_invoice();
		}

		if (db_client_data.is_valid() == false)
		{
			this->clear();
			syslog(LOG_CRIT, "INVOICE_PAGE: The client does not exist - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			utility::date_manager date_manager{};
			if (db_invoice_data.is_valid() == false)
			{
				int new_invoice_number{0};
				++new_invoice_number;
				this->invoice_number->set_text(std::to_string(new_invoice_number));
				this->invoice_date->set_text(date_manager.current_date());

				syslog(LOG_CRIT, "INVOICE_PAGE: There are no invoices associated with the client - "
						 "filename %s, line number %d", __FILE__, __LINE__);
			}
			else
			{
				data::pdf_invoice pdf_latest_invoice(pdf_invoices.back());
				data::invoice invoice{pdf_latest_invoice.get_invoice()};
				int new_invoice_number{std::stoi(invoice.get_id())};
				++new_invoice_number;
				this->invoice_number->set_text(std::to_string(new_invoice_number));
				this->invoice_date->set_text(date_manager.current_date());
				this->admin_data = pdf_latest_invoice.get_business();
				this->client_data = pdf_latest_invoice.get_client();
				populate_list_store(pdf_invoices);
			}
		}
        }
}

void gui::invoice_page::connect_invoice_view()
{
        this->invoice_store = std::shared_ptr<Gio::ListStore<invoice_entries>>{
                Gio::ListStore<invoice_entries>::create()};
        if (!this->invoice_store)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The invoice_store is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        if (!this->invoice_view)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The invoice_view is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::MultiSelection> selection_model = Gtk::MultiSelection::create(this->invoice_store);
        if (!selection_model)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The invoice selection_model is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->invoice_view->set_name("invoice_view");
        this->invoice_view->set_model(selection_model);
        this->invoice_view->set_single_click_activate(false);
        this->invoice_view->signal_activate().connect(
                sigc::mem_fun(*this, &invoice_page::edit_known_invoice));

        selection_model->signal_selection_changed().connect(
                        sigc::mem_fun(*this, &invoice_page::selected_invoice));

        invoices(this->invoice_view);
}

void gui::invoice_page::invoices(const std::unique_ptr<Gtk::ListView>& view)
{
        if (!view)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The view is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<Gtk::SignalListItemFactory> factory = Gtk::SignalListItemFactory::create();
        if (!factory)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The factory is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        factory->signal_setup().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::invoice_setup)));
        factory->signal_bind().connect(sigc::mem_fun(*this, &invoice_page::bind_invoices));
        factory->signal_teardown().connect(sigc::bind(sigc::mem_fun(*this, &invoice_page::invoice_teardown)));
        view->set_factory(factory);
}

void gui::invoice_page::invoice_setup(const Glib::RefPtr<Gtk::ListItem>& _item)
{
        if (!_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The _item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto label = Gtk::make_managed<Gtk::Label>("", Gtk::Align::START);
        if (!label)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The label is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        label->set_hexpand(true);
        label->set_vexpand(false);
        label->set_size_request(200, 35);
        _item->set_child(*label);
}

void gui::invoice_page::invoice_teardown(const Glib::RefPtr<Gtk::ListItem>& _item)
{
        if (!_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The _item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        _item->unset_child();
}

void gui::invoice_page::bind_invoices(const Glib::RefPtr<Gtk::ListItem>& _item)
{
        if (!_item)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The _item is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto data = std::dynamic_pointer_cast<invoice_entries>(_item->get_item());
        if (!data)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The data is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        auto label = dynamic_cast<Gtk::Label*>(_item->get_child());
        if (!label)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The label is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        data::pdf_invoice pdf_invoice{data->pdf_invoice};
	data::invoice invoice{pdf_invoice.get_invoice()};
        std::string details{"# " + invoice.get_id() + ", " + invoice.get_name() + ", " + invoice.get_date() + " "};
        label->set_text(details);
}

void gui::invoice_page::populate_list_store(const std::vector<data::pdf_invoice>& _pdf_invoices)
{
        if (_pdf_invoices.empty())
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The _invoices are not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        for (const auto& pdf_invoice : _pdf_invoices)
        {
                if (pdf_invoice.is_valid())
                {
                        this->invoice_store->append(invoice_entries::create(pdf_invoice));
                        Glib::signal_timeout().connect_once([this]() {
                                this->invoices_adjustment->set_value(this->invoices_adjustment->get_upper());
                        }, 30);
                }
        }
}

void gui::invoice_page::selected_invoice(uint _position, uint _items_selected)
{
        ++_position;
        ++_items_selected;

        this->invoices_selected.clear();
        Glib::RefPtr<Gtk::SelectionModel> selection_model = this->invoice_view->get_model();
        if (!selection_model)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The selection_model is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        Glib::RefPtr<const Gtk::Bitset> items_list{selection_model->get_selection()};
        for (guint item : *items_list)
        {
                auto data = this->invoice_store->get_item(item);
                if (!data)
                        return;
                auto invoice = std::dynamic_pointer_cast<invoice_entries>(data);
                if (!invoice)
                        return;

                data::pdf_invoice temp{invoice->pdf_invoice};
                this->invoices_selected.push_back(invoice->pdf_invoice);
        }
}

void gui::invoice_page::email_sent()
{
	if (this->email_future.get() == false)
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: Failed to send the email - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		if (this->no_internet_alert.show() == false)
		{
			syslog(LOG_CRIT, "INVOICE_PAGE: Failed to show the no_internet_alert - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
	}
	else
	{
		syslog(LOG_CRIT, "INVOICE_PAGE: Email successfully sent - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
}

void gui::invoice_page::populate_description_store(const data::invoice& _invoice)
{
        if (_invoice.is_valid() == false)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The invoice data is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->description_store->remove_all();
        for (const data::column& column : _invoice.get_description_column())
        {
                if (column.is_valid())
                {
                        this->description_store->append(column_entries::create(
                                                column.get_quantity(),
                                                column.get_description(),
                                                column.get_amount()));
                        Glib::signal_timeout().connect_once([this]() {
                                this->description_adjustment->set_value(this->description_adjustment->get_upper());
                        }, 30);
                }
        }
}

void gui::invoice_page::populate_material_store(const data::invoice& _invoice)
{
        if (_invoice.is_valid() == false)
        {
                syslog(LOG_CRIT, "INVOICE_PAGE: The invoice data is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->material_store->remove_all();
        for (const data::column& column : _invoice.get_material_column())
        {
                if (column.is_valid())
                {
			syslog(LOG_CRIT, "INVOICE_PAGE: The material column data is not valid - "
					 "filename %s, line number %d", __FILE__, __LINE__);
                }
		else
		{
                        this->material_store->append(column_entries::create(
                                                column.get_quantity(),
                                                column.get_description(),
                                                column.get_amount()));
                        Glib::signal_timeout().connect_once([this]() {
                                this->material_adjustment->set_value(this->material_adjustment->get_upper());
                        }, 30);
		}
        }
}

//GCOVR_EXCL_STOP

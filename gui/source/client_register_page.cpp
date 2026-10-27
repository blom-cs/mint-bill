/*******************************************************************************
 * @file client_register_page.cpp
 *
 * @brief Implementation of the gui::client_register_page class, providing the
 *        logic for binding client-registration widgets, interacting with the
 *        underlying data model, and managing user-driven save and search
 *        operations.
 *
 * @details
 * This implementation:
 *   - Retrieves all relevant Gtk::Entry and Gtk::MessageDialog widgets.
 *   - Connects response handlers for save-confirmation and error dialogs.
 *   - Clears and populates UI fields based on database query results.
 *   - Extracts user input into a data::client object used by model::client.
 *   - Logs errors such as invalid data, empty search queries, or missing UI
 *     widgets using syslog for operational visibility.
 *
 * Error handling ensures that the GUI remains responsive and consistent even
 * when data retrieval, saving, or UI binding issues occur.
 *******************************************************************************/
#include <paths.h>
#include <syslog.h>
#include <client_data.h>
#include <client_register_page.h>

//GCOVR_EXCL_START
gui::client_register_page::~client_register_page()
{
}

bool gui::client_register_page::create(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
        bool created{true};
        if (!_ui_builder)
        {
                syslog(LOG_CRIT, "CLIENT_REGISTER_PAGE: the _ui_builder is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                created = false;
        }
        else
        {
                create_entries(_ui_builder);
                connect_save_alert();
                connect_wrong_info_alert();
        }

        return created;
}

bool gui::client_register_page::search(const std::string& _business_name)
{
        bool searched{true};
	this->clear_all_entries();
        if (_business_name.empty())
        {
                syslog(LOG_CRIT, "CLIENT_REGISTER_PAGE: business name is empty - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
		searched = false;
        }
        else
        {
		display_on_ui(_business_name);
        }

        return searched;
}

bool gui::client_register_page::save()
{
	bool success{false};
	if (!this->save_alert_dialog)
	{
                syslog(LOG_CRIT, "CLIENT_REGISTER_PAGE: the save_alert_dialog is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		success = true;
		this->save_alert_dialog->show();
	}

	return success;
}

void gui::client_register_page::create_entries(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
        this->email = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-email-entry")};
        this->cellphone = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-cell-number-entry")};
        this->vat_number = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-vat-number-entry")};
        this->business_name = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-business-name-entry")};
        this->statment_schedule = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-statement-schedule-entry")};
        this->business_area_code = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-area-code-entry")};
        this->business_town_name = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-town-name-entry")};
        this->business_street_address = std::unique_ptr<Gtk::Entry>{
                _ui_builder->get_widget<Gtk::Entry>("register-address-entry")};
        this->save_alert_dialog = std::unique_ptr<Gtk::MessageDialog>{
                _ui_builder->get_widget<Gtk::MessageDialog>("client-save-button-alert")};
        this->wrong_info_alert_dialog = std::unique_ptr<Gtk::MessageDialog>{
                _ui_builder->get_widget<Gtk::MessageDialog>("client-wrong-info-alert")};
}

void gui::client_register_page::connect_save_alert()
{
        if (!this->save_alert_dialog)
        {
                syslog(LOG_CRIT, "CLIENT_REGISTER_PAGE: the save_alert_dialog is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->save_alert_dialog->signal_response().connect([this] (int response) {
                data::client data{extract_page_entries()};
		model::client client_model{app::paths::database()};
                switch(response)
                {
                        case GTK_RESPONSE_YES:
                                syslog(LOG_INFO, "CLIENT_REGISTER_PAGE: user chose to save the client information - "
                                                 "filename %s, line number %d", __FILE__, __LINE__);
                                if (client_model.save(data) == false)
                                {
                                        this->save_alert_dialog->hide();
                                        this->wrong_info_alert_dialog->show();
                                }
                                else
                                {
                                        this->save_alert_dialog->hide();
                                        clear_all_entries();
                                }
                                break;
                        case GTK_RESPONSE_NO:
                                syslog(LOG_INFO, "CLIENT_REGISTER_PAGE: user chose not to save the client information - "
                                                 "filename %s, line number %d", __FILE__, __LINE__);
                                this->save_alert_dialog->hide();
                                break;
                        default:
                                this->save_alert_dialog->hide();
                                break;
                }
        });
}

void gui::client_register_page::connect_wrong_info_alert()
{
        if (!this->wrong_info_alert_dialog)
        {
                syslog(LOG_CRIT, "CLIENT_REGISTER_PAGE: the wrong_info_alert_dialog is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
                return;
        }

        this->wrong_info_alert_dialog->signal_response().connect([this] (int response) {
                switch (response)
                {
                        case GTK_RESPONSE_CLOSE:
                                this->wrong_info_alert_dialog->hide();
                                break;
                        default:
                                this->wrong_info_alert_dialog->hide();
                                break;
                }
        });
}

void gui::client_register_page::clear_all_entries()
{
        this->business_name->set_text("");
        this->business_street_address->set_text("");
        this->business_area_code->set_text("");
        this->business_town_name->set_text("");
        this->cellphone->set_text("");
        this->email->set_text("");
        this->vat_number->set_text("");
        this->statment_schedule->set_text("");
}

void gui::client_register_page::display_on_ui(const std::string& _business_name)
{
	model::client client_model{app::paths::database()};
	data::client data = std::any_cast<data::client> (client_model.load(_business_name));
	if (data.is_valid() == false)
	{
                syslog(LOG_CRIT, "CLIENT_REGISTER_PAGE: Client data is invalid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		this->email->set_text(data.get_email());
		this->vat_number->set_text(data.get_vat_number());
		this->cellphone->set_text(data.get_cellphone());
		this->business_name->set_text(data.get_name());
		this->statment_schedule->set_text(data.get_statement_schedule());
		this->business_area_code->set_text(data.get_area_code());
		this->business_town_name->set_text(data.get_town());
		this->business_street_address->set_text(data.get_address());
	}
}

data::client gui::client_register_page::extract_page_entries()
{
        data::client data{};
        data.set_email(this->email->get_text());
        data.set_vat_number(this->vat_number->get_text());
        data.set_cellphone(this->cellphone->get_text());
        data.set_name(this->business_name->get_text());
        data.set_statement_schedule(this->statment_schedule->get_text());
        data.set_area_code(this->business_area_code->get_text());
        data.set_town(this->business_town_name->get_text());
        data.set_address(this->business_street_address->get_text());

        return data;
}

//GCOVR_EXCL_STOP

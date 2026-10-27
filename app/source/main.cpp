/*****************************************************************************
 * @file    main.cpp
 *
 * @brief
 *   Application entry point and top-level orchestration for the mint-bill GUI.
 *
 * @details
 *   This file defines the `mint_bill` application class and the `main()`
 *   function that bootstraps the GTK application. It is responsible for:
 *
 *     - Creating the Gtk::Application instance and wiring up the activate
 *       callback.
 *     - Loading the UI definition from the .ui file and applying the CSS theme.
 *     - Initializing the main application windows and common GUI components:
 *       * Page stack and stack-driven navigation.
 *       * Global search bar.
 *       * Print, email, and save buttons.
 *     - Constructing and initializing the feature pages:
 *       * Admin page (business details and configuration).
 *       * Client registration page.
 *       * Invoice page (invoice creation, printing, emailing, saving).
 *       * Statement page (statement creation, printing, emailing, saving).
 *     - Opening the plain Turso database and loading persisted admin data,
 *       then showing the admin page or the invoice page.
 *     - Updating the UI
 *       (for example, the organization label) when valid data exists.
 *
 *   Error conditions and unexpected states are reported via syslog with
 *   critical severity, including file name and line number to aid in
 *   diagnostics. The class is intentionally non-copyable and non-movable to
 *   enforce a single owning instance managing process-wide resources such as
 *   the Gtk::Application, shared windows, and dispatcher.
 *
 *****************************************************************************/
#include <gui.h>
#include <stack.h>
#include <syslog.h>
#include <paths.h>
#include <gui_parts.h>
#include <invoice_page.h>
#include <admin_page.h>
#include <admin_model.h>
#include <admin_data.h>
#include <statement_page.h>
#include <client_register_page.h>


class mint_bill {
public:
	mint_bill();
	mint_bill(const mint_bill&) = delete;
	mint_bill(mint_bill&&) = delete;
	mint_bill& operator= (const mint_bill&) = delete;
	mint_bill& operator= (mint_bill&&) = delete;
	~mint_bill() = default;

	[[nodiscard]] bool create();
	[[nodiscard]] int launch(int argc, char** argv);

protected:
	void activate();
	[[nodiscard]] bool load_ui_file(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool settings_setup();
	[[nodiscard]] bool css_setup();
	[[nodiscard]] bool window_setup(const Glib::RefPtr<Gtk::Builder>&);
	void show_starting_page();
	[[nodiscard]] bool stack_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool search_bar_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool print_button_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool email_button_setup(const Glib::RefPtr<Gtk::Builder>&);
	[[nodiscard]] bool save_button_setup(const Glib::RefPtr<Gtk::Builder>&);

private:
	std::shared_ptr<Gtk::Application> app{nullptr};
	std::shared_ptr<Gtk::Window> mint_bill_window{nullptr};
	std::unique_ptr<Gtk::Label> organization_label{nullptr};
	gui::admin_page admin_page{};
	gui::client_register_page client_register_page{};
	gui::invoice_page invoice_page{};
	gui::statement_page statement_page{};
	gui::part::stack stack{"pages-stack"};
	gui::part::search_bar search_bar{"business-name-search"};
	gui::part::sub_button print_button{"print-button"};
	gui::part::sub_button email_button{"email-button"};
	gui::part::sub_button save_button{"save-button"};
};


int main(int argc, char** argv)
{
	int return_code{0};
	try
	{
		mint_bill mint_bill;
		if (mint_bill.create() == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to create mint_bill application - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			return_code = mint_bill.launch(argc, argv);
		}
	}
	catch (...)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to launch the mint_bill application - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}

	return return_code;
}

mint_bill::mint_bill() = default;

bool mint_bill::create()
{
        this->app = Gtk::Application::create(MINT_BILL_APP_ID);
	if (this->app == nullptr)
	{
                syslog(LOG_CRIT, "The mint_bill application is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
		return false;
	}
	else
	{
		this->app->signal_activate().connect(sigc::mem_fun(*this, &mint_bill::activate));
	}

	return true;
}

int mint_bill::launch(int argc, char** argv)
{
	return this->app->run(argc, argv);
}

void mint_bill::activate()
{
	(void)app::paths::database();

        Glib::RefPtr<Gtk::Builder> ui_builder{Gtk::Builder::create()};
	if (ui_builder == nullptr)
	{
                syslog(LOG_CRIT, "MINT_BILL: ui_builder is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
		return;
	}
	else
	{

		if (this->load_ui_file(ui_builder) == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to load the UI file - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->settings_setup() == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to set application settings - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->css_setup() == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to setup the application css - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->window_setup(ui_builder) == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to setup the application windows - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->admin_page.create(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to create the business page - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->client_register_page.create(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to create the client registration page - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->invoice_page.create(ui_builder, this->mint_bill_window) == false)
		{
			syslog(LOG_CRIT, "Failed to create the invoice page - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->statement_page.create(ui_builder, this->mint_bill_window) == false)
		{
			syslog(LOG_CRIT, "Failed to create the statement page - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->print_button_setup(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to setup the print button - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->email_button_setup(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to setup the email button - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->save_button_setup(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to setup the save button - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->search_bar_setup(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to setup the search bar - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		if (this->stack_setup(ui_builder) == false)
		{
			syslog(LOG_CRIT, "Failed to setup the stack - "
					 "filename %s, line number %d", __FILE__, __LINE__);
			return;
		}

		this->show_starting_page();
	}
}

bool mint_bill::load_ui_file(const Glib::RefPtr<Gtk::Builder>& ui_builder)
{
        bool verified{false};
        try
        {
                verified = true;
                ui_builder->add_from_file(app::paths::ui());
        }
        catch(const Glib::FileError& ex)
        {
                std::string error_string{ex.what()};
                syslog(LOG_CRIT, "MINT_BILL: ui file loading - %s", error_string.c_str());
                return verified;
        }
        catch(const Glib::MarkupError& ex)
        {
                std::string error_string{ex.what()};
                syslog(LOG_CRIT, "MINT_BILL: ui file loading - %s", error_string.c_str());
                return verified;
        }
        catch(const Gtk::BuilderError& ex)
        {
                std::string error_string{ex.what()};
                syslog(LOG_CRIT, "MINT_BILL: ui file loading - %s", error_string.c_str());
                return verified;
        }

        return verified;
}

bool mint_bill::stack_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (this->stack.create(_ui_builder) == false)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to create the stack - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		success = this->stack.subscribe("search_bar", [this] (const std::string& _stack_page_name) {
			if (this->search_bar.update(_stack_page_name) == false)
			{
				syslog(LOG_CRIT, "MINT_BILL: failed to update the search_bar - "
						 "filename %s, line number %d", __FILE__, __LINE__);
				return false;
			}
			else
			{
				if (_stack_page_name == "invoice-page" || _stack_page_name == "statement-page")
				{
					if (this->print_button.enable() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to enable the print_button - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}

					if (this->email_button.enable() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to enable the email_button - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}

					if (this->save_button.enable() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to enable the save_button - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else if (_stack_page_name == "register-page" || _stack_page_name == "admin-page")
				{
					if (this->print_button.disable() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to disable the print_button - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}

					if (this->email_button.disable() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to disable the email_button - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}

					if (this->save_button.enable() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to enable the save_button - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else
				{
					syslog(LOG_CRIT, "MINT_BILL: the stack page is not known: %s", _stack_page_name.c_str());

					return false;
				}
			}

			return true;
		});
	}

	return success;
}

bool mint_bill::search_bar_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (this->search_bar.create(_ui_builder) == false)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to create the search_bar - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		success = this->search_bar.subscribe("search", [this] (const std::string& _keyword) {
			if (this->stack.current_page() == "invoice-page")
			{
				if (this->invoice_page.search(_keyword) == false)
				{
					syslog(LOG_CRIT, "MINT_BILL: failed to perform invoice_page search - "
							 "filename %s, line number %d", __FILE__, __LINE__);
				}
			}
			else if (this->stack.current_page() == "statement-page")
			{
				if (this->statement_page.search(_keyword) == false)
				{
					syslog(LOG_CRIT, "MINT_BILL: failed to perform statement_page search - "
							 "filename %s, line number %d", __FILE__, __LINE__);
				}
			}
			else if (this->stack.current_page() == "register-page")
			{
				if (this->client_register_page.search(_keyword) == false)
				{
					syslog(LOG_CRIT, "MINT_BILL: failed to perform register_page search - "
							 "filename %s, line number %d", __FILE__, __LINE__);
				}
			}
			else if (this->stack.current_page() == "admin-page")
			{
				if (this->admin_page.search(_keyword) == false)
				{
					syslog(LOG_CRIT, "MINT_BILL: failed to perform admin_page search - "
							 "filename %s, line number %d", __FILE__, __LINE__);
				}
			}
			else
			{
				syslog(LOG_CRIT, "Search function not implemented for: %s", this->stack.current_page().c_str());

				return false;
			}

			return true;
		});
	}

	return success;
}

bool mint_bill::print_button_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (this->print_button.create(_ui_builder) == false)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to create the print_button - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->print_button.enable() == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to enable the print_button - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->print_button.subscribe([this] () {
				if (this->stack.current_page() == "invoice-page")
				{
					if (this->invoice_page.print() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the invoice_page print - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else if (this->stack.current_page() == "statement-page")
				{
					if (this->statement_page.print() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the statement_page print - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else
				{
					syslog(LOG_CRIT, "MINT_BILL: the print function not implemented for: %s", this->stack.current_page().c_str());

					return false;
				}

				return true;
			});
		}
	}

	return success;
}

bool mint_bill::email_button_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (this->email_button.create(_ui_builder) == false)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to create the email_button - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->email_button.enable() == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to enable the email_button - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->email_button.subscribe([this] () {
				if (this->stack.current_page() == "invoice-page")
				{
					if (this->invoice_page.email() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the invoice_page email - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else if (this->stack.current_page() == "statement-page")
				{
					if (this->statement_page.email() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the statement_page email - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else
				{
					syslog(LOG_CRIT, "MINT_BILL: the print function not implemented for: %s", this->stack.current_page().c_str());

					return false;
				}

				return true;
			});
		}
	}

	return success;
}

bool mint_bill::save_button_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	if (this->save_button.create(_ui_builder) == false)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to create the save_button - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		if (this->save_button.enable() == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: failed to enable the save_button - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
		else
		{
			success = this->save_button.subscribe([this] () {
				if (this->stack.current_page() == "invoice-page")
				{
					if (this->invoice_page.save() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the invoice_page save - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else if (this->stack.current_page() == "statement-page")
				{
					if (this->statement_page.save() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the statement_page save - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else if (this->stack.current_page() == "register-page")
				{
					if (this->client_register_page.save() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the client_register_page save - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else if (this->stack.current_page() == "admin-page")
				{
					if (this->admin_page.save() == false)
					{
						syslog(LOG_CRIT, "MINT_BILL: failed to perform the admin_page save - "
								 "filename %s, line number %d", __FILE__, __LINE__);
					}
				}
				else
				{
					syslog(LOG_CRIT, "MINT_BILL: the print function not implemented for: %s", this->stack.current_page().c_str());

					return false;
				}

				return true;
			});
		}
	}

	return success;
}

bool mint_bill::settings_setup()
{
	bool success{false};
	std::shared_ptr<Gtk::Settings> system_settings = Gtk::Settings::get_default();
	if (system_settings == nullptr)
	{
		syslog(LOG_CRIT, "MINT_BILL: failed to get the default system settings - "
				 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		success = true;
		system_settings->property_gtk_cursor_blink() = false;
		system_settings->property_gtk_application_prefer_dark_theme() = true;
	}

	return success;
}

bool mint_bill::css_setup()
{
	bool success{false};
	std::shared_ptr<Gdk::Display> display = Gdk::Display::get_default();
	std::shared_ptr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
	if (display == nullptr || css_provider == nullptr)
	{
                syslog(LOG_CRIT, "MINT_BILL: the css_provided or display is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
	}
	else
	{
		success = true;
		css_provider->load_from_path(app::paths::css());
		Gtk::StyleContext::add_provider_for_display(
			display,
			css_provider,
			GTK_STYLE_PROVIDER_PRIORITY_USER
		);
	}

	return success;
}

bool mint_bill::window_setup(const Glib::RefPtr<Gtk::Builder>& _ui_builder)
{
	bool success{false};
	this->organization_label = std::unique_ptr<Gtk::Label>{
		_ui_builder->get_widget<Gtk::Label>("organization-name")};
        this->mint_bill_window = std::shared_ptr<Gtk::Window> {
		_ui_builder->get_widget<Gtk::Window>("main-window")};
        if (this->mint_bill_window == nullptr)
        {
                syslog(LOG_CRIT, "MINT_BILL: the windows is not valid - "
                                 "filename %s, line number %d", __FILE__, __LINE__);
        }
        else
        {
                success = true;
                this->mint_bill_window->set_visible(true);
                this->app->add_window(*this->mint_bill_window);
        }

	return success;
}

void mint_bill::show_starting_page()
{
	model::admin admin_model{app::paths::database()};
	std::any data{admin_model.load()};
	data::admin admin_data{std::any_cast<data::admin> (data)};
	if (admin_data.is_valid() == false)
	{
		syslog(LOG_CRIT, "MINT_BILL: the admin data is not valid - "
				 "filename %s, line number %d", __FILE__, __LINE__);
		if (this->stack.set_current_page("admin-page") == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: Failed to make admin-page the visible page - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
	}
	else
	{
		this->organization_label->set_text(admin_data.get_name());
		if (this->stack.set_current_page("invoice-page") == false)
		{
			syslog(LOG_CRIT, "MINT_BILL: Failed to make invoice-page the visible page - "
					 "filename %s, line number %d", __FILE__, __LINE__);
		}
	}
}

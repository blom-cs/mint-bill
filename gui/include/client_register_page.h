/*******************************************************************************
 * @file client_register_page.h
 *
 * @brief Declaration of the gui::client_register_page class, responsible for
 *        creating, displaying, validating, and saving client registration data
 *        within the application's GUI.
 *
 * @details
 * The client_register_page class implements the interface::register_page
 * interface and provides all UI logic related to registering or editing client
 * information. This includes business name, contact details, VAT number,
 * statement schedule, and address information.
 *
 * Responsibilities:
 *   - Bind UI elements using Gtk::Builder for all client-related entry fields.
 *   - Load and display client data from the database.
 *   - Validate user input and extract UI values into a data::client object.
 *   - Present save-confirmation and error-alert dialogs.
 *
 * The class is intentionally non-copyable and non-movable to preserve unique
 * widget ownership and to prevent duplication of GTK state.
 *******************************************************************************/
#ifndef _CLIENT_REGISTER_PAGE_H_
#define _CLIENT_REGISTER_PAGE_H_
#include <gui.h>
#include <client_model.h>

namespace gui {
        //GCOVR_EXCL_START
        class client_register_page : public interface::register_page {
                public:
                        client_register_page() = default;
                        client_register_page(const client_register_page&) = delete;
                        client_register_page(client_register_page&&) = delete;
                        client_register_page& operator= (const client_register_page&) = delete;
                        client_register_page& operator= (client_register_page&&) = delete;
                        ~client_register_page() override;

			[[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&) override;
			[[nodiscard]] virtual bool search(const std::string&) override;
			[[nodiscard]] virtual bool save() override;

                private:
                        void create_entries(const Glib::RefPtr<Gtk::Builder>&);
                        void connect_save_alert();
                        void connect_wrong_info_alert();
                        void clear_all_entries();
                        void display_on_ui(const std::string&);
                        data::client extract_page_entries();

                private:
                        std::unique_ptr<Gtk::Entry> email{};
                        std::unique_ptr<Gtk::Entry> cellphone{};
                        std::unique_ptr<Gtk::Entry> vat_number{};
                        std::unique_ptr<Gtk::Entry> business_name{};
                        std::unique_ptr<Gtk::Entry> statment_schedule{};
                        std::unique_ptr<Gtk::Entry> business_area_code{};
                        std::unique_ptr<Gtk::Entry> business_town_name{};
                        std::unique_ptr<Gtk::Entry> business_street_address{};
                        std::unique_ptr<Gtk::MessageDialog> wrong_info_alert_dialog{};
                        std::unique_ptr<Gtk::MessageDialog> save_alert_dialog{};
        };

        //GCOVR_EXCL_STOP
}
#endif

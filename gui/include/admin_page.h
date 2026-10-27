/*******************************************************************************
 * @file admin_page.h
 *
 * @brief Declaration of the gui::admin_page class responsible for managing
 *        business (admin) information within the application's GUI layer.
 *
 * @details
 * The admin_page class implements the interface::register_page interface and
 * provides the user-facing functionality for viewing, editing, validating, and
 * saving business-related data. This includes organization details, contact
 * information, banking data, and the SMTP email password.
 *
 * Responsibilities:
 *   - Bind to GTK widgets using a Gtk::Builder UI definition.
 *   - Populate the UI from database records via model::admin.
 *   - Extract widget state into a data::admin object for persistence.
 *   - Present confirmation dialogs and error alerts.
 *
 * The class is intentionally non-copyable and non-movable to prevent accidental
 * duplication of GTK widget ownership and ensure GUI correctness.
 *******************************************************************************/
#ifndef _ADMIN_PAGE_H_
#define _ADMIN_PAGE_H_
#include <gui.h>
#include <admin_model.h>

namespace gui {
//GCOVR_EXCL_START
class admin_page : public interface::register_page {
public:
	admin_page() = default;
	admin_page(const admin_page&) = delete;
	admin_page(admin_page&&) = delete;
	admin_page& operator= (const admin_page&) = delete;
	admin_page& operator= (admin_page&&) = delete;
	~admin_page() override;

	[[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&) override;
	[[nodiscard]] virtual bool search(const std::string&) override;
	[[nodiscard]] virtual bool save() override;

private:
	void create_entries(const Glib::RefPtr<Gtk::Builder>&);
	void connect_wrong_info_alert();
	void connect_save_alert();
	void clear_entries();
	void update_business_info_with_db_data(const std::string&);
	data::admin extract_page_entries();

private:

private: // Label
	Gtk::Label* organization_label{nullptr};

private: // Entries
	std::unique_ptr<Gtk::Entry> name{};
	std::unique_ptr<Gtk::Entry> street_address{};
	std::unique_ptr<Gtk::Entry> area_code{};
	std::unique_ptr<Gtk::Entry> town_name{};
	std::unique_ptr<Gtk::Entry> cellphone{};
	std::unique_ptr<Gtk::Entry> email{};
	std::unique_ptr<Gtk::Entry> bank_name{};
	std::unique_ptr<Gtk::Entry> branch_code{};
	std::unique_ptr<Gtk::Entry> account_number{};
	std::unique_ptr<Gtk::Entry> client_message{};
	std::unique_ptr<Gtk::PasswordEntry> password{};

private: // Dialogs
	std::unique_ptr<Gtk::MessageDialog> wrong_info_alert_dialog{};
	std::unique_ptr<Gtk::MessageDialog> save_alert_dialog{};
};

//GCOVR_EXCL_STOP
}
#endif

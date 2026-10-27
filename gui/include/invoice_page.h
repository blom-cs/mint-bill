/*******************************************************************************
 * @file invoice_page.h
 *
 * @brief Declaration of the gui::invoice_page class and supporting data
 *        structures used to create, edit, save, print, and email invoices
 *        within the GUI.
 *
 * @details
 * This header defines:
 *   - gui::column_entries: a Glib::Object wrapper representing a single row in
 *     a line-item column (quantity, description, amount).
 *   - gui::invoice_entries: a Glib::Object wrapper holding a data::pdf_invoice
 *     for display in the “known invoices” list.
 *   - gui::invoice_page: an implementation of interface::operations_page that
 *     provides the full invoice workflow in the UI.
 *
 * The invoice_page class is responsible for:
 *   - Creating and binding Gtk::ColumnView / Gtk::ListView models and factories
 *     for materials, descriptions, and known invoices.
 *   - Managing Gtk entries, labels, buttons, dialogs, and scroll adjustments
 *     related to invoice data.
 *   - Loading invoice and client/business data from the database via
 *     model::invoice, and populating the GUI accordingly.
 *   - Extracting user-edited invoice data back into data::invoice, including
 *     description and material columns (data::column).
 *   - Computing per-column totals and the overall grand total, keeping labels
 *     in sync with list-store contents.
 *   - Orchestrating save/print/email operations using dialog wrappers
 *     (part::dialog), feature::email, and gui::part::printer.
 *   - Performing asynchronous email sending and signaling completion through
 *     Glib::Dispatcher.
 *
 * The class is non-copyable and non-movable to avoid duplicating complex GUI
 * state and shared model/list-store ownership.
 *******************************************************************************/
#ifndef _INVOICE_PAGE_H_
#define _INVOICE_PAGE_H_
#include <gui.h>
#include <invoice_pdf.h>
#include <regex>
#include <future>
#include <thread>
#include <vector>
#include <iomanip>
#include <sstream>
#include <email.h>
#include <printer.h>
#include <gui_parts.h>
#include <column_data.h>
#include <invoice_data.h>
#include <invoice_model.h>
#include <pdf_invoice_data.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page-renderer.h>

namespace gui {
        //GCOVR_EXCL_START
        struct column_entries : public Glib::Object {
                public:
                        unsigned int quantity{0};
                        std::string description{""};
                        double amount{0.0};

                        static Glib::RefPtr<column_entries> create()
                        {
                                return Glib::make_refptr_for_instance<column_entries>(new column_entries());
                        }

                        static Glib::RefPtr<column_entries> create(const unsigned int& _quantity,
                                                                   const std::string& _description,
                                                                   const double& _amount)
                        {
                                return Glib::make_refptr_for_instance<column_entries>(new column_entries(
                                                        _quantity, _description, _amount));
                        }

                protected:
                        column_entries() {}
                        explicit column_entries(const unsigned int& _quantity,
                                                const std::string& _description,
                                                const double& _amount)
                                                : quantity{_quantity},
                                                  description{_description},
                                                  amount{_amount} {}
        };

        struct invoice_entries : public Glib::Object {
                public:
                        data::pdf_invoice pdf_invoice{};

                        static Glib::RefPtr<invoice_entries> create(const data::pdf_invoice& _pdf_invoice)
                        {
                                return Glib::make_refptr_for_instance<invoice_entries>(new invoice_entries(_pdf_invoice));
                        }

                protected:
                        explicit invoice_entries(const data::pdf_invoice& _pdf_invoice) : pdf_invoice{_pdf_invoice} {}
        };

        class invoice_page : public interface::operations_page {
                public:
                        invoice_page();
                        invoice_page(const invoice_page&) = delete;
                        invoice_page(invoice_page&&) = delete;
                        invoice_page& operator= (const invoice_page&) = delete;
                        invoice_page& operator= (invoice_page&&) = delete;
                        ~invoice_page() override;

			[[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&,
							  const std::shared_ptr<Gtk::Window>&) override;
			[[nodiscard]] virtual bool search(const std::string&) override;
			[[nodiscard]] virtual bool print() override;
			[[nodiscard]] virtual bool email() override;
			[[nodiscard]] virtual bool save() override;

                private: // GUI creations
                        void create_views(const Glib::RefPtr<Gtk::Builder>&);
                        void create_entries(const Glib::RefPtr<Gtk::Builder>&);
                        void create_dialogs(const Glib::RefPtr<Gtk::Builder>&);
                        void create_buttons(const Glib::RefPtr<Gtk::Builder>&);
			void clear();
			bool save_setup(const Glib::RefPtr<Gtk::Builder>&);
			bool email_setup(const Glib::RefPtr<Gtk::Builder>&);
			bool print_setup(const Glib::RefPtr<Gtk::Builder>&,
                                               const std::shared_ptr<Gtk::Window>&);
			bool wrong_info_setup(const Glib::RefPtr<Gtk::Builder>&);
			bool no_internet_setup(const Glib::RefPtr<Gtk::Builder>&);
			bool no_item_selected_setup(const Glib::RefPtr<Gtk::Builder>&);
			bool wrong_data_in_amount_column_setup(const Glib::RefPtr<Gtk::Builder>&);
			bool wrong_data_in_quantity_column_setup(const Glib::RefPtr<Gtk::Builder>&);

                private: // Button events
                        void connect_material_add_button();
                        void connect_material_delete_button(const Glib::RefPtr<Gtk::MultiSelection>&);
                        void connect_description_add_button();
                        void connect_description_delete_button(const Glib::RefPtr<Gtk::MultiSelection>&);

                private: // Email events
                        void email_sent();

                private: // Invoice
                        void populate(const std::string&);
                        void connect_invoice_view();
                        void invoices(const std::unique_ptr<Gtk::ListView>&);
                        void invoice_setup(const Glib::RefPtr<Gtk::ListItem>&);
                        void bind_invoices(const Glib::RefPtr<Gtk::ListItem>&);
                        void invoice_teardown(const Glib::RefPtr<Gtk::ListItem>&);
                        void populate_list_store(const std::vector<data::pdf_invoice>&);
                        void selected_invoice(uint, uint);

                private: // View events
                        void connect_material_view();
                        void connect_description_view();
                        void connect_material_list_store();
                        void connect_description_list_store();
                        void edit_known_invoice(uint);

                private: // helper
                        void setup(const Glib::RefPtr<Gtk::ListItem>&);
                        void teardown(const Glib::RefPtr<Gtk::ListItem>&);
                        void bind_amount(const Glib::RefPtr<Gtk::ListItem>&);
                        void bind_quantity(const Glib::RefPtr<Gtk::ListItem>&);
                        void bind_description(const Glib::RefPtr<Gtk::ListItem>&);
                        void amount_column(const std::unique_ptr<Gtk::ColumnView>&);
                        void quantity_column(const std::unique_ptr<Gtk::ColumnView>&);
                        void description_column(const std::unique_ptr<Gtk::ColumnView>&);
                        void update_material_total(uint, uint, uint);
                        void update_description_total(uint, uint, uint);
                        void populate_description_store(const data::invoice&);
                        void populate_material_store(const data::invoice&);
                        [[nodiscard]] double compute_grand_total();
                        [[nodiscard]] data::invoice extract_invoice_data();
                        [[nodiscard]] double compute_total(const Glib::RefPtr<Gio::ListStore<column_entries>>&);
                        [[nodiscard]] std::vector<data::column> retrieve_column_data(const Glib::RefPtr<Gio::ListStore<column_entries>>&,
										     const long long&);

                private: // Member features
                        int number_of_pages{0};
                        bool email_success{false};
			std::future<bool> email_future;
                        std::string paid_status{"Not Paid"};
                        std::string grand_total{""};
			std::string business_name{""};
			data::admin admin_data{};
			data::client client_data{};
                        data::invoice invoice_edit{};
                        std::string material_total{""};
                        std::string description_total{""};
                        Glib::Dispatcher email_dispatcher{};
                        std::vector<std::any> invoices_selected{};

                private: // Member Entries
                        std::unique_ptr<Gtk::Entry> job_card{};
                        std::unique_ptr<Gtk::Entry> order_number{};
                        std::unique_ptr<Gtk::Entry> invoice_date{};
                        std::unique_ptr<Gtk::Entry> invoice_number{};
                        std::unique_ptr<Gtk::SearchEntry> search_entry{};

                private: // Member Views
                        std::unique_ptr<Gtk::ColumnView> material_view{};
                        std::unique_ptr<Gtk::ColumnView> description_view{};
                        std::shared_ptr<Gtk::Adjustment> material_adjustment{};
                        std::shared_ptr<Gtk::Adjustment> description_adjustment{};
                        std::shared_ptr<Gio::ListStore<column_entries>> material_store{};
                        std::shared_ptr<Gio::ListStore<column_entries>> description_store{};

                private: // Member buttons
                        std::unique_ptr<Gtk::Button> material_add_button{};
                        std::unique_ptr<Gtk::Button> material_delete_button{};
                        std::unique_ptr<Gtk::Button> description_add_button{};
                        std::unique_ptr<Gtk::Button> description_delete_button{};

                private: // Member labels
                        std::unique_ptr<Gtk::Label> grand_total_label{};
                        std::unique_ptr<Gtk::Label> material_total_label{};
                        std::unique_ptr<Gtk::Label> description_total_label{};

                private: // Member Message dialogs
			part::dialog email_alert{"invoice-email-alert"};
			part::dialog save_alert{"invoice-save-button-alert"};
			part::dialog print_alert{"invoice-print-button-alert"};
			part::dialog wrong_info_alert{"invoice-wrong-info-alert"};
			part::dialog no_internet_alert{"invoice-email-no-internet-alert"};
			part::dialog no_item_selected_alert{"invoice-no-item-selected-alert"};
			part::dialog wrong_data_in_amount_column_alert{"invoice-data-in-amount-column-alert"};
			part::dialog wrong_data_in_quantity_column_alert{"invoice-data-in-quantity-column-alert"};

                private:
                        std::unique_ptr<Gtk::ListView> invoice_view{};
                        std::shared_ptr<Gtk::Adjustment> invoices_adjustment{};
                        std::shared_ptr<Gio::ListStore<invoice_entries>> invoice_store{};
        };

        //GCOVR_EXCL_STOP
}
#endif

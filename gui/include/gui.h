/******************************************************************************
 * @filename gui.h
 *
 * @brief GUI interface definitions for high-level application UI behavior.
 *
 * @details
 * This header defines abstract GUI interfaces that must be implemented by
 * concrete UI components. These interfaces allow the application to construct
 * GUI pages, bind UI elements, handle user actions, and interact with
 * application observers. They provide a consistent contract enabling modular,
 * testable UI design across different pages of the system.
 *
 * Interfaces:
 * - @interface gui: Responsible for initializing the main UI structure from a
 *   Gtk::Builder instance and attaching the primary observer.
 *
 * - @interface register_page: Defines the behavior of the registration page,
 *   including construction, database searches, and saving.
 *
 * - @interface operations_page: Defines the behavior of the operations page,
 *   including construction, database searches, printing, emailing, and saving.
 ******************************************************************************/
#ifndef _GUI_H_
#define _GUI_H_
#include <part.h>
#include <gtkmm.h>

namespace interface {

/*****************************************************************************
 * @brief Interface for constructing and initializing the main GUI.
 *
 * @details
 * The gui interface abstracts the initialization of the primary UI layout.
 * Implementations must construct UI components using the provided Gtk::Builder
 * and attach an observer for event notifications.
 *
 * @param builder  A Gtk::Builder reference used to construct UI widgets.
 * @param observer An observer receiving notifications for UI events.
 *
 * @return true if initialization succeeds; false otherwise.
 *****************************************************************************/
class gui {
public:
        virtual ~gui() = default;

        [[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&, const interface::observer&) = 0;
};

/*****************************************************************************
 * @brief Interface for the registration page of the application.
 *
 * @details
 * The register_page interface defines all required actions for the
 * registration workflow. Implementations should:
 * - Build the page from a Gtk::Builder.
 * - Perform search operations for client or invoice retrieval.
 * - Save data after user modifications.
 *
 * @param builder A Gtk::Builder reference for UI construction.
 * @param keyword A search term for lookup operations.
 *
 * @return true on success; false otherwise.
 *****************************************************************************/
class register_page {
public:
        virtual ~register_page() = default;

        [[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&) = 0;
        [[nodiscard]] virtual bool search(const std::string&) = 0;
        [[nodiscard]] virtual bool save() = 0;
};

/*****************************************************************************
 * @brief Interface for the operations page of the application.
 *
 * @details
 * The operations_page interface governs workflow-related actions such as:
 * - Page construction using Gtk::Builder and the main window handle.
 * - Searching, printing, emailing, and saving operational data.
 *
 * @param builder A Gtk::Builder reference for UI construction.
 * @param window  The parent window used for dialog associations.
 * @param keyword A keyword used for searching stored data.
 *
 * @return true on success; false otherwise.
 *****************************************************************************/
class operations_page {
public:
        virtual ~operations_page() = default;

        [[nodiscard]] virtual bool create(const Glib::RefPtr<Gtk::Builder>&, const std::shared_ptr<Gtk::Window>&) = 0;
        [[nodiscard]] virtual bool search(const std::string&) = 0;
        [[nodiscard]] virtual bool print() = 0;
        [[nodiscard]] virtual bool email() = 0;
        [[nodiscard]] virtual bool save() = 0;
};
}
#endif

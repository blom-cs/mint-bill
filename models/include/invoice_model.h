/*******************************************************************************
 * @file invoice_model.h
 *
 * @brief Invoice domain model and operations interface implementation.
 *
 * @details
 * Declares the `model::invoice` class, which implements the
 * `interface::model_operations` interface for invoice-related workflows.
 *
 * Responsibilities:
 *  - Load invoice-related data for a given business from persistent storage
 *    and assemble it into higher-level domain objects.
 *  - Persist invoice data (including related labor/line items and statement
 *    information) to the database.
 *  - Prepare invoice data for downstream use cases:
 *      * `prepare_for_email()` – build a `data::email` object with all
 *        necessary context and PDF attachments.
 *      * `prepare_for_print()` – generate raw PDF documents as strings for
 *        printing.
 *
 * The model holds:
 *  - `database_file` : path to the plain Turso database.
 ******************************************************************************/
#ifndef _INVOICE_MODEL_H_
#define _INVOICE_MODEL_H_
#include <string>
#include <models.h>
#include <pdf_invoice_data.h>

namespace model {
class invoice: public interface::model_operations {
public:
	invoice() = delete;
	explicit invoice(const std::string&);
	invoice(const invoice&) = delete;
	invoice(invoice&&) = delete;
	invoice& operator=(const invoice&) = delete;
	invoice& operator=(invoice&&) = delete;
	virtual ~invoice() override;

	[[nodiscard]] virtual std::vector<std::any> load(const std::string&) const override;
	[[nodiscard]] virtual bool save(const std::any&) const override;
	[[nodiscard]] virtual data::email prepare_for_email(const std::vector<std::any>&) const override;
	[[nodiscard]] virtual std::vector<std::string> prepare_for_print(const std::vector<std::any>&) const override;


private:
	std::string database_file;

};
}
#endif

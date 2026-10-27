/*******************************************************************************
 * @file statement_model.h
 *
 * @brief Declaration of the model::statement class used to load, save, and
 *        generate printable/email-ready customer statements.
 *
 * @details
 * The model::statement class implements the interface::model_operations API and
 * provides model-layer functionality for:
 *
 *   • Loading statement records for a specific business/client.
 *   • Saving a statement entry and its associated metadata to the database.
 *   • Preparing statement data for email distribution (PDF generation + metadata).
 *   • Preparing statement data for printing (PDF file generation).
 *
 * Responsibilities:
 *   - Wrap lower-level serializer/database logic behind a clean high-level API.
 *   - Convert SQL query results into domain objects using serialize::statement.
 *   - Invoke the PDF generator and convert PDF handles into string paths.
 *   - Enforce basic validation of provided input (business name, statement data).
 *
 * Protected helpers:
 *   - convert_pdfs_to_strings() — Turns std::any PDF handles into file paths for print workflows.
 *
 * The statement model owns the database file path used for all database
 * operations. This class is non-copyable but movable.
 *******************************************************************************/
#ifndef _STATEMENT_MODEL_H_
#define _STATEMENT_MODEL_H_
#include <models.h>

namespace model {
class statement: public interface::model_operations {
public:
	statement() = delete;
	explicit statement(const std::string&);
	statement(const statement&) = default;
	statement(statement&&) = default;
	statement& operator= (const statement&) = default;
	statement& operator= (statement&&) = default;
	virtual ~statement() override;

	[[nodiscard]] virtual std::vector<std::any> load(const std::string&) const override;
	[[nodiscard]] virtual bool save(const std::any&) const override;
	[[nodiscard]] virtual data::email prepare_for_email(const std::vector<std::any>&) const override;
	[[nodiscard]] virtual std::vector<std::string> prepare_for_print(const std::vector<std::any>&) const override;

protected:
	[[nodiscard]] virtual std::vector<std::string> convert_pdfs_to_strings(const std::vector<std::any>&) const;

private:
	std::string database_file;

};
}
#endif

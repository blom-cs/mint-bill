/*******************************************************************************
 * @file client_model.h
 *
 * @brief Client model interface to the persistence layer.
 *
 * @details
 * Declares the `model::client` class, which implements the
 * `interface::model_register` interface to provide a model-layer façade for
 * client records.
 *
 * Responsibilities:
 *  - Load a single `data::client` record associated with a given business
 *    name from the backing SQLite database.
 *  - Save a `data::client` instance and its associated business details using
 *    transactional SQL operations.
 *
 * Design notes:
 *  - The model owns only the database connection configuration
 *    (`database_file`); no connection is kept open
 *    between calls.
 *  - All I/O is funneled through the generic `load()` / `save()` interface
 *    inherited from `interface::model_register`, using `std::any` as the
 *    transport type between layers.
 *  - Internal `DATA_FIELDS` and `PARAMETERS` enums document the expected
 *    column and parameter ordering used by the serialization and SQL layers.
 ******************************************************************************/
#ifndef _CLIENT_MODEL_H_
#define _CLIENT_MODEL_H_
#include <models.h>

namespace model {
class client: public interface::model_register {
public:
	client() = delete;
	explicit client(const std::string&);
	client(const client&) = delete;
	client(client&&) = delete;
	client& operator= (const client&) = delete;
	client& operator= (client&&) = delete;
	virtual ~client() override;

	[[nodiscard]] virtual std::any load(const std::string&) override;
	[[nodiscard]] virtual bool save(const std::any&) override;

private:
	std::string database_file{""};


	enum DATA_FIELDS {
		NAME = 0,
		EMAIL,
		CELLPHONE_NUMBER,
		ADDRESS,
		AREA_CODE,
		TOWN_NAME,
		VAT_NUMBER,
		STATEMENT_SCHEDULE
	};

	enum PARAMETERS {
		DETAILS = 0,
		CLIENT
	};
};
}
#endif

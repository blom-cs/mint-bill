/*******************************************************************************
 * @file admin_model.h
 *
 * @brief Admin model interface for loading and saving administrator data.
 *
 * @details
 * This header declares the `model::admin` class, which implements the
 * `interface::model_register` interface to provide persistence for
 * administrator-related data such as contact details, banking information,
 * and client messaging.
 *
 * The model encapsulates:
 *  - The path of the plain Turso database. No connection is kept open.
 *  - An API to load admin records either without a key or by business name.
 *  - An API to save admin data received as a type-erased `std::any`.
 *
 * Internally, the model collaborates with serialization helpers and the
 * SQLite wrapper to convert between database rows and strongly-typed
 * `data::admin` / `data::business` objects, ensuring that UI and other
 * layers remain decoupled from SQL and schema details.
 *******************************************************************************/
#ifndef _ADMIN_MODEL_H_
#define _ADMIN_MODEL_H_
#include <models.h>
#include <sqlite.h>

namespace model {
class admin: public interface::model_register {
public:
	admin() = delete;
	explicit admin(const std::string&);
	admin(const admin& new_data) = delete;
	admin(admin&&) = default;
	admin& operator= (const admin&) = delete;
	admin& operator= (admin&&) = delete;
	virtual ~admin() override;

	[[nodiscard]] virtual std::any load();
	[[nodiscard]] virtual std::any load(const std::string&) override;
	[[nodiscard]] virtual bool save(const std::any&) override;

private:
	std::string database_file{""};

	enum DATA_FIELDS {
		NAME = 0,
		EMAIL,
		CELLPHONE,
		ADDRESS,
		AREA_CODE,
		TOWN,
		BANK,
		BRANCH_CODE,
		ACCOUNT_NUMBER,
		APP_PASSWORD,
		CLIENT_MESSAGE
	};

	enum PARAMETERS {
		DETAILS = 0,
		ADMIN
	};
};
}
#endif

/*****************************************************************************
 * @file paths.cpp
 *
 * @brief
 *   Resolves UI/CSS/email/database paths. Installed builds use ~/.local/share;
 *   the live database is seeded once and never overwritten.
 *
 *****************************************************************************/
#include <paths.h>
#include <cstdlib>
#include <string>
#include <syslog.h>
#include <filesystem>
#include <system_error>


#ifdef MINTBILL_USE_USER_DATA_DIR
namespace {
std::filesystem::path user_data_dir()
{
	std::filesystem::path root;
	if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && xdg[0] != '\0')
	{
		root = std::filesystem::path{xdg};
	}
	else
	{
		const char* home = std::getenv("HOME");
		if (home == nullptr || home[0] == '\0')
		{
			root = std::filesystem::path{"."};
		}
		else
		{
			root = std::filesystem::path{home} / ".local" / "share";
		}
	}

	const std::filesystem::path dir{root / MINT_BILL_APP_ID};
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
	if (ec)
	{
		syslog(LOG_CRIT, "PATHS: failed to create data directory %s - "
				 "filename %s, line number %d", dir.c_str(), __FILE__, __LINE__);
	}

	return dir;
}

std::string file_in_data_dir(const std::string& name)
{
	return (user_data_dir() / name).string();
}

std::string seed_user_database()
{
	const std::filesystem::path dir{user_data_dir()};
	const std::filesystem::path dest{dir / (std::string{MINT_BILL_APP_NAME} + ".db")};
	if (std::filesystem::exists(dest))
	{
		return dest.string();
	}

	const std::filesystem::path seed{dir / (std::string{MINT_BILL_APP_NAME} + ".db.template")};
	if (!std::filesystem::exists(seed))
	{
		syslog(LOG_CRIT, "PATHS: template database missing at %s - "
				 "filename %s, line number %d", seed.c_str(), __FILE__, __LINE__);
		return dest.string();
	}

	std::error_code ec;
	std::filesystem::copy_file(seed, dest, std::filesystem::copy_options::skip_existing, ec);
	if (ec)
	{
		syslog(LOG_CRIT, "PATHS: failed to seed database %s - "
				 "filename %s, line number %d", dest.c_str(), __FILE__, __LINE__);
	}

	return dest.string();
}
}
#endif

const std::string& app::paths::ui()
{
	static const std::string path = [] {
#ifdef MINTBILL_USE_USER_DATA_DIR
		return file_in_data_dir("mint-bill.ui");
#else
		return std::string{MINTBILL_UI_PATH};
#endif
	}();

	return path;
}

const std::string& app::paths::css()
{
	static const std::string path = [] {
#ifdef MINTBILL_USE_USER_DATA_DIR
		return file_in_data_dir("mint-bill.css");
#else
		return std::string{MINTBILL_CSS_PATH};
#endif
	}();

	return path;
}

const std::string& app::paths::email_txt()
{
	static const std::string path = [] {
#ifdef MINTBILL_USE_USER_DATA_DIR
		return file_in_data_dir("email.txt");
#else
		return std::string{MINTBILL_EMAIL_TXT_PATH};
#endif
	}();

	return path;
}

const std::string& app::paths::email_html()
{
	static const std::string path = [] {
#ifdef MINTBILL_USE_USER_DATA_DIR
		return file_in_data_dir("email.html");
#else
		return std::string{MINTBILL_EMAIL_HTML_PATH};
#endif
	}();

	return path;
}

const std::string& app::paths::database()
{
	static const std::string path = [] {
#ifdef MINTBILL_USE_USER_DATA_DIR
		return seed_user_database();
#else
		return std::string{MINTBILL_DB_PATH};
#endif
	}();

	return path;
}

/*****************************************************************************
 * @file paths.h
 *
 * @brief
 *   Runtime path helpers for Mint-Bill assets and the live Turso database.
 *
 * @details
 *   Development builds (no MINTBILL_USE_USER_DATA_DIR) use compile-time
 *   source-tree paths.
 *
 *   Installed builds resolve everything under the user's XDG data dir:
 *
 *     $XDG_DATA_HOME/<app-id>/          (default ~/.local/share/<app-id>/)
 *       mint-bill.ui
 *       mint-bill.css
 *       email.txt
 *       email.html
 *       <app-name>.db.template          (seed only; replaced on upgrade)
 *       <app-name>.db                   (live DB; never replaced)
 *
 *   The live database is seeded from the template on first run only
 *   (copy_options::skip_existing), so application updates cannot wipe
 *   invoices, clients, or statements.
 *
 *****************************************************************************/
#ifndef _PATHS_H_
#define _PATHS_H_
#include <string>

namespace app {
namespace paths {
	[[nodiscard]] const std::string& ui();
	[[nodiscard]] const std::string& css();
	[[nodiscard]] const std::string& email_txt();
	[[nodiscard]] const std::string& email_html();
	[[nodiscard]] const std::string& database();
}
}
#endif

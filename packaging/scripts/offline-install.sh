#!/bin/bash
################################################################################
# Install Mint-Bill from the RPMs sitting next to this script.
#
# Stops the existing user services, removes the old program files, then unpacks
# the new payloads into the user's home directory. A live Turso database is
# never replaced by the empty template shipped in the RPM.
#
# Usage (from the unpacked bundle):
#   ./install.sh
#   ./install.sh --install
#   ./install.sh --uninstall
################################################################################
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ACTION=""
HELP=0

log() {
	printf 'INFO: %s\n' "$*"
}

die() {
	printf 'ERROR: %s\n' "$*" >&2
	exit 1
}

usage() {
	cat <<'EOF'
Usage: ./install.sh <action>

Run with no arguments to show this help. An action is required.

Actions:
  --install
      Install mint-bill-tme and mint-bill-tce for this user and
      start both programs. A live Turso database is left in place.
  --uninstall
      Remove mint-bill-tme and mint-bill-tce, their user services,
      data directories, and backup directories.

Examples:
  ./install.sh --install
  ./install.sh --uninstall
EOF
}

set_action() {
	local next="$1"
	if [[ -n "${ACTION}" && "${ACTION}" != "${next}" ]]; then
		die "choose only one of --install or --uninstall"
	fi
	ACTION="${next}"
}

data_home() {
	if [[ -n "${XDG_DATA_HOME:-}" ]]; then
		printf '%s\n' "${XDG_DATA_HOME}"
	else
		printf '%s\n' "${HOME}/.local/share"
	fi
}

run_systemctl() {
	if [[ "${MINT_BILL_NO_SYSTEMD:-}" == "1" ]]; then
		return 0
	fi
	if ! command -v systemctl >/dev/null 2>&1; then
		log "systemctl is not available; skipped: systemctl $*"
		return 0
	fi
	systemctl --user "$@" || log "systemctl --user $* failed"
}

mint_bill_units() {
	printf '%s\n' \
		mint-bill.service \
		mint-bill-db-backup.service \
		mint-bill-db-backup.timer \
		mint-bill-tme.service \
		mint-bill-tme-db-backup.service \
		mint-bill-tme-db-backup.timer \
		mint-bill-tce.service \
		mint-bill-tce-db-backup.service \
		mint-bill-tce-db-backup.timer
}

unit_loaded() {
	local state
	state="$(systemctl --user show -p LoadState --value "$1" 2>/dev/null || true)"
	[[ "${state}" == "loaded" || "${state}" == "masked" ]]
}

systemctl_loaded() {
	local action="$1"
	local unit
	if [[ "${MINT_BILL_NO_SYSTEMD:-}" == "1" ]]; then
		return 0
	fi
	if ! command -v systemctl >/dev/null 2>&1; then
		log "systemctl is not available; skipped: systemctl ${action}"
		return 0
	fi
	while IFS= read -r unit; do
		[[ -n "${unit}" ]] || continue
		unit_loaded "${unit}" || continue
		if [[ "${action}" == "disable" ]]; then
			local enabled
			enabled="$(systemctl --user is-enabled "${unit}" 2>/dev/null || true)"
			[[ "${enabled}" == "enabled" || "${enabled}" == "alias" ]] || continue
		fi
		systemctl --user "${action}" "${unit}" || log "could not ${action} ${unit}"
	done < <(mint_bill_units)
}

stop_units() {
	systemctl_loaded stop
}

uninstall_programs() {
	local unit
	local name
	local desktop
	local wants
	stop_units
	systemctl_loaded disable
	for name in mint-bill mint-bill-tme mint-bill-tce; do
		rm -f "${HOME}/.local/bin/${name}"
		rm -f "${HOME}/.local/libexec/${name}-db-backup"
		rm -f "${HOME}/.local/libexec/${name}"
	done
	for unit in \
		mint-bill.service mint-bill-db-backup.service mint-bill-db-backup.timer \
		mint-bill-tme.service mint-bill-tme-db-backup.service mint-bill-tme-db-backup.timer \
		mint-bill-tce.service mint-bill-tce-db-backup.service mint-bill-tce-db-backup.timer
	do
		rm -f "${HOME}/.config/systemd/user/${unit}"
		for wants in default.target.wants graphical-session.target.wants timers.target.wants; do
			rm -f "${HOME}/.config/systemd/user/${wants}/${unit}"
		done
	done
	for desktop in org.app.mint-bill.desktop org.app.mint-bill-tme.desktop org.app.mint-bill-tce.desktop; do
		rm -f "${HOME}/.local/share/applications/${desktop}"
		rm -f "$(data_home)/applications/${desktop}"
	done
	if [[ "$(id -u)" -eq 0 ]]; then
		rm -f /usr/bin/mint-bill /usr/bin/mint-bill-tme /usr/bin/mint-bill-tce
		rm -f /usr/share/applications/org.app.mint-bill.desktop \
			/usr/share/applications/org.app.mint-bill-tme.desktop \
			/usr/share/applications/org.app.mint-bill-tce.desktop
		if command -v rpm >/dev/null 2>&1; then
			rpm -q mint-bill >/dev/null 2>&1 && rpm -e mint-bill || true
			rpm -q mint-bill-tme >/dev/null 2>&1 && rpm -e mint-bill-tme || true
			rpm -q mint-bill-tce >/dev/null 2>&1 && rpm -e mint-bill-tce || true
		fi
	fi
	run_systemctl daemon-reload
	log "removed Mint-Bill programs and services"
}

remove_application_data() {
	local name
	for name in org.app.mint-bill org.app.mint-bill-tme org.app.mint-bill-tce; do
		rm -rf "$(data_home)/${name}"
	done
	for name in mint-bill mint-bill-tme mint-bill-tce; do
		rm -rf "${HOME}/${name}-backups"
	done
	if [[ -n "${MINT_BILL_BACKUP_DIR:-}" && "${MINT_BILL_BACKUP_DIR}" != "${HOME}" && "${MINT_BILL_BACKUP_DIR}" != "/" ]]; then
		rm -rf "${MINT_BILL_BACKUP_DIR}"
	fi
	if [[ "${MINT_BILL_NO_KEYRING:-}" != "1" ]] && command -v secret-tool >/dev/null 2>&1; then
		secret-tool clear xdg:schema org.app.mint-bill.password number 0 even true >/dev/null 2>&1 \
			|| true
	fi
	if [[ "$(id -u)" -eq 0 ]]; then
		rm -rf /usr/share/org.app.mint-bill /usr/share/org.app.mint-bill-tme /usr/share/org.app.mint-bill-tce
	fi
	log "removed Mint-Bill data and backups"
}

install_tree() {
	local tree="$1"
	local file
	local base
	local appid
	local dest
	local exec_line
	local share_dir
	local unit_dir="${HOME}/.config/systemd/user"

	mkdir -p "${HOME}/.local/bin" "${HOME}/.local/libexec" "${unit_dir}" "$(data_home)/applications"
	while IFS= read -r file; do
		[[ -n "${file}" ]] || continue
		base="$(basename "${file}")"
		cp -a "${file}" "${HOME}/.local/bin/${base}"
		chmod 755 "${HOME}/.local/bin/${base}"
	done < <(find "${tree}" -type f -path '*/bin/mint-bill*' -print)

	while IFS= read -r file; do
		[[ -n "${file}" ]] || continue
		base="$(basename "${file}")"
		cp -a "${file}" "${HOME}/.local/libexec/${base}"
		chmod 755 "${HOME}/.local/libexec/${base}"
	done < <(find "${tree}" -type f -path '*/libexec/*' -print)

	while IFS= read -r share_dir; do
		[[ -n "${share_dir}" ]] || continue
		appid="$(basename "${share_dir}")"
		dest="$(data_home)/${appid}"
		mkdir -p "${dest}"
		for file in "${share_dir}"/*; do
			[[ -e "${file}" ]] || continue
			base="$(basename "${file}")"
			case "${base}" in
				*.db.template)
					cp -a "${file}" "${dest}/${base}"
					;;
				*.db)
					log "skipped packaged database ${base}"
					;;
				*)
					cp -a "${file}" "${dest}/${base}"
					;;
			esac
		done
	done < <(find "${tree}" -type d -name 'org.app.mint-bill-*' -print)

	while IFS= read -r file; do
		[[ -n "${file}" ]] || continue
		base="$(basename "${file}")"
		cp -a "${file}" "${unit_dir}/${base}"
		if [[ "${base}" == mint-bill*.service && "${base}" != *-db-backup.service ]]; then
			sed -i 's/^After=graphical-session\.target$/After=graphical-session-pre.target/' \
				"${unit_dir}/${base}"
		fi
	done < <(find "${tree}" -type f \( -name 'mint-bill*.service' -o -name 'mint-bill*.timer' \) -print)

	while IFS= read -r file; do
		[[ -n "${file}" ]] || continue
		base="$(basename "${file}")"
		dest="$(data_home)/applications/${base}"
		cp -a "${file}" "${dest}"
		exec_line="$(sed -n 's/^Exec=//p' "${dest}" | head -n 1)"
		if [[ -n "${exec_line}" && "${exec_line}" != /* ]]; then
			sed -i "s|^Exec=.*|Exec=${HOME}/.local/bin/${exec_line}|" "${dest}"
		fi
	done < <(find "${tree}" -type f -name 'org.app.mint-bill*.desktop' -print)
}

install_rpms() {
	local rpm
	local stage
	local rpms=()
	if [[ -n "${MINT_BILL_PAYLOAD_DIR:-}" ]]; then
		install_tree "${MINT_BILL_PAYLOAD_DIR}"
		return 0
	fi
	shopt -s nullglob
	rpms=("${SCRIPT_DIR}"/mint-bill-*.rpm)
	shopt -u nullglob
	[[ "${#rpms[@]}" -gt 0 ]] || die "no Mint-Bill RPMs found in ${SCRIPT_DIR}"
	for rpm in "${rpms[@]}"; do
		log "unpacking $(basename "${rpm}")"
		stage="$(mktemp -d)"
		(
			cd "${stage}"
			rpm2cpio "${rpm}" | cpio -idm --quiet
		)
		install_tree "${stage}"
		rm -rf "${stage}"
	done
}

install_runtime_deps() {
	local deps=()
	shopt -s nullglob
	deps=("${SCRIPT_DIR}"/deps/*.rpm)
	shopt -u nullglob
	[[ "${#deps[@]}" -gt 0 ]] || return 0
	if [[ "$(id -u)" -ne 0 ]]; then
		log "runtime RPMs are in ${SCRIPT_DIR}/deps. Install them as root: dnf install ${SCRIPT_DIR}/deps/*.rpm"
		return 0
	fi
	command -v dnf >/dev/null 2>&1 || die "dnf is required to install ${SCRIPT_DIR}/deps"
	dnf -y install "${deps[@]}"
}

enable_units() {
	local tenant
	local unit
	[[ "${MINT_BILL_NO_SYSTEMD:-}" == "1" ]] && return 0
	command -v systemctl >/dev/null 2>&1 || return 0
	systemctl --user daemon-reload || {
		log "could not reload user systemd"
		return 0
	}
	for tenant in tme tce; do
		unit="${HOME}/.config/systemd/user/mint-bill-${tenant}.service"
		[[ -f "${unit}" ]] || continue
		systemctl --user enable "mint-bill-${tenant}.service" "mint-bill-${tenant}-db-backup.timer" \
			|| die "could not enable ${tenant} services"
		systemctl --user start "mint-bill-${tenant}.service" \
			|| die "could not start mint-bill-${tenant}"
		systemctl --user is-active --quiet "mint-bill-${tenant}.service" \
			|| die "mint-bill-${tenant} is not running"
	done
}

parse_args() {
	while [[ "$#" -gt 0 ]]; do
		case "$1" in
			--help|-h)
				HELP=1
				shift
				;;
			--install)
				set_action install
				shift
				;;
			--uninstall)
				set_action uninstall
				shift
				;;
			*)
				die "unknown argument: $1"
				;;
		esac
	done
}

main() {
	if [[ "$#" -eq 0 ]]; then
		usage
		return 0
	fi
	parse_args "$@"
	if [[ "${HELP}" -eq 1 ]]; then
		usage
		return 0
	fi
	if [[ -z "${ACTION}" ]]; then
		die "choose one of --install or --uninstall"
	fi
	if [[ "${ACTION}" == "uninstall" ]]; then
		uninstall_programs
		remove_application_data
		log "Mint-Bill is uninstalled for this user"
		return 0
	fi
	stop_units
	uninstall_programs
	install_runtime_deps
	install_rpms
	enable_units
	log "Mint-Bill is installed for this user"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
	main "$@"
fi

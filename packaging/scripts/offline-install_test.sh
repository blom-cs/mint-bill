#!/bin/bash
# Exercises the offline installer against a fake home and a fake RPM payload.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
INSTALL="${ROOT}/packaging/scripts/offline-install.sh"
export MINT_BILL_NO_KEYRING=1

fail() {
	printf 'FAIL: %s\n' "$*" >&2
	exit 1
}

grep -qi sqlcipher "${INSTALL}" && fail "installer still mentions sqlcipher"
grep -qi legacy "${INSTALL}" && fail "installer still mentions a legacy install"

make_payload() {
	local dir="$1"
	mkdir -p \
		"${dir}/usr/bin" \
		"${dir}/usr/libexec" \
		"${dir}/usr/lib/systemd/user" \
		"${dir}/usr/share/applications" \
		"${dir}/usr/share/org.app.mint-bill-tme" \
		"${dir}/usr/share/org.app.mint-bill-tce"
	printf '#!/bin/bash\necho new-tme\n' > "${dir}/usr/bin/mint-bill-tme"
	printf '#!/bin/bash\necho new-tce\n' > "${dir}/usr/bin/mint-bill-tce"
	printf '#!/bin/bash\necho backup\n' > "${dir}/usr/libexec/mint-bill-tme-db-backup"
	printf '#!/bin/bash\necho backup\n' > "${dir}/usr/libexec/mint-bill-tce-db-backup"
	chmod 755 "${dir}/usr/bin/"* "${dir}/usr/libexec/"*
	printf '[Unit]\nDescription=tme\nAfter=graphical-session.target\n[Service]\nExecStart=%%h/.local/bin/mint-bill-tme\n' \
		> "${dir}/usr/lib/systemd/user/mint-bill-tme.service"
	printf '[Unit]\nDescription=tce\nAfter=graphical-session.target\n[Service]\nExecStart=%%h/.local/bin/mint-bill-tce\n' \
		> "${dir}/usr/lib/systemd/user/mint-bill-tce.service"
	printf '[Unit]\nDescription=backup\n[Timer]\nOnCalendar=weekly\n' \
		> "${dir}/usr/lib/systemd/user/mint-bill-tme-db-backup.timer"
	printf '[Unit]\nDescription=backup\n[Timer]\nOnCalendar=weekly\n' \
		> "${dir}/usr/lib/systemd/user/mint-bill-tce-db-backup.timer"
	printf 'Exec=mint-bill-tme\n' > "${dir}/usr/share/applications/org.app.mint-bill-tme.desktop"
	printf 'Exec=mint-bill-tce\n' > "${dir}/usr/share/applications/org.app.mint-bill-tce.desktop"
	printf 'template\n' > "${dir}/usr/share/org.app.mint-bill-tme/mint-bill-tme.db.template"
	printf 'ui\n' > "${dir}/usr/share/org.app.mint-bill-tme/mint-bill.ui"
	printf 'packaged-tme\n' > "${dir}/usr/share/org.app.mint-bill-tme/mint-bill-tme.db"
	printf 'template\n' > "${dir}/usr/share/org.app.mint-bill-tce/mint-bill-tce.db.template"
	printf 'packaged-tce\n' > "${dir}/usr/share/org.app.mint-bill-tce/mint-bill-tce.db"
}

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
payload="${work}/payload"
make_payload "${payload}"

# A live database stays in place, including its sidecar. The packaged database is skipped.
home="${work}/home1"
mkdir -p "${home}/.local/bin" "${home}/.config/systemd/user" "${home}/.local/share/org.app.mint-bill-tce"
printf '#!/bin/bash\necho old\n' > "${home}/.local/bin/mint-bill"
printf '[Unit]\nDescription=old\n' > "${home}/.config/systemd/user/mint-bill.service"
plain="${home}/.local/share/org.app.mint-bill-tce/mint-bill-tce.db"
printf 'live-turso\n' > "${plain}"
printf 'sidecar\n' > "${plain}-log"
before="$(cksum "${plain}")"
HOME="${home}" \
XDG_DATA_HOME="${home}/.local/share" \
MINT_BILL_NO_SYSTEMD=1 \
MINT_BILL_PAYLOAD_DIR="${payload}" \
	bash "${INSTALL}" --install
[[ "$(cksum "${plain}")" == "${before}" ]] || fail "live database was rewritten"
[[ "$(cat "${plain}-log")" == "sidecar" ]] || fail "live database log was removed"
[[ "$(cat "${plain}")" != "packaged-tce" ]] || fail "packaged database replaced the live file"
[[ ! -e "${home}/.local/share/org.app.mint-bill-tme/mint-bill-tme.db" ]] \
	|| fail "packaged database was copied into an empty data directory"
[[ -f "${home}/.local/share/org.app.mint-bill-tme/mint-bill-tme.db.template" ]] || fail "template was not installed"
[[ ! -e "${home}/.local/bin/mint-bill" ]] || fail "old binary was not removed"
[[ ! -e "${home}/.config/systemd/user/mint-bill.service" ]] || fail "old service was not removed"
[[ -x "${home}/.local/bin/mint-bill-tme" ]] || fail "tme binary was not installed"
[[ -x "${home}/.local/bin/mint-bill-tce" ]] || fail "tce binary was not installed"
[[ -f "${home}/.config/systemd/user/mint-bill-tme.service" ]] || fail "new service was not installed"
grep -q "Exec=${home}/.local/bin/mint-bill-tme" \
	"${home}/.local/share/applications/org.app.mint-bill-tme.desktop" \
	|| fail "desktop Exec was not rewritten"

# Uninstall removes the programs, data, and backups, and leaves unrelated files.
home="${work}/home2"
mkdir -p \
	"${home}/.local/bin" \
	"${home}/.local/libexec" \
	"${home}/.config/systemd/user/default.target.wants" \
	"${home}/.config/systemd/user/timers.target.wants" \
	"${home}/.local/share/applications" \
	"${home}/.local/share/org.app.mint-bill-tme"
printf '#!/bin/bash\necho tme\n' > "${home}/.local/bin/mint-bill-tme"
printf '#!/bin/bash\necho backup\n' > "${home}/.local/libexec/mint-bill-tme-db-backup"
chmod 755 "${home}/.local/bin/mint-bill-tme" "${home}/.local/libexec/mint-bill-tme-db-backup"
printf '[Unit]\nDescription=tme\n' > "${home}/.config/systemd/user/mint-bill-tme.service"
ln -s ../mint-bill-tme.service "${home}/.config/systemd/user/default.target.wants/mint-bill-tme.service"
printf '[Timer]\nOnCalendar=weekly\n' > "${home}/.config/systemd/user/mint-bill-tme-db-backup.timer"
ln -s ../mint-bill-tme-db-backup.timer \
	"${home}/.config/systemd/user/timers.target.wants/mint-bill-tme-db-backup.timer"
printf 'Exec=%s\n' "${home}/.local/bin/mint-bill-tme" \
	> "${home}/.local/share/applications/org.app.mint-bill-tme.desktop"
plain="${home}/.local/share/org.app.mint-bill-tme/mint-bill-tme.db"
printf 'live-turso\n' > "${plain}"
printf 'keep-template\n' > "${home}/.local/share/org.app.mint-bill-tme/mint-bill-tme.db.template"
printf 'sidecar\n' > "${plain}-log"
mkdir -p "${home}/.local/share/other-app" "${home}/mint-bill-tme-backups"
printf 'keep\n' > "${home}/.local/share/other-app/keep"
printf 'backup\n' > "${home}/mint-bill-tme-backups/mint-bill-tme-1.db"
HOME="${home}" \
XDG_DATA_HOME="${home}/.local/share" \
MINT_BILL_NO_SYSTEMD=1 \
	bash "${INSTALL}" --uninstall
[[ ! -e "${home}/.local/bin/mint-bill-tme" ]] || fail "uninstall left the binary"
[[ ! -e "${home}/.local/libexec/mint-bill-tme-db-backup" ]] || fail "uninstall left the backup script"
[[ ! -e "${home}/.config/systemd/user/mint-bill-tme.service" ]] || fail "uninstall left the service"
[[ ! -e "${home}/.config/systemd/user/mint-bill-tme-db-backup.timer" ]] || fail "uninstall left the timer"
[[ ! -e "${home}/.config/systemd/user/default.target.wants/mint-bill-tme.service" ]] \
	|| fail "uninstall left the service enable link"
[[ ! -e "${home}/.config/systemd/user/timers.target.wants/mint-bill-tme-db-backup.timer" ]] \
	|| fail "uninstall left the timer enable link"
[[ ! -e "${home}/.local/share/applications/org.app.mint-bill-tme.desktop" ]] \
	|| fail "uninstall left the desktop file"
[[ ! -e "${home}/.local/share/org.app.mint-bill-tme" ]] || fail "uninstall left the data directory"
[[ ! -e "${home}/mint-bill-tme-backups" ]] || fail "uninstall left the backup directory"
[[ "$(cat "${home}/.local/share/other-app/keep")" == "keep" ]] || fail "uninstall removed unrelated data"

# Two actions are rejected before anything is removed.
home="${work}/home3"
mkdir -p "${home}/.local/bin"
printf '#!/bin/bash\necho stay\n' > "${home}/.local/bin/mint-bill-tme"
chmod 755 "${home}/.local/bin/mint-bill-tme"
set +e
HOME="${home}" MINT_BILL_NO_SYSTEMD=1 \
	bash "${INSTALL}" --uninstall --install >"${work}/mix.out" 2>"${work}/mix.err"
status="$?"
set -e
[[ "${status}" -ne 0 ]] || fail "--uninstall accepted --install"
[[ -x "${home}/.local/bin/mint-bill-tme" ]] || fail "rejected uninstall removed the binary"

# The old migration flag is an unknown argument and changes nothing.
home="${work}/home4"
mkdir -p "${home}/.local/bin"
printf '#!/bin/bash\necho stay\n' > "${home}/.local/bin/mint-bill-tme"
chmod 755 "${home}/.local/bin/mint-bill-tme"
set +e
HOME="${home}" MINT_BILL_NO_SYSTEMD=1 \
	bash "${INSTALL}" --legacy-to tme >"${work}/legacy.out" 2>"${work}/legacy.err"
status="$?"
set -e
[[ "${status}" -ne 0 ]] || fail "--legacy-to was accepted"
grep -q 'unknown argument' "${work}/legacy.err" || fail "--legacy-to was not reported as unknown"
[[ -x "${home}/.local/bin/mint-bill-tme" ]] || fail "--legacy-to removed the binary"
[[ ! -e "${home}/.local/bin/mint-bill-tce" ]] || fail "--legacy-to installed a program"

# No arguments prints help and does not install or remove programs.
home="${work}/home5"
mkdir -p "${home}/.local/bin"
printf '#!/bin/bash\necho stay\n' > "${home}/.local/bin/mint-bill-tme"
chmod 755 "${home}/.local/bin/mint-bill-tme"
HOME="${home}" MINT_BILL_NO_SYSTEMD=1 \
	bash "${INSTALL}" >"${work}/help.out" 2>"${work}/help.err"
[[ ! -s "${work}/help.err" ]] || fail "help wrote an error"
grep -q -- '--install' "${work}/help.out" || fail "help omitted --install"
grep -q -- '--uninstall' "${work}/help.out" || fail "help omitted --uninstall"
grep -qi sqlcipher "${work}/help.out" && fail "help mentions sqlcipher"
grep -qi legacy "${work}/help.out" && fail "help mentions a legacy install"
[[ -x "${home}/.local/bin/mint-bill-tme" ]] || fail "help removed the binary"
[[ ! -e "${home}/.local/bin/mint-bill-tce" ]] || fail "help installed a program"
HOME="${home}" MINT_BILL_NO_SYSTEMD=1 \
	bash "${INSTALL}" --help >"${work}/help2.out"
grep -q -- '--install' "${work}/help2.out" || fail "--help omitted --install"
grep -qi legacy "${work}/help2.out" && fail "--help mentions a legacy install"

# A fresh install does not stop or disable units that are not loaded, and it starts both programs.
home="${work}/home6"
fakectl="${work}/bin"
mkdir -p "${fakectl}" "${home}"
cat > "${fakectl}/systemctl" <<'EOF'
#!/bin/bash
printf '%s\n' "$*" >> "${MINT_BILL_SYSTEMCTL_LOG}"
if [[ "$1" == "--user" && "$2" == "show" ]]; then
	printf 'not-found\n'
	exit 0
fi
if [[ "$1" == "--user" && ( "$2" == "daemon-reload" || "$2" == "enable" || "$2" == "start" || "$2" == "is-active" ) ]]; then
	exit 0
fi
printf 'unexpected systemctl %s\n' "$*" >&2
exit 1
EOF
chmod 755 "${fakectl}/systemctl"
: > "${work}/systemctl.log"
PATH="${fakectl}:${PATH}" \
HOME="${home}" \
XDG_DATA_HOME="${home}/.local/share" \
MINT_BILL_SYSTEMCTL_LOG="${work}/systemctl.log" \
MINT_BILL_PAYLOAD_DIR="${payload}" \
	bash "${INSTALL}" --install >"${work}/fresh.out" 2>"${work}/fresh.err"
grep -q 'Failed to stop' "${work}/fresh.err" && fail "fresh install reported a missing unit stop"
grep -q 'does not exist' "${work}/fresh.err" && fail "fresh install reported a missing unit disable"
grep -q 'systemctl --user stop' "${work}/fresh.err" && fail "fresh install logged a stop failure"
grep -Eq '(^| )stop( |$)' "${work}/systemctl.log" && fail "fresh install stopped a unit that is not loaded"
grep -Eq '(^| )disable( |$)' "${work}/systemctl.log" && fail "fresh install disabled a unit that is not loaded"
grep -q -- '--user start mint-bill-tme.service' "${work}/systemctl.log" \
	|| fail "fresh install did not start mint-bill-tme"
grep -q -- '--user start mint-bill-tce.service' "${work}/systemctl.log" \
	|| fail "fresh install did not start mint-bill-tce"
[[ -x "${home}/.local/bin/mint-bill-tme" ]] || fail "fresh install did not install tme"
[[ ! -e "${home}/.local/share/org.app.mint-bill-tme/mint-bill-tme.db" ]] \
	|| fail "fresh install copied the packaged database"
[[ -f "${home}/.local/share/org.app.mint-bill-tce/mint-bill-tce.db.template" ]] \
	|| fail "fresh install skipped the template"
grep -q 'After=graphical-session-pre.target' \
	"${home}/.config/systemd/user/mint-bill-tme.service" \
	|| fail "tme service still waits for the graphical session"

printf 'OK\n'

#!/bin/bash
################################################################################
# Build offline Fedora RPMs for Mint-Bill tenants (tme and/or tce).
#
# Usage:
#   ./scripts/package-rpm.sh           # both tenants
#   ./scripts/package-rpm.sh all
#   ./scripts/package-rpm.sh tme
#   ./scripts/package-rpm.sh tce
################################################################################
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REQUEST="${1:-all}"
read_build_setting() {
	sed -n "s/^${1}=//p" "${ROOT}/scripts/build.sh" | head -n 1
}

VERSION="${VERSION:-$(read_build_setting PROJECT_VERSION_NUM)}"
VERSION_PREFIX="${PROJECT_VERSION_PREFIX:-$(read_build_setting PROJECT_VERSION_PREFIX)}"
if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
	echo "ERROR: PROJECT_VERSION_NUM in scripts/build.sh must be MAJOR.MINOR.PATCH (got '${VERSION}')" >&2
	exit 1
fi
OUT="${ROOT}/build/rpm"
TOOL_DIR="${ROOT}/../mint-bill-tool"
DIST="$(rpm --eval '%{dist}' 2>/dev/null || echo .fc40)"
ARCH="$(uname -m)"

if ! command -v rpmbuild >/dev/null 2>&1; then
	echo "ERROR: rpmbuild not found. Install rpm-build rpmdevtools." >&2
	exit 1
fi

tenants=()
case "${REQUEST}" in
	all|both|"")
		tenants=(tme tce)
		;;
	tme|tce)
		tenants=("${REQUEST}")
		;;
	*)
		echo "ERROR: tenant must be tme, tce, or all (got '${REQUEST}')" >&2
		exit 1
		;;
esac

mkdir -p "${OUT}"
if [[ -d "${TOOL_DIR}" ]]; then
	mkdir -p "${TOOL_DIR}"
fi

build_one() {
	local tenant="$1"
	local name="mint-bill-${tenant}"
	local top="${ROOT}/build/rpmbuild-${tenant}"

	rm -rf "${top}"
	mkdir -p "${top}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

	echo "INFO: creating source tarball ${name}-${VERSION}.tar.gz"
	tar -C "${ROOT}" \
		--exclude='./build' \
		--exclude='./.git' \
		--exclude='./test/coverage' \
		--exclude='./test/objs' \
		--exclude='./docs/doxydoc' \
		--exclude='*.swp' \
		--transform "s|^\.|${name}-${VERSION}|" \
		-czf "${top}/SOURCES/${name}-${VERSION}.tar.gz" \
		.

	cp "${ROOT}/packaging/rpm/mint-bill.spec" "${top}/SPECS/${name}.spec"

	echo "INFO: rpmbuild -bb tenant=${tenant}"
	rpmbuild \
		--define "_topdir ${top}" \
		--define "tenant ${tenant}" \
		--define "mint_bill_version ${VERSION}" \
		-bb "${top}/SPECS/${name}.spec"

	local rpm_file
	rpm_file="$(find "${top}/RPMS" -name "${name}-${VERSION}-*.${ARCH}.rpm" ! -name '*debug*' | head -n 1)"
	if [[ -z "${rpm_file}" ]]; then
		echo "ERROR: rpmbuild finished but no RPM was found under ${top}/RPMS" >&2
		find "${top}/RPMS" -type f >&2 || true
		exit 1
	fi

	cp -a "${rpm_file}" "${OUT}/"
	echo "INFO: wrote ${OUT}/$(basename "${rpm_file}")"
	if [[ -d "${TOOL_DIR}" ]]; then
		cp -a "${rpm_file}" "${TOOL_DIR}/"
		echo "INFO: copied $(basename "${rpm_file}") into ${TOOL_DIR}"
	fi
}

for tenant in "${tenants[@]}"; do
	build_one "${tenant}"
done

BUNDLE_NAME="mint-bill${VERSION_PREFIX}${VERSION}"
BUNDLE="${OUT}/${BUNDLE_NAME}"
rm -rf "${BUNDLE}"
mkdir -p "${BUNDLE}/deps"
shopt -s nullglob
app_rpms=("${OUT}"/mint-bill-*.rpm)
shopt -u nullglob
if [[ "${#app_rpms[@]}" -eq 0 ]]; then
	echo "ERROR: no application RPMs were copied to ${OUT}" >&2
	exit 1
fi
cp -a "${app_rpms[@]}" "${BUNDLE}/"
cp -a "${ROOT}/packaging/scripts/offline-install.sh" "${BUNDLE}/install.sh"
chmod 755 "${BUNDLE}/install.sh"

if command -v dnf >/dev/null 2>&1; then
	echo "INFO: fetching local (offline) runtime RPMs into ${BUNDLE}/deps"
	if dnf download --destdir "${BUNDLE}/deps" \
		gtkmm4.0 glibmm2.68 pangomm2.48 cairomm1.16 libsigc++30 \
		poppler-cpp poppler; then
		rm -f "${BUNDLE}/deps"/*.i686.rpm
	else
		echo "WARNING: could not download runtime RPMs; ${BUNDLE}/deps may be incomplete."
	fi
fi

ARCHIVE="${OUT}/${BUNDLE_NAME}.tar.gz"
tar -C "${OUT}" -czf "${ARCHIVE}" "${BUNDLE_NAME}"
if [[ -d "${TOOL_DIR}" ]]; then
	cp -a "${BUNDLE}/." "${TOOL_DIR}/"
	echo "INFO: copied the bundle into ${TOOL_DIR}"
fi

echo
echo "Bundle: ${BUNDLE}"
echo "Archive: ${ARCHIVE}"
echo
echo "On the office PC (no internet), unpack the archive and run:"
echo "  ./install.sh"
echo "That prints the usage help. To install:"
echo "  ./install.sh --install"
echo "A live Turso database is left in place."
echo "To remove the programs, services, data, and backups:"
echo "  ./install.sh --uninstall"

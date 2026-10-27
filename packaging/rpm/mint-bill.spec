# Offline RPM for Mint-Bill. Live customer data is NOT in this package.
#
#   rpmbuild --define "_topdir <top>" --define "tenant tme" -bb mint-bill.spec
#
# The live Turso database lives in:
#   $XDG_DATA_HOME/org.app.mint-bill-<tenant>/<name>.db
# and is created on first launch from the packaged template. rpm -U replaces
# only files under /usr, so invoices, clients, and statements survive upgrades.

%global tenant %{?tenant}%{!?tenant:tme}
%global appid org.app.mint-bill-%{tenant}

Name:           mint-bill-%{tenant}
Version:        %{mint_bill_version}
Release:        1%{?dist}
Summary:        Offline invoicing desktop application (%{tenant})
License:        Apache-2.0
URL:            https://github.com/blom-cs/mint-bill
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.25
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  gtkmm4.0-devel
BuildRequires:  gtk4-devel
BuildRequires:  poppler-cpp-devel
BuildRequires:  libcurl-devel
BuildRequires:  cairomm1.16-devel
BuildRequires:  systemd-rpm-macros

Requires:       gtkmm4.0
Requires:       gtk4
Requires:       poppler-cpp
Requires:       libcurl
Requires:       cairomm1.16

%description
Mint-Bill is a desktop invoicing and statement system. This package is the
%{tenant} build, intended for fully offline Fedora machines: install from a
local RPM (or a USB directory of RPMs). No network is required at install
time. Runtime network is only needed if you send invoices by email.

This RPM is a payload. The supported install unpacks it into the user's
~/.local tree (binary, UI, CSS, email templates, DB template). The live
Turso database is ~/.local/share/%{appid}/%{name}.db and is never
replaced on upgrade.

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S . -B _build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DCMAKE_INSTALL_LIBDIR=%{_libdir} \
  -DAPP_NAME=%{name} \
  -DAPP_ID=%{appid} \
  -DMINT_BILL_VERSION=%{mint_bill_version} \
  -DAPP_DISPLAY_NAME="Mint Bill $(echo %{tenant} | tr '[:lower:]' '[:upper:]')" \
  -DBUILD_PROJECT=ON \
  -DINSTALL_APPLICATION=ON
cmake --build _build --parallel %{?_smp_mflags}

%install
rm -rf %{buildroot}
DESTDIR=%{buildroot} cmake --install _build

%post
%systemd_user_post %{name}.service %{name}-db-backup.timer

%preun
%systemd_user_preun %{name}.service %{name}-db-backup.timer

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_libexecdir}/%{name}-db-backup
%{_datadir}/%{appid}/
%{_userunitdir}/%{name}.service
%{_userunitdir}/%{name}-db-backup.service
%{_userunitdir}/%{name}-db-backup.timer
%{_datadir}/applications/%{appid}.desktop

%changelog
* Sun Sep 13 2026 Dawid J. Blom <dawidjblom@gmail.com> - 1.0.0-1
- Replace Flatpak packaging with an offline Fedora RPM
- Keep the live database outside the package so upgrades do not wipe data

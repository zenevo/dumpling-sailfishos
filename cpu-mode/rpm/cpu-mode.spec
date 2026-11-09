Name:       cpu-mode
Summary:    CPU/GPU mode switcher for OnePlus 5T (dumpling)
Version:    1.0.0
Release:    1
License:    MIT
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  qt5-qtdeclarative-devel

%description
A small Sailfish OS application to switch between CPU power modes
(powersave / balanced / performance), with a live CPU frequency chart.

%prep
%setup -q

%build
cd %{name}-%{version}
qmake
make %{?_smp_mflags}

%install
cd %{name}-%{version}
make install INSTALL_ROOT=%{buildroot}
install -D -m 755 data/cpu-mode-apply.sh %{buildroot}%{_bindir}/cpu-mode-apply
install -D -m 644 data/cpu-mode.desktop %{buildroot}%{_datadir}/applications/cpu-mode.desktop
install -D -m 644 data/cpu-mode.path %{buildroot}/usr/lib/systemd/system/cpu-mode.path
install -D -m 644 data/cpu-mode.service %{buildroot}/usr/lib/systemd/system/cpu-mode.service
for s in 86 108 128 172 256 512; do
  install -D -m 644 icons/cpu-mode-$s.png %{buildroot}%{_datadir}/icons/hicolor/${s}x${s}/apps/cpu-mode.png
done

%post
mkdir -p /home/defaultuser/.config/cpu-mode
chown -R defaultuser:defaultuser /home/defaultuser/.config/cpu-mode 2>/dev/null || true
systemctl daemon-reload >/dev/null 2>&1 || true
systemctl enable cpu-mode.path >/dev/null 2>&1 || true
systemctl start cpu-mode.path >/dev/null 2>&1 || true

%postun
systemctl daemon-reload >/dev/null 2>&1 || true

%files
%{_bindir}/cpu-mode
%{_bindir}/cpu-mode-apply
%{_datadir}/cpu-mode
%{_datadir}/applications/cpu-mode.desktop
%{_datadir}/icons/hicolor/*/apps/cpu-mode.png
/usr/lib/systemd/system/cpu-mode.path
/usr/lib/systemd/system/cpu-mode.service

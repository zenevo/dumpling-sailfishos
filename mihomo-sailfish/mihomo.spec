%define debug_package %{nil}
%define __strip /bin/true

Name:       mihomo
Version:    1.19.33
Release:    1
Summary:    Mihomo (Clash.Meta) proxy core
License:    GPL-3.0
URL:        https://github.com/MetaCubeX/mihomo
Source0:    mihomo
Source1:    mihomo.service
Source2:    config.yaml
BuildArch:  aarch64

Requires:   systemd
Requires:   iproute
Requires:   iptables

%description
Mihomo (formerly Clash.Meta) is a rule-based proxy core.
This package ships the statically linked aarch64 binary, a systemd unit and
a minimal configuration for Sailfish OS.

%prep

%build

%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_sysconfdir}/mihomo
mkdir -p %{buildroot}%{_unitdir}
mkdir -p %{buildroot}%{_licensedir}/%{name}

install -m 0755 %{SOURCE0} %{buildroot}%{_bindir}/mihomo
install -m 0644 %{SOURCE1} %{buildroot}%{_unitdir}/mihomo.service
install -m 0644 %{SOURCE2} %{buildroot}%{_sysconfdir}/mihomo/config.yaml
echo "GPL-3.0" > %{buildroot}%{_licensedir}/%{name}/LICENSE

%files
%{_bindir}/mihomo
%dir %{_sysconfdir}/mihomo
%config(noreplace) %{_sysconfdir}/mihomo/config.yaml
%{_unitdir}/mihomo.service
%{_licensedir}/%{name}/LICENSE

%post
systemctl daemon-reload >/dev/null 2>&1 || :

%preun
if [ "$1" -eq 0 ]; then
    systemctl stop mihomo.service >/dev/null 2>&1 || :
fi
systemctl daemon-reload >/dev/null 2>&1 || :

%changelog
* Sat Oct 31 2026 dumpling port <port@localhost> - 1.19.33-1
- Initial packaging for Sailfish OS (aarch64)

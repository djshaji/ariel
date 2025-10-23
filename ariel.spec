Name:           ariel
Version:        1.0.0
Release:        1%{?dist}
Summary:        Cross-platform LV2 plugin host with GTK4 and JACK audio

License:        MIT
URL:            https://github.com/djshaji/ariel
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  meson >= 0.56.0
BuildRequires:  gcc
BuildRequires:  pkgconfig(gtk4) >= 4.0
BuildRequires:  pkgconfig(lilv-0)
BuildRequires:  desktop-file-utils

Requires:       gtk4 >= 4.0
Requires:       lilv

%description
Ariel is a modern cross-platform LV2 plugin host built with GTK4 and lilv
for real-time audio processing. The application provides an intuitive
interface for loading and managing LV2 plugins with JACK audio backend
support, featuring a plugin browser, mixer controls, and transport controls.

%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install
mkdir -p %{buildroot}%{_datadir}/ariel/themes
cp -r themes/* %{buildroot}%{_datadir}/ariel/themes/

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/*.desktop

%files
%license LICENSE
%doc README.md
%{_bindir}/ariel
%{_datadir}/applications/*.desktop
%{_datadir}/ariel/ariel-theme.css
%{_datadir}/ariel/themes/
%{_datadir}/metainfo/ariel.appdata.xml

%changelog
* %(date "+%%a %%b %%d %%Y") %{getenv:USER} <user@example.com> - 1.0.0-1
- Initial RPM package for Ariel LV2 host

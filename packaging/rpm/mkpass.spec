Name:           mkpass
Version:        %{version}
Release:        1%{?dist}
Summary:        Secure deterministic password and passphrase generator using Argon2
License:        GPL-3.0-or-later
URL:            https://github.com/kgorelov/mkpass
Source0:        mkpass-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  sqlite-devel
BuildRequires:  (qt6-qtbase-devel or qt5-qtbase-devel)

%description
mkpass is a deterministic, stateless password and passphrase generator
utilizing Argon2, HMAC-SHA512, and Diceware algorithms. This package
provides the command-line utility.

%package gui
Summary:        Qt graphical interface for mkpass password generator
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       (qt6-qtbase-gui or qt5-qtbase-gui)

%description gui
Graphical user interface (Qt) for the mkpass password generator.

%prep
%setup -q

%build
%if 0%{?cmake_build:1}
  %cmake -DWITH_GUI=ON -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
  %cmake_build
%else
  mkdir -p build
  cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=%{_prefix} -DWITH_GUI=ON -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
  make %{?_smp_mflags}
%endif

%install
%if 0%{?cmake_install:1}
  %cmake_install
%else
  cd build
  make install DESTDIR=%{buildroot}
%endif

%files
%license debian/copyright
%{_bindir}/mkpass
%{_mandir}/man1/mkpass.1*

%files gui
%{_bindir}/mkpass-gui
%{_datadir}/applications/mkpass.desktop
%{_datadir}/icons/hicolor/256x256/apps/mkpass.png
%{_mandir}/man1/mkpass-gui.1*

%changelog
* Sun Jun 21 2026 Kirill Gorelov <kgorelov@gmail.com> - 0.1.0-1
- Initial release package for Fedora and enterprise Linux.

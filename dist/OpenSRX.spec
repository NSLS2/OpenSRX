# Stop generation of debug rpms
%global debug_package %{nil}

Name:           OpenSRX
Version:        0.1.2
Release:        1%{?dist}
Summary:        C++ library for interfacing with Keyence SR-X barcode readers

License:        BSD-3-Clause
URL:            https://github.com/jwlodek/OpenSRX
Source0:        https://github.com/jwlodek/OpenSRX/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  cmake-rpm-macros
BuildRequires:  gcc-c++
BuildRequires:  make

BuildArch:      x86_64

%description
OpenSRX is a C++ library for communicating with Keyence SR-X series barcode
readers over serial or Ethernet connections. It provides type-safe parameter
get/set, image readback, and full scanner control.

%package devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains the header files and unversioned shared library symlink
needed for developing applications that use %{name}.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_INSTALL_LIBDIR=%{_libdir}
%cmake_build

%install
%cmake_install

%files
%{_libdir}/libOpenSRX.so.%{version}
%{_libdir}/libOpenSRX.so.0

%files devel
%{_libdir}/libOpenSRX.so
%{_includedir}/OpenSRX/*

%changelog
* Wed Jun 17 2026 Wlodek, Jakub <jwlodek.dev@gmail.com> - 0.1.2-1
- Refactor inheritance for communication interface classes, to be able to avoid leaking asio headers in the public API.
- Refactor logging code to avoid leaking spdlog/fineftp headers as dependency in public API.

* Wed May 13 2026 Wlodek, Jakub <jwlodek.dev@gmail.com> - 0.1.1-1
- Always build fineftp dependency as a static library.

* Wed May 13 2026 Wlodek, Jakub <jwlodek.dev@gmail.com> - 0.1.0-1
- Initial RPM release of the OpenSRX library.

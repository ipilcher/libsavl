%define so_ver	0.7
%define subver	0

Name:		libsavl
Summary:	Simple AVL tree library for C programs
Version:	%{so_ver}.%{subver}
Release:	1%{?dist}
License:	GPLv2+
Source0:	https://github.com/ipilcher/libsavl/archive/v%{version}/%{name}-%{version}.tar.gz
BuildRequires:	gcc doxygen

%description
A simple, lightweight, low-level AVL tree implementation for C programs.

%package devel
Summary:	Development files for libsavl
%description devel
The files required to build programs that use libsavl.

%package doc
Summary:	API documentation for libsavl
%description doc
API documentation for libsavl, in HTML format.

%prep
%autosetup -n %{name}-%{version}

%build
# Build the library
gcc %optflags -std=gnu99 -Wall -Wextra -Wcast-align -shared -fPIC \
	-Wl,-soname,%{name}.so.%{so_ver} -o %{name}.so.%{version} savl.c
# Build the API docs
doxygen Doxyfile

%install
# Main package
%__mkdir_p %{buildroot}%{_libdir}
%__cp %{name}.so.%{version} %{buildroot}%{_libdir}
%__ln_s %{name}.so.%{version} %{buildroot}%{_libdir}/%{name}.so.%{so_ver}
# devel
%__mkdir_p %{buildroot}%{_includedir}
%__cp savl.h %{buildroot}%{_includedir}
%__ln_s %{name}.so.%{version} %{buildroot}%{_libdir}/%{name}.so
# API docs
%__mkdir_p %{buildroot}%{_docdir}/%{name}
%__cp_r docs/html %{buildroot}%{_docdir}/%{name}

%files
%attr(0755,root,root) %{_libdir}/%{name}.so.%{version}
%{_libdir}/%{name}.so.%{so_ver}

%files devel
%attr(0644,root,root) %{_includedir}/savl.h
%{_libdir}/%{name}.so

%changelog
* Tue Feb 22 2022 Ian Pilcher <arequipeno@gmail.com> - 0.7.1-1
- Add doc subpackage

* Mon Feb 21 2022 Ian Pilcher <arequipeno@gmail.com> - 0.7.0-1
- Initial SPEC file

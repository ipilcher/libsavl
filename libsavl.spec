%global __os_install_post %{nil}

%define git_ver		%(git describe --tags --abbrev=0 | tr -d v)
%define git_dir		%(pwd)

%define so_ver		%{git_ver}
%define dev_name	savl

Name:		lib%{dev_name}
Summary:	Simple AVL tree library
Version:	%{so_ver}
Release:	1%{?dist}
License:	GPLv2+
Source0:	https://github.com/ipilcher/%{name}/archive/refs/tags/%{version}.tar.gz
BuildRequires:	git-core gcc

%description
A simple low-level AVL tree library for C programs.

%package devel
Summary:	Development files for libsavl

%description devel
The files required to build programs that use libsavl.

%prep

%build
cd %{git_dir}
git checkout v%{git_ver}
gcc -g -O0 -Wall -Wextra -shared -fPIC -Wl,-soname,%{name}.so.%{so_ver} \
	-o %{name}.so.%{so_ver} savl.c
git checkout main

%install
cd %{git_dir}
rm -rf %{buildroot}
mkdir -p mkdir -p %{buildroot}%{_libdir}
cp %{name}.so.%{so_ver} %{buildroot}%{_libdir}/
mkdir -p %{buildroot}%{_includedir}
cp %{dev_name}.h %{buildroot}%{_includedir}/
ln -s %{name}.so.%{so_ver} %{buildroot}%{_libdir}/%{name}.so

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%attr(0755,root,root) %{_libdir}/%{name}.so.%{so_ver}

%files devel
%{_libdir}/%{name}.so
%attr(0644,root,root) %{_includedir}/%{dev_name}.h

%changelog
* Thu Feb  3 2022 Ian Pilcher <arequipeno@gmail.com> - ?.?-1
- Auto-detect latest git tag
- Move git checkout to %build section
- Don't leave repo in detached HEAD state

* Thu Feb  3 2022 Ian Pilcher <arequipeno@gmail.com> - 0.3-0
- 0.3
- Disable optimization & binary stripping

* Thu Feb  3 2022 Ian Pilcher <arequipeno@gmail.com> - 0.2-0
- 0.2
- Fix savl_free() SEGFAULT on empty tree

* Tue Feb  1 2022 Ian Pilcher <arequipeno@gmail.com> - 0.1-0
- 0.1
- Change ldconfig path to /sbin/ to make DNF happy

* Thu Dec  9 2021 Ian Pilcher <arequipeno@gmail.com> - 0.0-0
- Initial SPEC

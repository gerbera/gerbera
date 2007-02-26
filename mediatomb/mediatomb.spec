%define name mediatomb 
%define version 0.9.0
%define release pre

Version: %{version}
Summary: %{name} - UPnP AV Mediaserver 
Name: %{name}
Release: %{release}
License: GPL
Group: Applications/Multimedia
Source: %{name}-%{version}.tar.gz
Buildroot: %{_tmppath}/%{name}-%{version}-buildroot 
BuildRequires: sqlite-devel => 3
BuildRequires: file
Packager: Sergey Bostandzhyan <jin@mediatomb.cc> 

%description
MediaTomb - UPnP AV Mediaserver for Linux.

%prep 

%setup -q

%build

./configure --prefix=$RPM_BUILD_ROOT/%{_prefix}

make

%install
rm -rf $RPM_BUILD_ROOT

make install
install -D -m0755 scripts/mediatomb-service %{buildroot}%{_initrddir}/mediatombd
%clean
rm -rf $RPM_BUILD_ROOT

%post
chkconfig --add mediatombd

%preun
chkconfig --del mediatombd

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL doc/doxygen.conf
%{_bindir}/mediatomb
%{_datadir}/%{name}/
%{_initrddir}/mediatombd

%changelog
* Mon Feb 26 2007 Sergey Bostandzhyan <jin@mediatomb.cc>
- Removed some files that were no longer needed.
* Wed Sep  7 2005 Sergey Bostandzhyan <jin@mediatomb.cc>
- Removed some buildrequires, our configure script should handle different
  scenarios itself.
* Wed Jun 15 2005 Sergey Bostandzhyan <jin@mediatomb.cc>
- Added init.d script + chkconfig
* Thu Apr 14 2005 Sergey Bostandzhyan <jin@mediatomb.cc>
- Initial release


%define name mediatomb   
%define version 0.8.1
%define release 1

Version: %{version}
Summary: %{name} - UPnP AV Mediaserver 
Name: %{name}
Release: %{release}
License: GPL
Group: Applications/Multimedia
Source: %{name}-%{version}.tar.gz
Buildroot: %{_tmppath}/%{name}-%{version}-buildroot 
BuildRequires: sqlite-devel => 3
BuildRequires: libupnp-devel file
Packager: Sergey Bostandzhyan <jin@deadlock.dhs.org> 

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

sed "s,__DEFAULT_SOURCE__,%{_datadir}/%{name}," \
    < scripts/tomb-install-dist.py > scripts/tomb-install

chmod +x scripts/tomb-install


install scripts/tomb-install $RPM_BUILD_ROOT/%{_bindir}/tomb-install
install -D scripts/mediatomb-service $RPM_BUILD_ROOT/%{_initrddir}/mediatomb

%clean
rm -rf $RPM_BUILD_ROOT

%post
chkconfig --add mediatomb

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL doc/doxygen.conf TODO FAQ
%doc doc/scripting-dev.txt doc/scripting-intro.txt doc/mysql.txt
%{_bindir}/mediatomb
%{_bindir}/tomb-install
%{_datadir}/%{name}/
%{_initrddir}/mediatomb

%changelog
* Wed Sep  7 2005 Sergey Bostandzhyan <jin@deadlock.dhs.org>
- Removed some buildrequires, our configure script should handle different
  scenarios itself.
* Wed Jun 15 2005 Sergey Bostandzhyan <jin@deadlock.dhs.org>
- Added init.d script + chkconfig
* Thu Apr 14 2005 Sergey Bostandzhyan <jin@deadlock.dhs.org>
- Initial release


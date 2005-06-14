%define name mediatomb   
%define version 0.8.0
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
BuildRequires: libjs-devel 
BuildRequires: id3lib => 3.8.3
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
chkconfig --add mediatomb

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL doc/doxygen.conf 
%doc scripting-dev.txt scripting-intro.txt
%{_bindir}/mediatomb
%{_bindir}/tomb-install
%{_datadir}/%{name}/
%{_initrddir}/mediatomb
%changelog                      
* Thu Apr 14 2005 Sergey Bostandzhyan <jin@deadlock.dhs.org>
- Initial release


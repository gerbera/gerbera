%define name mediatomb   
%define version 0.7.1
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

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL doc/doxygen.conf 
%{_bindir}/mediatomb
%{_bindir}/tomb-install
%{_datadir}/%{name}/

%changelog                      
* Thu Apr 14 2005 Sergey Bostandzhyan <jin@deadlock.dhs.org>
- Initial release


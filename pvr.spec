Name:		pvr
Version:	0.0.1
Release:	1%{?dist}%{?extra_release}
Summary:	Portable Virtual Machine Repository utility
Source:		http://www.migsoft.net/projects/pvr/pvr-%{version}.tar.gz

Group:		Applications/Emulators
License:	GPLv2+
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root

Requires:	libxml2
Requires:	libvirt

%description
Portable Virtual Machine Repository, or PVR for short, is the utility for handling
the portable virtual machines on the external hard-drives/flash-drives. These media
have to contain the pvr.xml description file in the root directory of underlaying
file system.

%prep
%setup -q -n pvr-%{version}

%build
%configure
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc COPYING README
%{_bindir}/pvr

%changelog
* Wed Apr 25 2012 Michal Novotny <mignov@gmail.com> - pvr-0.0.1
- Initial release

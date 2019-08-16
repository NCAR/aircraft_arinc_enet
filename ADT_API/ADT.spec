Summary: Alta API library
Name: arinc_enet
Version: 3.3.0
Release: 0
Group: Applications/Engineering
Source: ADT_API-3.3.0.tar.bz2
License: none
#URL: http://svn.openstreetmap.org/applications/utils/export/osm2pgsql 
Distribution: RHEL 7.6 Linux
#Requires: geos proj postgresql libxml2 bzip2 
#BuildRequires: geos proj-devel postgresql-devel libxml2-devel bzip2-devel gcc-c++ 
Buildroot: %{_tmppath}/%{name}-root

%description

%prep
%setup -n ADT_API

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/opt/local/lib
mkdir -p $RPM_BUILD_ROOT/opt/local/include
cp ADT_L?.h $RPM_BUILD_ROOT/opt/local/include
cp *so* $RPM_BUILD_ROOT/opt/local/lib

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(0755,root,root) /opt/local/lib/libADT_L1_Linux_x86_64.so
%attr(0644,root,root) /opt/local/include/ADT_L0.h
%attr(0644,root,root) /opt/local/include/ADT_L1.h

%changelog
* Tue Aug 13 2019 <cjw@ucar.edu> 3.3.0-0
- created initial package 

Summary: Alta API library
Name: arinc_enet
Version: 3.3.1
Release: 1
Group: Applications/Engineering
Source: ADT_API-3.3.1.tar.bz2
License: none
Distribution: RHEL 7.6 Linux
Buildroot: %{_tmppath}/%{name}-root

%description

%prep
%setup -n ADT_API

%build


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/opt/local/lib
mkdir -p $RPM_BUILD_ROOT/opt/local/include/Alta
cp ADT_L?.h $RPM_BUILD_ROOT/opt/local/include/Alta
cp libADT_L?_Linux_x86_64_v3310.a $RPM_BUILD_ROOT/opt/local/lib

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(0644,root,root) /opt/local/lib/libADT_L0_Linux_x86_64_v3310.a
%attr(0644,root,root) /opt/local/lib/libADT_L1_Linux_x86_64_v3310.a
%attr(0644,root,root) /opt/local/include/Alta/ADT_L0.h
%attr(0644,root,root) /opt/local/include/Alta/ADT_L1.h

%changelog
* Thu Nov 14 2019 <cjw@ucar.edu> 3.3.1-1
- Change to static libraries.
- Move .h into Alta/ subdir.
* Tue Sep 17 2019 <cjw@ucar.edu> 3.3.1-0
- New release from Alta.
* Tue Aug 13 2019 <cjw@ucar.edu> 3.3.0-0
- created initial package 

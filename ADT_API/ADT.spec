Summary: Alta API library
Name: arinc_enet
Version: 4.0.0
Release: 1
Group: Applications/Engineering
Source: ADT_API-4.0.0.tar.bz2
License: none
BuildArch: x86_64
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
cp libADT_L?_Linux_x86_64_v4000.a $RPM_BUILD_ROOT/opt/local/lib

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(0644,root,root) /opt/local/lib/libADT_L0_Linux_x86_64_v4000.a
%attr(0644,root,root) /opt/local/lib/libADT_L1_Linux_x86_64_v4000.a
%attr(0644,root,root) /opt/local/include/Alta/ADT_L0.h
%attr(0644,root,root) /opt/local/include/Alta/ADT_L1.h

%changelog
* Wed Jan 27 2021 <cjw@ucar.edu> 4.0.0-1
- Update to 4.0.0 which has Kernel 5.0 support.
* Thu Nov 14 2019 <cjw@ucar.edu> 3.3.1-1
- Change to static libraries.
- Move .h into Alta/ subdir.
* Tue Sep 17 2019 <cjw@ucar.edu> 3.3.1-0
- New release from Alta.
* Tue Aug 13 2019 <cjw@ucar.edu> 3.3.0-0
- created initial package

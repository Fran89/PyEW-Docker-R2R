%define debug_package %{nil}
%define version 7.6
%define release fedora.0
%define path_version earthworm_%{version}
# To build the rpm use a command something like
# rpmbuild -ba rpm/ew.fedora.spec
# see: http://earthworm.isti.com/trac/earthworm/wiki/RPM
Summary: Earthworm
License: ISTI
Group: Development/Tools
Name: earthworm
Packager: Tim Zander (t.zander@isti.com)
Vendor: ISTI
#Provides: earthworm = %{version}-%{release}
Release: %{release}
Source: http://www.isti2.com/ew/distribution/earthworm_v7.6-src.tar.gz
URL: http://earthworm.isti.com
Version: %{version}
Buildroot: /tmp/earthworm/
%description
This EW RPM has been tested on Fedora systems.

%prep
%setup -c -n opt/earthworm/

%build
export EW_INSTALL_HOME=`pwd`
export EW_INSTALL_VERSION=%{path_version}
cd %{path_version}
source environment/ew_linux.bash
cd src
make unix

%install
rm -fr $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/opt/earthworm
cp -r earthworm_7.6 $RPM_BUILD_ROOT/opt/earthworm/
rm -fr $RPM_BUILD_ROOT/opt/earthworm/%{path_version}/src
rm -fr $RPM_BUILD_ROOT/opt/earthworm/%{path_version}/include
rm -fr $RPM_BUILD_ROOT/opt/earthworm/%{path_version}/include_cpp
rm -fr $RPM_BUILD_ROOT/opt/earthworm/%{path_version}/lib

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%config /opt/earthworm/%{path_version}/params
%config /opt/earthworm/%{path_version}/environment
%doc /opt/earthworm/%{path_version}/release_notes*
%doc /opt/earthworm/%{path_version}/README*
%docdir /opt/earthworm/%{path_version}/ewdoc
/opt/earthworm/%{path_version}/*


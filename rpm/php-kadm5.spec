# $Id$
# vi: shiftwidth=4 tabstop=4 smarttab expandtab formatoptions=croql

# Basic Information
Name:   php-kadm5
Version:    0.2.4
Release:    1%{?dist}
Summary:    A module kerberos administration in PHP
Group:   Development/Languages
License:    GPLv2
URL: http://cs.stanford.edu

# Packager Information
Packager: Miles Davis <miles@cs.stanford.edu>

# Build Information
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# Source Information
Source0:    php-kadm5.tar.gz
#Patch0:

# Dependency Information
BuildRequires:  php-devel krb5-devel
Requires: php

%description

%prep
%setup -q -n kadm5

%build
phpize
./configure 
make 

%install
mkdir -p $RPM_BUILD_ROOT%{_libdir}/php/modules
install -m 755 modules/kadm5.so $RPM_BUILD_ROOT%{_libdir}/php/modules

%clean
rm -rf %{buildroot}

%post
#/sbin/ldconfig

%postun
#/sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc
%attr(755, root, root) %{_libdir}/php/modules/*
#%{_mandir}/man1/*

%changelog
* Tue Aug 29 2017 Miles Davis <miles@cs.stanford.edu> 0.2.4
- Initial Spec File


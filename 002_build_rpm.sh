#!/bin/sh
mkdir rpmbuild -p
mkdir rpmbuild/SOURCES -p
mkdir rpmbuild/SPECS -p
cp redhat/*.spec rpmbuild/SPECS/
cp frr*.tar.gz rpmbuild/SOURCES/

rpmbuild --define "_topdir `pwd`/rpmbuild" -ba rpmbuild/SPECS/frr.spec

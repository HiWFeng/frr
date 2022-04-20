#!/bin/sh

rm frr*.tar.gz -rf

cd rpmbuild
rm -rf BUILD
rm -rf BUILDROOT
rm -rf RPMS
rm -rf SRPMS
rm -rf SOURCES/* 


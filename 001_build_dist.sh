#!/bin/sh
./bootstrap.sh

extra_version=$1
if [ "$extra_version" == "" ];then
   echo "use none extra_version"
    extra_version="none"
fi

echo "======== extra version is $extra_version ====== "

./configure \
    --bindir=/usr/bin \
    --sbindir=/usr/lib/frr \
    --sysconfdir=/etc/frr \
    --libdir=/usr/lib/frr \
    --libexecdir=/usr/lib/frr \
    --localstatedir=/var/run/frr \
    --with-moduledir=/usr/lib/frr/modules \
    --enable-snmp=agentx \
    --enable-multipath=64 \
    --enable-user=frr \
    --enable-group=frr \
    --enable-vty-group=frrvty \
    --disable-ldpd \
    --enable-fpm \
    --with-pkg-extra-version=-$extra_version \
    SPHINXBUILD=/usr/bin/sphinx-build

make dist


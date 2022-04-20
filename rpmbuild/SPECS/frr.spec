# configure options
#
# Some can be overridden on rpmbuild commandline with:
# rpmbuild --define 'variable value'
#   (use any value, ie 1 for flag "with_XXXX" definitions)
#
# E.g. rpmbuild --define 'release_rev 02' may be useful if building
# rpms again and again on the same day, so the newer rpms can be installed.
# bumping the number each time.

#################### FRRouting (FRR) configure options #####################
# with-feature options
%{!?with_babeld:        %global  with_babeld        1 }
%{!?with_bfdd:          %global  with_bfdd          1 }
%{!?with_bgp_vnc:       %global  with_bgp_vnc       0 }
%{!?with_cumulus:       %global  with_cumulus       0 }
%{!?with_eigrpd:        %global  with_eigrpd        1 }
%{!?with_fpm:           %global  with_fpm           1 }
%{!?with_ldpd:          %global  with_ldpd          1 }
%{!?with_multipath:     %global  with_multipath     256 }
%{!?with_nhrpd:         %global  with_nhrpd         1 }
%{!?with_ospfapi:       %global  with_ospfapi       1 }
%{!?with_ospfclient:    %global  with_ospfclient    1 }
%{!?with_pam:           %global  with_pam           0 }
%{!?with_pbrd:          %global  with_pbrd          1 }
%{!?with_pimd:          %global  with_pimd          1 }
%{!?with_vrrpd:         %global  with_vrrpd         1 }
%{!?with_rtadv:         %global  with_rtadv         1 }
%{!?with_watchfrr:      %global  with_watchfrr      1 }
%{!?with_pathd:         %global  with_pathd         1 }

# user and group
%{!?frr_user:           %global  frr_user           frr }
%{!?vty_group:          %global  vty_group          frrvty }

# path defines
%define     configdir   %{_sysconfdir}/%{name}
%define     _sbindir    /usr/lib/frr
%define     zeb_src     %{_builddir}/%{name}-%{frrversion}
%define     zeb_rh_src  %{zeb_src}/redhat
%define     zeb_docs    %{zeb_src}/doc
%define     frr_tools   %{zeb_src}/tools

# defines for configure
%define     rundir  %{_localstatedir}/run/%{name}

############################################################################

#### Version String tweak
# Remove invalid characters form version string and replace with _
%{expand: %%global rpmversion %(echo '8.2.2-none' | tr [:blank:]- _ )}
%define         frrversion   8.2.2-none

#### Check for systemd or init.d (upstart)
# Check for init.d (upstart) as used in CentOS 6 or systemd (ie CentOS 7)
%if 0%{?fedora} || 0%{?rhel} >= 7 || 0%{?suse_version} >= 1210
    %global initsystem systemd
%else
%if 0%{?rhel} && 0%{?rhel} < 7
    %global initsystem upstart
%else
    %{expand: %%global initsystem %(if [[ `/sbin/init --version 2> /dev/null` =~ upstart ]]; then echo upstart; elif [[ `readlink -f /sbin/init` = /usr/lib/systemd/systemd ]]; then echo systemd; elif [[ `systemctl` =~ -\.mount ]]; then echo systemd; fi)}
%endif
%endif

# Check for python version - use python2.7 on CentOS 6, otherwise python3
%if 0%{?rhel} && 0%{?rhel} < 7
    %global use_python2 1
%else
    %global use_python2 0
%endif

# If init system is systemd, then always enable watchfrr
%if "%{initsystem}" == "systemd"
    %global with_watchfrr 1
%endif

#### Check for RedHat 6.x or CentOS 6.x - they are too old to support PIM.
####   Always disable it on these old systems unconditionally
#
# if CentOS / RedHat and version < 7, then disable PIMd (too old, won't work)
%if 0%{?rhel} && 0%{?rhel} < 7
    %global  with_pimd  0
%endif

# misc internal defines
%{!?frr_uid:            %global  frr_uid            92 }
%{!?frr_gid:            %global  frr_gid            92 }
%{!?vty_gid:            %global  vty_gid            85 }

%define daemon_list zebra ripd ospfd bgpd isisd ripngd ospf6d pbrd staticd bfdd fabricd pathd

%if %{with_ldpd}
    %define daemon_ldpd ldpd
%else
    %define daemon_ldpd ""
%endif

%if %{with_pimd}
    %define daemon_pimd pimd
%else
    %define daemon_pimd ""
%endif

%if %{with_pbrd}
    %define daemon_pbrd pbrd
%else
    %define daemon_pbrd ""
%endif

%if %{with_nhrpd}
    %define daemon_nhrpd nhrpd
%else
    %define daemon_nhrpd ""
%endif

%if %{with_eigrpd}
    %define daemon_eigrpd eigrpd
%else
    %define daemon_eigrpd ""
%endif

%if %{with_babeld}
    %define daemon_babeld babeld
%else
    %define daemon_babeld ""
%endif

%if %{with_vrrpd}
    %define daemon_vrrpd vrrpd
%else
    %define daemon_vrrpd ""
%endif

%if %{with_watchfrr}
    %define daemon_watchfrr watchfrr
%else
    %define daemon_watchfrr ""
%endif

%if %{with_bfdd}
    %define daemon_bfdd bfdd
%else
    %define daemon_bfdd ""
%endif

%if %{with_pathd}
    %define daemon_pathd pathd
%else
    %define daemon_pathd ""
%endif

%define all_daemons %{daemon_list} %{daemon_ldpd} %{daemon_pimd} %{daemon_nhrpd} %{daemon_eigrpd} %{daemon_babeld} %{daemon_watchfrr} %{daemon_pbrd} %{daemon_bfdd} %{daemon_vrrpd} %{daemon_pathd}

#release sub-revision (the two digits after the CONFDATE)
%{!?release_rev:        %global  release_rev        01 }

Summary: Routing daemon
Name:           frr
Version:        %{rpmversion}
Release:        %{release_rev}%{?dist}
License:        GPLv2+
Group:          System Environment/Daemons
Source0:        https://github.com/FRRouting/frr/archive/%{name}-%{frrversion}.tar.gz
URL:            https://www.frrouting.org
Requires(pre):  shadow-utils
Requires(preun): info
Requires(post): info
BuildRequires:  bison >= 2.7
BuildRequires:  c-ares-devel
BuildRequires:  flex
BuildRequires:  gcc
BuildRequires:  json-c-devel
BuildRequires:  libcap-devel
BuildRequires:  make
BuildRequires:  ncurses-devel
BuildRequires:  readline-devel
BuildRequires:  texinfo
BuildRequires:  libyang2-devel
%if 0%{?rhel} && 0%{?rhel} < 7
#python27-devel is available from ius community repo for RedHat/CentOS 6
BuildRequires:  python27-devel
BuildRequires:  python27-sphinx
%else
%if %{use_python2}
BuildRequires:  python-devel >= 2.7
BuildRequires:  python-sphinx
%else
BuildRequires:  python3-devel
BuildRequires:  python3-sphinx
%endif
%endif
%if 0%{?rhel} > 7
#platform-python-devel is needed for /usr/bin/pathfix.py
BuildRequires:  platform-python-devel
%endif
Requires:       initscripts
%if %{with_pam}
BuildRequires:  pam-devel
%endif
%if "%{initsystem}" == "systemd"
BuildRequires:      systemd
BuildRequires:      systemd-devel
Requires(post):     systemd
Requires(preun):    systemd
Requires(postun):   systemd
%else
Requires(post):     chkconfig
Requires(preun):    chkconfig
# Initscripts > 5.60 is required for IPv6 support
Requires(pre):      initscripts >= 5.60
%endif
Provides:           routingdaemon = %{version}-%{release}
Obsoletes:          gated mrt zebra frr-sysvinit
Conflicts:          bird


%description
FRRouting is a free software that manages TCP/IP based routing
protocol. It takes multi-server and multi-thread approach to resolve
the current complexity of the Internet.

FRRouting supports BGP4, OSPFv2, OSPFv3, ISIS, RIP, RIPng, PIM, LDP
NHRP, Babel, PBR, EIGRP and BFD.

FRRouting is a fork of Quagga.


%package contrib
Summary: contrib tools for frr
Group: System Environment/Daemons

%description contrib
Contributed/3rd party tools which may be of use with frr.


%package pythontools
Summary: python tools for frr
%if 0%{?rhel} && 0%{?rhel} < 7
#python27 is available from ius community repo for RedHat/CentOS 6
BuildRequires:  python27
Requires:  python27-ipaddress
%else
%if %{use_python2}
BuildRequires:  python2
Requires:  python2-ipaddress
%else
BuildRequires:  python3
%endif
%endif
Group: System Environment/Daemons

%description pythontools
Contributed python 2.7 tools which may be of use with frr.


%package devel
Summary: Header and object files for frr development
Group: System Environment/Daemons
Requires: %{name} = %{version}-%{release}

%description devel
The frr-devel package contains the header and object files neccessary for
developing OSPF-API and frr applications.


%package rpki-rtrlib
Summary: BGP RPKI support (rtrlib)
Group: System Environment/Daemons
BuildRequires:  librtr-devel >= 0.5
Requires: %{name} = %{version}-%{release}

%description rpki-rtrlib
Adds RPKI support to FRR's bgpd, allowing validation of BGP routes
against cryptographic information stored in WHOIS databases.  This is
used to prevent hijacking of networks on the wider internet.  It is only
relevant to internet service providers using their own autonomous system
number.


%package snmp
Summary: SNMP support
Group: System Environment/Daemons
BuildRequires: net-snmp-devel
Requires: %{name} = %{version}-%{release}

%description snmp
Adds SNMP support to FRR's daemons by attaching to net-snmp's snmpd
through the AgentX protocol.  Provides read-only access to current
routing state through standard SNMP MIBs.


%prep
%setup -q -n frr-%{frrversion}


%build

# For standard gcc verbosity, uncomment these lines:
#CFLAGS="%{optflags} -Wall -Wsign-compare -Wpointer-arith"
#CFLAGS="${CFLAGS} -Wbad-function-cast -Wwrite-strings"

# For ultra gcc verbosity, uncomment these lines also:
#CFLAGS="${CFLAGS} -W -Wcast-qual -Wstrict-prototypes"
#CFLAGS="${CFLAGS} -Wmissing-declarations -Wmissing-noreturn"
#CFLAGS="${CFLAGS} -Wmissing-format-attribute -Wunreachable-code"
#CFLAGS="${CFLAGS} -Wpacked -Wpadded"

%configure \
    --sbindir=%{_sbindir} \
    --sysconfdir=%{configdir} \
    --localstatedir=%{rundir} \
    --disable-static \
    --disable-werror \
    --enable-irdp \
%if %{with_multipath}
    --enable-multipath=%{with_multipath} \
%endif
    --enable-vtysh \
%if %{with_ospfclient}
    --enable-ospfclient \
%else
    --disable-ospfclient\
%endif
%if %{with_ospfapi}
    --enable-ospfapi \
%else
    --disable-ospfapi \
%endif
%if %{with_rtadv}
    --enable-rtadv \
%else
    --disable-rtadv \
%endif
%if %{with_ldpd}
    --enable-ldpd \
%else
    --disable-ldpd \
%endif
%if %{with_pimd}
    --enable-pimd \
%else
    --disable-pimd \
%endif
%if %{with_pbrd}
    --enable-pbrd \
%else
    --disable-pbrd \
%endif
%if %{with_nhrpd}
    --enable-nhrpd \
%else
    --disable-nhrpd \
%endif
%if %{with_eigrpd}
    --enable-eigrpd \
%else
    --disable-eigrpd \
%endif
%if %{with_babeld}
    --enable-babeld \
%else
    --disable-babeld \
%endif
%if %{with_vrrpd}
	--enable-vrrpd \
%else
	--disable-vrrpd \
%endif
%if %{with_pam}
    --with-libpam \
%endif
%if 0%{?frr_user:1}
    --enable-user=%{frr_user} \
    --enable-group=%{frr_user} \
%endif
%if 0%{?vty_group:1}
    --enable-vty-group=%{vty_group} \
%endif
%if %{with_fpm}
    --enable-fpm \
%else
    --disable-fpm \
%endif
%if %{with_watchfrr}
    --enable-watchfrr \
%else
    --disable-watchfrr \
%endif
%if %{with_cumulus}
    --enable-cumulus \
%endif
%if %{with_bgp_vnc}
    --enable-bgp-vnc \
%else
    --disable-bgp-vnc \
%endif
    --enable-isisd \
%if "%{initsystem}" == "systemd"
    --enable-systemd \
%endif
    --enable-rpki \
%if %{with_bfdd}
    --enable-bfdd \
%else
    --disable-bfdd \
%endif
%if %{with_pathd}
    --enable-pathd \
%else
    --disable-pathd \
%endif
    --enable-snmp
    # end

make %{?_smp_mflags} MAKEINFO="makeinfo --no-split"

%if %{use_python2}
# Change frr-reload.py to use python2.7
sed -e '1c #!/usr/bin/python2.7' -i %{zeb_src}/tools/frr-reload.py
sed -e '1c #!/usr/bin/python2.7' -i %{zeb_src}/tools/generate_support_bundle.py
%else
# Change frr-reload.py to use python3
sed -e '1c #!/usr/bin/python3' -i %{zeb_src}/tools/frr-reload.py
sed -e '1c #!/usr/bin/python3' -i %{zeb_src}/tools/generate_support_bundle.py
%endif

pushd doc
make info
popd


%install
mkdir -p %{buildroot}%{_sysconfdir}/{frr,sysconfig,logrotate.d,pam.d,default} \
         %{buildroot}%{_localstatedir}/log/frr %{buildroot}%{_infodir}
make DESTDIR=%{buildroot} INSTALL="install -p" CP="cp -p" install

# Remove this file, as it is uninstalled and causes errors when building on RH9
rm -rf %{buildroot}/usr/share/info/dir

# Remove debian init script if it was installed
rm -f %{buildroot}%{_sbindir}/frr

# kill bogus libtool files
rm -vf %{buildroot}%{_libdir}/frr/modules/*.la
rm -vf %{buildroot}%{_libdir}/*.la
rm -vf %{buildroot}%{_libdir}/frr/libyang_plugins/*.la

# install /etc sources
%if "%{initsystem}" == "systemd"
mkdir -p %{buildroot}%{_unitdir}
install -m644 %{zeb_src}/tools/frr.service %{buildroot}%{_unitdir}/frr.service
%else
mkdir -p %{buildroot}%{_initddir}
ln -s %{_sbindir}/frrinit.sh %{buildroot}%{_initddir}/frr
%endif

install %{zeb_src}/tools/etc/frr/daemons %{buildroot}%{_sysconfdir}/frr
install %{zeb_src}/tools/etc/frr/frr.conf %{buildroot}%{_sysconfdir}/frr/frr.conf.template
install -m644 %{zeb_rh_src}/frr.pam %{buildroot}%{_sysconfdir}/pam.d/frr
install -m644 %{zeb_rh_src}/frr.logrotate %{buildroot}%{_sysconfdir}/logrotate.d/frr
install -d -m750 %{buildroot}%{rundir}

%if 0%{?rhel} > 7 || 0%{?fedora} > 29
# avoid `ERROR: ambiguous python shebang in` errors
pathfix.py -pni "%{__python3} %{py3_shbang_opts}" %{buildroot}/usr/lib/frr/*.py
%py_byte_compile %{__python3} %{buildroot}/usr/lib/frr/*.py
%endif

%pre
# add vty_group
%if 0%{?vty_group:1}
    getent group %{vty_group} >/dev/null || groupadd -r -g %{vty_gid} %{vty_group}
%endif

# add frr user and group
%if 0%{?frr_user:1}
    # Ensure that frr_gid gets correctly allocated
    getent group %{frr_user} >/dev/null || groupadd -g %{frr_gid} %{frr_user}
    getent passwd %{frr_user} >/dev/null || \
    useradd -r -u %{frr_uid} -g %{frr_user} \
        -s /sbin/nologin -c "FRRouting suite" \
        -d %{rundir} %{frr_user}

    %if 0%{?vty_group:1}
        usermod -a -G %{vty_group} %{frr_user}
    %endif
%endif
exit 0


%post
# zebra_spec_add_service <service name> <port/proto> <comment>
# e.g. zebra_spec_add_service zebrasrv 2600/tcp "zebra service"

zebra_spec_add_service ()
{
    # Add port /etc/services entry if it isn't already there
    if [ -f %{_sysconfdir}/services ] && \
        ! %__sed -e 's/#.*$//' %{_sysconfdir}/services 2>/dev/null | %__grep -wq $1 ; then
        echo "$1        $2          # $3"  >> %{_sysconfdir}/services
    fi
}

zebra_spec_add_service zebrasrv 2600/tcp "zebra service"
zebra_spec_add_service zebra    2601/tcp "zebra vty"
zebra_spec_add_service staticd  2616/tcp "staticd vty"
zebra_spec_add_service ripd     2602/tcp "RIPd vty"
zebra_spec_add_service ripngd   2603/tcp "RIPngd vty"
zebra_spec_add_service ospfd    2604/tcp "OSPFd vty"
zebra_spec_add_service bgpd     2605/tcp "BGPd vty"
zebra_spec_add_service ospf6d   2606/tcp "OSPF6d vty"
zebra_spec_add_service isisd    2608/tcp "ISISd vty"
%if %{with_ospfapi}
    zebra_spec_add_service ospfapi  2607/tcp "OSPF-API"
%endif
%if %{with_babeld}
    zebra_spec_add_service babeld   2609/tcp "BABELd vty"
%endif
%if %{with_nhrpd}
    zebra_spec_add_service nhrpd    2610/tcp "NHRPd vty"
%endif
%if %{with_pimd}
    zebra_spec_add_service pimd     2611/tcp "PIMd vty"
%endif
%if %{with_pbrd}
    zebra_spec_add_service pbrd     2615/tcp "PBRd vty"
%endif
%if %{with_ldpd}
    zebra_spec_add_service ldpd     2612/tcp "LDPd vty"
%endif
%if %{with_eigrpd}
    zebra_spec_add_service eigrpd   2613/tcp "EIGRPd vty"
%endif
%if %{with_bfdd}
    zebra_spec_add_service bfdd     2617/tcp "BFDd vty"
%endif
zebra_spec_add_service fabricd	2618/tcp "Fabricd vty"
%if %{with_vrrpd}
    zebra_spec_add_service vrrpd    2619/tcp "VRRPd vty"
%endif
%if %{with_pathd}
    zebra_spec_add_service pathd    2620/tcp "Pathd vty"
%endif

%if "%{initsystem}" == "systemd"
    for daemon in %all_daemons ; do
        %systemd_post frr.service
    done
%else
    /sbin/chkconfig --add frr
%endif

# Fix bad path in previous config files
#  Config files won't get replaced by default, so we do this ugly hack to fix it
%__sed -i 's|watchfrr_options=|#watchfrr_options=|g' %{configdir}/daemons 2> /dev/null || true

# With systemd, watchfrr is mandatory. Fix config to make sure it's enabled if
# we install or upgrade to a frr built with systemd
%if "%{initsystem}" == "systemd"
    %__sed -i 's|watchfrr_enable=no|watchfrr_enable=yes|g' %{configdir}/daemons 2> /dev/null || true
%endif

/sbin/install-info %{_infodir}/frr.info.gz %{_infodir}/dir

# Create dummy config file if they don't exist so basic functions can be used.
if [ ! -e %{configdir}/frr.conf ] && [ ! -e %{configdir}/zebra.conf ]; then
    # No frr.conf and per daemon configs exist
    mv %{configdir}/frr.conf.template %{configdir}/frr.conf
%if 0%{?frr_user:1}
    chown %{frr_user}:%{frr_user} %{configdir}/frr.conf
%endif
    chmod 640 %{configdir}/frr.conf
fi
%if 0%{?frr_user:1}
    chown %{frr_user}:%{frr_user} %{configdir}/daemons
%endif

%if %{with_watchfrr}
    # No config for watchfrr - this is part of /etc/sysconfig/frr
    rm -f %{configdir}/watchfrr.*
%endif

if [ ! -e %{configdir}/vtysh.conf ]; then
    touch %{configdir}/vtysh.conf
    chmod 640 %{configdir}/vtysh.conf
%if 0%{?frr_user:1}
    %if 0%{?vty_group:1}
        chown %{frr_user}:%{vty_group} %{configdir}/vtysh.conf*
    %endif
%endif
fi


%postun
if [ "$1" -ge 1 ]; then
    #
    # Upgrade from older version
    #
    %if "%{initsystem}" == "systemd"
        ##
        ## Systemd Version
        ##
        %systemd_postun_with_restart frr.service
    %else
        ##
        ## init.d Version
        ##
        service frr restart >/dev/null 2>&1
    %endif
    :
fi


%preun
%if "%{initsystem}" == "systemd"
    ##
    ## Systemd Version
    ##
    if [ $1 -eq 0 ] ; then
        %systemd_preun frr.service
    fi
%else
    ##
    ## init.d Version
    ##
    if [ $1 -eq 0 ] ; then
        service frr stop  >/dev/null 2>&1
        /sbin/chkconfig --del frr
    fi
%endif
/sbin/install-info --delete %{_infodir}/frr.info.gz %{_infodir}/dir


%files
%doc COPYING
%doc doc/mpls
%doc README.md
/usr/share/yang/*.yang
%if 0%{?frr_user:1}
    %dir %attr(751,%{frr_user},%{frr_user}) %{configdir}
    %dir %attr(750,%{frr_user},%{frr_user}) %{_localstatedir}/log/frr
    %dir %attr(751,%{frr_user},%{frr_user}) %{rundir}
%else
    %dir %attr(750,root,root) %{configdir}
    %dir %attr(750,root,root) %{_localstatedir}/log/frr
    %dir %attr(750,root,root) %{rundir}
%endif
%{_infodir}/frr.info.gz
%{_mandir}/man*/*
%{_sbindir}/zebra
%{_sbindir}/staticd
%{_sbindir}/ospfd
%{_sbindir}/ripd
%{_sbindir}/bgpd
%exclude %{_sbindir}/ssd
%if %{with_watchfrr}
    %{_sbindir}/watchfrr
%endif
%{_sbindir}/ripngd
%{_sbindir}/ospf6d
%if %{with_pimd}
    %{_sbindir}/pimd
%endif
%if %{with_pbrd}
    %{_sbindir}/pbrd
%endif
%if %{with_vrrpd}
    %{_sbindir}/vrrpd
%endif
%{_sbindir}/isisd
%{_sbindir}/fabricd
%if %{with_ldpd}
    %{_sbindir}/ldpd
%endif
%if %{with_eigrpd}
    %{_sbindir}/eigrpd
%endif
%if %{with_nhrpd}
    %{_sbindir}/nhrpd
%endif
%if %{with_babeld}
    %{_sbindir}/babeld
%endif
%if %{with_bfdd}
    %{_sbindir}/bfdd
%endif
%if %{with_pathd}
    %{_sbindir}/pathd
    %{_libdir}/frr/modules/pathd_pcep.so
%endif
%{_libdir}/libfrr.so*
%{_libdir}/libfrrcares*
%{_libdir}/libfrrospf*
%if %{with_fpm}
    %{_libdir}/frr/modules/zebra_fpm.so
%endif
%{_libdir}/frr/modules/zebra_cumulus_mlag.so
%{_libdir}/frr/modules/dplane_fpm_nl.so
%{_libdir}/frr/modules/zebra_irdp.so
%{_libdir}/frr/modules/bgpd_bmp.so
%{_bindir}/*
%config(noreplace) %{configdir}/[!v]*.conf*
%config(noreplace) %attr(750,%{frr_user},%{frr_user}) %{configdir}/daemons
%if "%{initsystem}" == "systemd"
    %{_unitdir}/frr.service
%else
    %{_initddir}/frr
%endif
%config(noreplace) %{_sysconfdir}/pam.d/frr
%config(noreplace) %{_sysconfdir}/logrotate.d/frr
%{_sbindir}/frr-reload
%{_sbindir}/frrcommon.sh
%{_sbindir}/frrinit.sh
%{_sbindir}/watchfrr.sh


%files contrib
%doc tools


%files pythontools
%{_sbindir}/generate_support_bundle.py
%{_sbindir}/frr-reload.py
%{_sbindir}/frr_babeltrace.py
%if 0%{?rhel} > 7 || 0%{?fedora} > 29
%{_sbindir}/__pycache__/*
%else
%{_sbindir}/generate_support_bundle.pyc
%{_sbindir}/generate_support_bundle.pyo
%{_sbindir}/frr-reload.pyc
%{_sbindir}/frr-reload.pyo
%{_sbindir}/frr_babeltrace.pyc
%{_sbindir}/frr_babeltrace.pyo
%endif


%post rpki-rtrlib
# add rpki module to daemons
sed -i -e 's/^\(bgpd_options=\)\(.*\)\(".*\)/\1\2 -M rpki\3/' %{_sysconfdir}/frr/daemons

%postun rpki-rtrlib
# remove rpki module from daemons
sed -i 's/ -M rpki//' %{_sysconfdir}/frr/daemons

%files rpki-rtrlib
%{_libdir}/frr/modules/bgpd_rpki.so


%files snmp
%{_libdir}/libfrrsnmp.so*
%{_libdir}/frr/modules/*snmp.so


%files devel
%{_libdir}/lib*.so
%dir %{_includedir}/%{name}
%{_includedir}/%{name}/*.h
%dir %{_includedir}/%{name}/ospfd
%{_includedir}/%{name}/ospfd/*.h
%if %{with_bfdd}
    %dir %{_includedir}/%{name}/bfdd
    %{_includedir}/%{name}/bfdd/bfddp_packet.h
%endif
%if %{with_ospfapi}
    %dir %{_includedir}/%{name}/ospfapi
    %{_includedir}/%{name}/ospfapi/*.h
%endif
%if %{with_eigrpd}
    %dir %{_includedir}/%{name}/eigrpd
    %{_includedir}/%{name}/eigrpd/*.h
%endif


%changelog
* Sat Mar 12 2022 Martin Winter <mwinter@opensourcerouting.org> - %{version}

* Sat Mar 12 2022 Jafar Al-Gharaibeh <jafar@atcorp.com> - 8.2.2
- Fix zebra nexthop tracking
- Fix bgp graceful restart double config

* Wed Mar  9 2022 Jafar Al-Gharaibeh <jafar@atcorp.com> - 8.2.1
- Fix bgp rpki hang issue
- Fixes for grpc

* Tue Mar  1 2022 Jafar Al-Gharaibeh <jafar@atcorp.com> - 8.2
- The FRRouting community would like to announce FRR Release 8.2.
- This release consists of just over 800 commits from 62 authors.
- Selected features and bug fixes are listed below.
- babeld:
-    Fix the checks for truncated packets
- bfdd:
-    Correct one spelling error of comment
-    Fix detection timeout update
-    Fix possibly wrong counter of control packets
- bgpd:
-    Add "json" option to a few more show commands
-    Add 'show bgp <afi> <safi> json detail' header data
-    Add a 6 hour warning to missing policy
-    Add an ability to match ipv6 next-hop by prefix-list
-    Add autocomplete for access-list under bmp node
-    Add autocomplete for as-path filters
-    Add autocomplete for set/match community/large/ext lists
-    Add long-lived graceful restart capability
-    Add peer-groups to neighbor autocomplete
-    Adjust symbolic names for cease notifications according to rfc4486
-    Deprecate dpa, advertiser and rcid_path path attributes
-    Extended bgp administrative shutdown communication
-    Fix crash when using "show bgp vrf all"
-    Fix inconsistency of match ip/ipv6 next-hop commands
-    Fix missing name of default vrf
-    Handle TCP connection errors with connection callbacks for RPKI
-    Implement llgr helper mode
-    Implement rfc9072
-    Support redirect import more than one route-target ipv6
- docker:
-    Update alpine build enable set own version
- isisd:
-    Add link state traffic engineering support
-    Fix router capability tlv parsing issues
-    Fix running-config for fast-reroute
-    Make isis work with default vrf name different than 'default'
- ospf6d
-    Add missing vrf parameter to "clear ipv6 ospf6 interface"
-    Add prompt for commands with non-exist vrf
-    Add support for nssa type-7 address ranges
-    Add the ability of specifying router-id/area-id in no debug ospf6
-    Do not originate type-4 lsa when nssa
-    Do not send type-5 into stub area
-    Fix ecmp inter-area route nexthop update
-    Fix memory leak for `show ipv6 ospf6 zebra json`
- ospfd
-    Fix wrong comparison of routemap name
-    Fix crash on "ospf send-extra-data zebra"
-    Fix incorrect detection of topology changes in helper mode
-    Fix loss of mixed form in "range" command
-    Fix no-form of "graceful-restart" command
-    Fix summary-address deletion
-    Fix wrong parsing of te subtlv
- pbrd
-    Add vlan actions to vty
-    Pbr route maps get addr family of nhgs
-    Protect from a possible null dereference
- pimd
-    Do not allow 224.0.0.0/24 range in igmp join
-    Fix igmp user config
-    Fix msdp mesh grp with wildcard member addr
-    Fix stale forwarding entries left around after join goes away
-    Fix FRR drops IGMP packets for TOS value other than 0XC0
- redhat
-    Check if frr.conf already exists
-    Logrotate file has typo for staticd
- ripd
-    Fix packet send for non primary addresses
- vtysh
-    Add missing rpki node when showing config
-    Improve startup time by ca. ×6
-    remove `address-family evpn`
- watchfrr
-    Allow an integrated config to work within a namespace
- zebra
-    Add optional nhg id output to `show ip ro`
-    Add resolver flag for nexthop in json
-    Add support for json output in srv6 locator detail command
-    Don't lose next hop weights while exporting via fpm
-    Fix buffer overflow
-    Fix netns deletion
-    Fix route-map application when when using vrfs

* Tue Nov  2 2021 Jafar Al-Gharaibeh <jafar@atcorp.com> - 8.1
-    FRR 8.1 brings a long list of enhancements and fixes with 1200 commits from
-    75 developers. Thanks to all contributers.
- New Features:
-    Lua hooks are now feature complete, with one hook available for use (http://docs.frrouting.org/en/latest/scripting.html)
-    Improvements to SRv6 (Segment Routing over IPv6) (http://docs.frrouting.org/en/latest/zebra.html#segment-routing-ipv6)
-    Improvements to Prefix-SID (Type 5)
-    EVPN route type-5 gateway IP overlay Index (http://docs.frrouting.org/en/latest/bgp.html#evpn-overlay-index-gateway-ip)
-    OSPFv3 NSSA and NSSA totally stub areas (http://docs.frrouting.org/en/latest/ospf6d.html#ospf6-area)
-    OSPFv3 ASBR summarization (http://docs.frrouting.org/en/latest/ospf6d.html#asbr-summarisation-support-in-ospfv3)
-    OSPFv3 Graceful Restart (http://docs.frrouting.org/en/latest/ospf6d.html#graceful-restart)
-    OSPFv2 Graceful Restart (restarting mode added, helper was already implemented) (http://docs.frrouting.org/en/latest/ospfd.html#graceful-restart)
- Behavior Changes
-    Every node in running config now has an explicit "exit" tag
-    Link bandwidth in BGP is now correctly encoded according to IEEE 754. To stay with old incorrect encoding use:
-    `neighbor PEER disable-link-bw-encoding-ieee`
- Changelog
- alpine:
    Fix path for daemons file install
- BGP:
-    Add "json" option to "show bgp as-path-access-list"
-    Add `disable-addpath-rx` knob
-    Add an ability to set extcommunity to none in route-maps
-    Add counter of displayed show bgp summary when filtering
-    Add knob to config cond-adv scanner period
-    Add route-map `match alias` command
-    Add rpki source address configuration
-    Add show bgp summary filter by neighbor or as
-    Add terse display option on show bgp summary
-    Allow for auto-completion of community alias's created
-    Bgp knob to teardown session immediately when peer is unreachable
-    Expand 'bgp default <afi>-<safi>' cmds
-    Extend evpn next hop tracking to type-1 and type-4 routes
-    Fix "no router bgp x vrf default"
-    Flowspec redirect vrf uses vrf table instead of allocated table id
-    Handle quick flaps of an evpn prefix properly
-    Initial batch of evpn lttng tracepoints
-    Limit processing to what is needed in rpki validation
-    Modify vrf/view display in show bgp summary
-    Set 4096 instead of 65535 as new max packet size for a new peer
-    Set extended msg size only if we advertised and received capability
-    Show bgp community alias in json community list output
-    Show bgp prefixes by community alias
-    Show max packet size per update-group
-    Split soft reconfigure table task into several jobs to not block vtysh
-    Store distance received from a redistribute statement
-    Update route-type-1 legend to match output
- ISIS:
-    Fix sending of lsp with null seqno
- lib:
-    Add "json" option to "show ip[v6] access-list"
-    Add "json" option to "show ip[v6] prefix-list"
-    Add "json" option to "show route-map"
-    Prevent grpc assert on missing yang node
- NHRP:
-    Clear cache when shortcuts are cleared
-    Fix corrupt address being shown for shortcuts with no cache entry
-    Set prefix correctly in resolution request
- OSPF6:
-    Add debug commands for lsa all and route all
-    Add warning log for late hello packets
-    Add write-multiplier configuration
-    Don't update router-id if at least one adjacency is full
-    Extend the "redistribute" command with more options
-    Fix issue when displaying the redistribute command
-    Fix logging of border router routes
-    Json output for database dump show command
-    Link state id in lsa database json output
-    Send lsa update immediately when ospf instance is deleted
- OSPF:
-    Fix crash when creating vlink in unknown vrf
-    Gr conformance fix for hello packet dr election
-    Print extra lsa information in some log messages
-    Rfc conformance test case 25.23 issue fix
-    Show ip ospf route json does not shown metric and tag
-    Summary lsa is not originated when process is reset
- pathd:
-    Handle pcinitiated configuration, main thread
-    Handle pcinitiated messages, thread controller
-    Handle srp_id correctly
-    If pce ret no-path to pcreq don't retry pcreq nor delegate
- PBR:
-    Add `match ip-protocol [tcp|udp]`
-    Add ability to set/unset src and dest ports
-    Nhg "add" edge case for last in table range
-    Start inclusion of src and dst ports for pbrd
- PIM:
-    Add tos/ttl check for igmp conformance
-    Allow join prune intervals to be as small as 5 seconds
-    Allow msdp group name 'default'
-    Fix register suppress timer code
-    Fix uaf/heap corruption in bsm code
-    Fix command "no ip msdp mesh-group member"
-    Igmp groups are not getting timeout
-    Igmp memberships are not querier specific
-    Igmp sockets need to be iface-bound too
-    Prevent uninited usage of nexthop
-    Support msdp global timers configuration
- vtysh
-    Add cli timestamp '-t' flag
-    Add error code if daemon is not running
-    Fix searching commands in parent nodes
- yang:
-    Add msdp timer configuration
-    Fix bgp multicast prefix type
-    Mark a couple of prefix-list/access-list leafs as mandatory
-    Move multicast prefix type definition
-    Replace an empty pattern with a zero-length restriction
-    Rework pim msdp mesh group
-    Simplify msdp peer handling
- zebra :
-    Add "json" option to "show interface"
-    Various improvment to dataplane interface
-    Add message counts for `show zebra client`
-    Add nhg id to show ip route json
-    Add show command for ra interface lists
-    Fix ipv4 routes with ipv6 link local next hops install in fpm
-    Handle bridge mac address update in evpn contexts
-    Move individual lines to table in `show zebra client` command
-    Refresh vxlan evpn contexts, when bridge interface goes up
-    Update zl3vni when bridge link refreshed in other namespaces

* Wed Jul 21 2021 Martin Winter <mwinter@opensourcerouting.org> - 8.0
- Major changes
-    New daemon pathd for segment routing
-    EVPN Multihoming is now fully supported
-    OSPFv3 now supports VRFs
-    TI-LFA has been implemented in IS-IS and OSPF
-    Ability for Zebra to dump netlink messages in a human-friendly format
-    LDP gained SNMP support
-    libyang minimun version is now 2.0
- BABEL:
-    Add `distribute-list` commands
-    Fix memory leak in connected route handling
- BGP:
-    Add support for use of aliases with communities
-    Add support of tcp-mss for neighbors
-    Add support for EVPN Multihoming
-    Add ability to show BGP routes from a particular table version
-    Add support for for RFC 8050 (MRT add-path)
-    Add SNMP support for MPLS VPN
-    Add `show bgp summary wide` command to show more detailed output 
     on wide terminals
-    Add ability for peer-groups to have `ttl-security hops` configured
-    Add support for conditional Advertisement
-    Add support for RFC 4271 Delay Open Timer
-    Add a knob to not advertise until route is installed in fib
-    Add BGP-wide configuration for graceful shutdown
-    Add support for RFC 8654 extended messages
-    Improve RPKI reporting as well as new show commands
-    Improve handling of VRF route leaking
-    Improve scaling behavior for dynamic neighbors
-    Improve LL nexthop tracking to be interface based
-    Improve route reachability handling with respect to blackhole routes
-    Improve SNMP traps to RFC 4273 notifications
-    Improve EVPN routes to use L3 NHG's where applicable
-    Improvements to EVPN
-    Improvements to update behavior
-    Fix various issues with connection resolution
-    Fix statistics commands in some situations
-    Fix non-determistic locally-originated paths in bestpath selection
-    Continue working on transitioning to YANG/Northbound configuration
-    Various bug fixes and performance improvements
- EIGRP:
-    Add `distribute-list` commands
-    Ensure received AS number is the same as ours in all situations
-    Properly validate TLV lengths in some situations
- IS-IS:
-    Add ldb-sync functionality
-    Add TI-LFA functionality
-    Add support for Anycast-SID's
-    Add support for classic LFA RFC 5286
-    Add `show isis fast-reroute summary` command
-    Add support for Remote LFA RFC 7490
-    Fix Attach-bit processing in some scenarios
-    Cleanup BFD integration
-    Various bug fixes and performance improvements
- LDP:
-    Add SNMP support
-    Support for LDP IGP Synchronization
-    Support for RLFA clients
-    Various bug fixes and performance improvements
- LIBFRR:
-    Various bugfixes and performance improvements
- NHRP:
-    Add `nhrp multicast-nflog-group (1-65535)` command
-    Add configuration options for vici socket path
-    Add support for forwarding multicast packets
-    Fix handling of MTU
-    Fix handling of NAT extension
-    Retry IPsec under some conditions
- OSPFv2:
-    Add OSPF GR helper support
-    Add JSON support for various commands
-    Add `summary-address A.B.C.D/M ...` commands
-    Add `area X nssa suppress-fa` command
-    Add support for TI-LFA
-    Add support for BFD profiles
-    Add support for Traffic Engineering database
-    Add support for usage of DMPVPN with OSPF
-    Add `clear ip ospf neighbor` commands
-    Add YANG support for route-maps
-    Improvements to SNMP
-    Fixes for type 5 and type 7 LSA handling
-    Various bug fixes and performance improvements
- OSPFv3:
-    Add support for VRFs
-    Add JSON support to a bunch of commands
-    Add ability to control maximum paths for routes
-    Add `show ipv6 ospf6 vrfs` command
-    Add support for BFD profiles
-    Fix to not send hellos on loopbacks
-    Cleanup area handling around interfaces
-    YANG support for route-maps
-    Various bug fixes and performance improvements
- OSPFCLIENT:
-    Cleanup trust of user input
- PATHd:
-    Add support of SR-TE policy management daemon
-    Add optional support for PCEP to pathd
-    Integrate PCEP-LIB into FRR
- PBR:
-    Add `set installable` nhg command
-    Improve interface up/down event handling
- PIM:
-    Add YANG integration
-    Add JSON support to various commands
-    Add BFD profile support
-    Fixes to IGMP conformance
-    Fixes to behavior surrounding Prune and Prune-pending
-    Various bug fixes and performance improvements
- RIPNG:
-    Fix interface wakeup after shutdown
- SHARP:
-    Add ability to use Nexthop Groups
-    Add v4 redistribute watching
-    Add TED support
-    Various bug fixes
- STATIC:
-    Fix nexthop handling in some situations
-    Forbid blackhole and non-blackhole nexthops in a single route
- VRRP:
-    printf formatting cleanups
- VTYSH:
-    Add a `show history` command
-    Add `show memory <daemon>` support
-    Start deprecation cycle for `address-family evpn`
-    Display version with --help
-    Various bug fixes
- WATCHFRR:
-    Fix some crashes
- ZEBRA:
-    Add JSON support to various commands
-    Add Human readable netlink dumps
-    Add L2 NHG support
-    Add support for LSPs to FPM dataplane
-    Add ability for other protocol daemons to install nexthop groups into the kernel
-    Add YANG support for route-maps
-    Improve scale performance when handling a large number of VRF's
-    Improve network namespace handling
-    Improve asic-offload handling
-    Improve FreeBSD interface and route handling
-    Improve handling of neighbors in kernel dataplane plugin
-    Improve label manager
-    Improve route-map processing
-    Improve debug-ability of routes and VRFs
-    Improve FPM dataplane plugin
-    Improve handling of reachability / nexthop tracking on shutdown interfaces
-    Improve EVPN support
-    Fix startup handling of `set src X`
-    Various bug fixes and performance improvements

* Wed Mar  3 2021 Martin Winter <mwinter@opensourcerouting.org> - 7.5.1
- BABEL:
-    Fix connected route leak on change
- BFD:
-    Session lookup was sometimes wrong
-    Memory leak and handling cleanups
-    In some situations handle vrf appropriately when receiving packets
- BGP:
-    Peer Group Inheritance Fixes
-    Dissallow attempt to peer peers reachable via blackholes
-    Send BMP down message when reachability fails
-    Cleanup handling of aggregator data when the AGG AS is 0
-    Handle `neighbor <peer-group allowas-in` config changes properly
-    Properly parse community and lcommunity values in some circumstances
-    Allow peer-groups to configure `ttl-security hops`
-    Prevent v6 routes with v4 nexthops from being installed
-    Allow `default-originate` to be cleared from a peer group
-    Fix evpn route-map vni filter at origin
-    local routes were using non-default distance
-    Properly track if the nexthop was updated in some circumstances
-    Cleanup `show running` when running bgp with `-e X` values
-    Various Memory leaks in show commands
-    Properly withdraw exported routes when deleting a VRF
-    Avoid resetting ebgp-multihop if peer setting is the same as peer-group
-    Properly encode flowspec rules to zebra in some rare circumstances
-    Generate statistics for routes in bgp when we have exactly 1 route
-    Properly apply route-map for the default-originate command
- EIGRP:
-    Properly set MTU for eigrp packets sent
-    Various memory leaks and using uninited data fixes
- ISIS:
-    When last area address is removed, resign if we were the DR
-    Various memory leaks and using uninited data fixes
- LDP:
-    Various memory leaks and using uninited data fixes
- NHRP:
-    Use onlink routes when prefix == nh
-    Shortcut routes are installed with proper nexthop
- OSPF:
-    Prevent duplicate packet read in multiple vrf situation
-    Fix area removal at interface level
-    Restore Point to MultiPoint interface types
-    Correctly handle MTU change on startup
-    Multi Instance initialization sometimes was not successful
-    NSSA translate-always was not working properly
- OSPFv3:
-    Don't send hellos on loopback interfaces
-    Handle ECMP better when a sub-path is removed
-    Memory leak and handling fixes
-    Fix Link LSA not updating when router priority is modified
-    Some output from show commands was wrong
-    Intra area remote connected prefixes sometimes not installed
- PBR:
-    Various memory leaks and using uninited data fixes
- PIM:
-    SGRpt prune received during prune didn't override holdtime
-    Various memory leaks and using uninited data fixes
- STATIC:
-    Fix VRF and usage on startup in some instances
-    Tableid was being mishandled in some cases
- VTYSH:
-    Disable bracketed paste in readline.
- WATCHFRR:
-    Various memory leaks and using uninited data fixes
- ZEBRA:
-    Always install blackhole routes using kernel routes instead of nexthops
-    Various memory leaks and using uninited data fixes
-    Dissallow resolution to duplicate nexthops that created infinite nexthops
-    Apply the route-map delay-timer globally
-    Some routes were stuck in Queued state when using the FPM
-    Better handle vrf creation when using namespaces
-    Set NUD_NOARP on sticky mac entries in addtion to NTF_STICKY
-    Allow `set src X` to work on startup
- FRR Library:
-    Fix a variety of memory leaks
-    Fix VRF Creation in some instances
-    RPKI context editing was not properly handled in reload situations
-    routemap code was not properly handling modification of CLI in some instances
- SNAPCRAFT:
-    Update to using rtrlib 0.7.0
-    Fix passthrough path for Libyang 1.x
- ALPINE:
-    Remove old docker deps

* Mon Nov  2 2020 Donald Sharp <sharpd@nvidia.com> - 7.5
- BFD
-   Profile support
-   Minimum ttl support
- BGP
-   rpki VRF support
-   GR fixes
-   Add wide option to display of routes
-   Add `maximum-prefix <num> force`
-   Add `bestpath-routes` to neighbor command
-   Add `bgp shutdown message MSG...` command
-   Add v6 Flowspec support
-   Add `neighbor <neigh> shutdown rtt` command
-   Allow update-delay to be applied globaly
- EVPN
-   Beginning of MultiHoming Support
- ISIS
-   Segment Routing Support
-   VRF Support
-   Guard against adj timer display overflow
-   Add support for Anycast-SIDs
-   Add support for Topology Independent LFA (TI-LFA)
-   Add `lsp-gen-interval 2` to isis configuration
- OSPF
-   Segment Routing support for ECMP
-   Various LSA fixes
-   Prevent crash if transferring config amongst instances
- PBR
-   Adding json support to commands
-   DSCP/ECN based PBR Matching
- PIM
-   Add more json support to commands
-   Fix missing mesh-group commands
-   MSDP SA forwarding
-   Clear (s,g,rpt) ifchannel on (*, G) prune received
-   Fix igmp querier election and IP address mapping
-   Crash fix when RP is removed
- STATIC
-   Northbound Support
- YANG
-   Filter and route-map Support
-   OSPF model definition
-   BGP model definition
- VTYSH
-   Speed up output across daemons
-   Fix build-time errors for some --enable flags
-   Speed up output of configuration across daemons
- ZEBRA
-   nexthop group support for FPM
-   northbound support for rib model
-   Backup nexthop support
-   netlink batching support
-   Allow upper level protocols to request ARP
-   Add json output for zebra ES, ES-EVI and access vlan dumps
-
- Upgrade to using libyang1.0.184
-
- RPM
-   Moved RPKI to subpackage
-   Added SNMP subpackage
-
- As always there are too many bugfixes to list individually.  This release
- compromises just over 1k of commits by the community, with contributors from
- 70 people.

* Tue Jun 30 2020 Martin Winter <mwinter@opensourcerouting.org> - 7.4
- BGPd
-    Use sequence numbers for community lists
-    Fixes to nexthop groups
-    Add feature to limit outgoing number of routes
-    Per Neighbor Graceful Restart
-    Multiple Graceful Restart fixes
-    Support sub-Type-4 and sub-Type-5 for the VPNv4 SRv6 backend
-    rfc7606 support: treat certain malformed routes as withdraw
-    allow origin override for route aggregates
-    rfc6608 support: Subcodes for BGP Finite State Machine Error
-    rfc7607 support: Codification of AS 0 Processing
-    rfc6286 support: Autonomous-System-Wide Unique BGP Identifier for BGP-4
-    Unequal cost multipath (a.ka. weighted ECMP) with BGP link-bandwidth
-    Enable rfc8212 by default except datacenter profile
- staticd
-    Add debug support
- vtysh
-    Add copy command to copy config from file into running config
- LDPd
-    adding support for LDP ordered label distribution control
- ISISd
-    IS-IS Segment Routing support
- SHARPd
-    add initial support to add/remove lsps
- Zebra
-    fix broadcast address in IPv4 networks with /31 mask
-    Add Graceful Restart support for Protocol Daemon restarts
- lib
-    migrate route-maps to use northbound interface
- plus countless bug fixes and other improvements

* Mon Jun 15 2020 Sascha Kattelmann <sascha@netdef.org>
- Add Pathd support

* Wed May 06 2020 David Lamparter <equinox@opensourcerouting.org> - 7.3.1
- upstream 7.3.1

* Fri Feb 14 2020 Martin Winter <mwinter@opensourcerouting.org> - 7.3
- BGPd
-    EVPN PIP Support
-    Route Aggregation code speed ups
-    BGP Vector I/O speed ups
-    New CLI: `set distance XXX`
-    New CLI: `aggregate-address A.B.C.D/M route-map WORD`
-    New CLI: `bgp reject-as-sets`
-    New CLI: `advertise pip ...`
-    New CLI: `match evpn rd ASN:NN_OR_IP-ADDRESS:NN`
-    New CLI: `show bgp l2vpn evpn community|large-community X`
-    New CLI: `show bgp l2vpn evpn A.B.C.D`
-    Auto-completion for clear bgp command
-    Add ability to set tcp socket buffer size
- OSPFd
-    Partial MPLS TE support
- PBRd
-    New CLI: `set vrf unchanged|NAME`
- BFDd
-    VRF Support
-    New CLI: 'show bfd peers brief'
-    New CLI: 'clear bfd peer ...'
- PIMd
-    Significant Speedups in accessing Internal Data for higher scale
-    Support for joining any-source Multicast
-    Updated CLI: 'show ip pim upstream-join-desired'
-    New CLI: 'show ip pim channel'
-    Debug Cleanup
-    MLAG experimental support
- VRRPd
-    VRF Support
-    Northbound Conversion- NHRPd
- LDPd
- vtysh
-    New CLI: `banner motd line LINE...`
- yang
-    New CLI: `show yang operational-data XPATH`
-    New CLI: `debug northbound`
- Zebra
-    Nexthop Group support
-    New CLI: 'debug zebra nexthop [detail]'
-    New CLI: 'show router-id'
-    MLAG experimental support
- watchfrr
-    Additional status messages of system state to systemd
-    New CLI: `watchfrr ignore DAEMON`
- Others
-    As always all daemons have received too many bug fixes to fully list
-    There has been a significant focus on increasing test coverage
- Change in Behavior:
-    ISISd
-       All areas created default automatically to level-1-2
-    Zebra
-       Nexthop Group Installation in Kernel is turned on by default
        if the kernel supports-    New CLI: 'show nexthop-group rib [singleton]'
-    Man Pages
-       Renamed to frr-* to remove collision with other packages

* Fri Jan 17 2020 Martin Winter <mwinter@opensourcerouting.org> - 7.2.1
- BGPd
-   Fix Addpath issue
-   Do not apply eBGP policy for iBGP peers
-   Show `ip` and `fqdn` in json output for `show [ip] bgp <route> json`
-   Fix large route-distinguisher's format
-   Fix `no bgp listen range ...` configuration command
-   Autocomplete neighbor for clear bgp
-   Reflect the distance in RIB when it is changed for an arbitrary afi/safi
-   Notify "Peer De-configured" after entering 'no neighbor <neighbor> cmd
-   Fix per afi/safi addpath peer counting
-   Rework BGP dampening to be per AFI/SAFI
-   Do not send next-hop as :: in MP_REACH_NLRI if no link-local exists
-   Override peer's TTL only if peer-group is configured with TTL
-   Remove error message for unkown afi/safi combination
-   Keep the session down if maximum-prefix is reached
- OSPFd
-   Fix BFD down not tearing down OSPF adjacency for point-to-point net
- BFDd
-   Fix multiple VRF handling
-   VRF security improvement
- PIMd
-   Fix rp crash
- NHRPd
-   Make sure `no ip nhrp map <something>` works as expected
- LDPd
-   Add missing sanity check in the parsing of label messages
- Zebra
-   Use correct state when installing evpn macs
-   Capture dplane plugin flags
- lib
-   Fix interface config when vrf changes
-   Fix Interface Infinite Loop Walk (for special interfaces such as bond)
- snapcraft
-   fix missing vrrpd daemon
- Others
-   Rename man pages (to avoid conflicts with other packages)
-   Various other fixes for code cleanup and memory leaks

* Fri Dec 27 2019 Donatas Abraitis <donatas.abraitis@gmail.com>
- Add CentOS 8 support

* Tue Oct 15 2019 Martin Winter <mwinter@opensourcerouting.org> - 7.2
- ALL Daemons
-   -N <namespace> to allow for config file locating when running FRR inside
     of a namespace
-   Impoved Testing across all daemons
- BFD
-   VRF Support
-   Conversion to Northbound interface
- BGP
-   Aggregate-address add route-map support
-   BMP Support
-   Improved JSON output for many commands
-   `show bgp afi safi summary failed` command
-   `clear bop *` clears all peers
-   Show FQDN for `show bgp ipv4 uni` commands
-   Display BestPath selection reason as part of show commands
- EIGRP
-   Infrastructure changes to allow VRF's
-   SIGHUP signals the config reload
-   Conversion to Northbound interface
- ISIS
-   BFD Support
-   Support for circuits with MTU > 8192
- PBRD
-   fwmark support as part of match criteria
-   autocompletion of PBRMAPS
-   Improved Nexthop Support
- PIMD
-   PIM-BSM receive support
-   Improved debugging support
-   Store ECMP paths that are not currently legal for use
-   Disallow igmp query from a non-connected source
-   Many new cli improvements and changes
- VRRPD
-   Add Support for RFC 3768 and RFC 5798
- Route-Maps
-   Add sequence numbers to access-lists
-   Add `match ip next-hop type blackhole`
-   Improved ability to notice dependency changes
- SHARPD
-   `sharp watch [import|nexthop]` you can now specify a prefix instead
    of assuming a /32
- STATICD
-   Significantly Improved NHT
- ZEBRA
-   Many dataplane improvements for routes, neighbor table and EVPN
-   NHT cli can now be specified per VRF and improved ability to control
    NHT data being shown
-   Removed duplicate processing of routes
-   Improved debugablility
-   RMAC and VxLan support for the FPM
- LIB
-   RCU support
-   Nexthop Group Improvements
-   `log-filter WORD` added
- Building
-   openssl support
-   libcap should be used as part of build or significant slowdowns
    will be experienced
-   Lua builds have been fixed
-   Improved Cross building

* Mon Jun 17 2019 David Lamparter <equinox@opensourcerouting.org> - 7.1
- gRPC northbound plugin
- "table NNN" removed from zebra
- more dataplane MT work
- EVPN in non-default VRFs
- RFC 8212 (default deny policy for eBGP)
- RFC 8106 (IPv6 RA DNS options)

* Wed May  8 2019 Martin Winter <mwinter@opensourcerouting.org> - 7.0.1
- bgp:
-   Don't send Updates with BGP Max-Prefix Overflow
-   Make sure `next-hop-self all` backward compatible with force
-   Fix as-path validation in "show bgp regexp"
-   Fix interface-based peers to override peergroups
-   Fix removing private AS numbers if local-as is used
-   Fix show bgp labeled_unicast
-   Add command to lookup prefixes in rpki table
-   Fix peer count in "show bgp ipv6 summary"
-   Add missing ipv6 only peer flag action
-   Fix address family output in "show bgp [ipv4|ipv6] neighbors"
-   Add missing checks for vpnv6 nexthops
-   Fix nexthop for ipv6 vpn case
- rip: Fix removal of passive interfaces
- ospf:
-   Fix json timer output
-   Fix milliseconds in json output
- bfd:
-   Fix source port according RFC 5881, Sec 4
-   Fix IPv6 link-local peer removal
-   Fix interface clean up when deleting interface
- pim: Fix interface clean up when deleting interface
- nhrp: Fix interface clean up when deleting interface
- lib:
-   Workaround to get FRR building with libyang 0.x and 1.x
-   Fix in priv handling
-   Make priv elevation thread-safe
- zebra:
-   Pseudowire event recovery
-   Fix race condition in label manager
-   Fix system routes selection and next-hop tracking
-   Set connected route metric based on devaddr metric
-   Display metric for connected routes
-   Add selected fib details to json output
-   Always use replace if installing new route
- watchfrr: Silently ignore declare failures (for backward compatibility)
- RPM packages: Switch to new init script

* Thu Feb 28 2019 Martin Winter <mwinter@opensourcerouting.org> - 7.0
- Added libyang dependency: New work for northbound interface based on libyang
- Fabricd: New Daemon based on https://datatracker.ietf.org/doc/draft-white-openfabric/
- various bug fixes and other enhancements

* Sun Oct  7 2018 Martin Winter <mwinter@opensourcerouting.org> - 6.0
- Staticd: New daemon responsible for management of static routes
- ISISd: Implement dst-src routing as per draft-ietf-isis-ipv6-dst-src-routing
- BFDd: new daemon for BFD (Bidrectional Forwarding Detection). Responsible
  for notifying link changes to make routing protocols converge faster.
- various bug fixes

* Thu Jul  5 2018 Martin Winter <mwinter@opensourcerouting.org> - 5.0.1
- Support Automake 1.16.1
- BGPd: Support for flowspec ICMP, DSCP, packet length, fragment and tcp flags
- BGPd: fix rpki validation for ipv6
- VRF: Workaround for kernel bug on Linux 4.14 and newer
- Zebra: Fix interface based routes from zebra not marked up
- Zebra: Fix large zebra memory usage when redistribute between protocols
- Zebra: Allow route-maps to match on source instance
- BGPd: Backport peer-attr overrides, peer-level enforce-first-as and filtered-routes fix
- BGPd: fix for crash during display of filtered-routes
- BGPd: Actually display labeled unicast routes received
- Label Manager: Fix to work correctly behind a label manager proxy

* Thu Jun  7 2018 Martin Winter <mwinter@opensourcerouting.org> - 5.0
- PIM: Add a Multicast Trace Command draft-ietf-idmr-traceroute-ipm-05
- IS-IS: Implement Three-Way Handshake as per RFC5303
- BGPD: Implement VPN-VRF route leaking per RFC4364.
- BGPD: Implement VRF with NETNS backend
- BGPD: Flowspec
- PBRD: Add a new Policy Based Routing Daemon

* Mon May 28 2018 Rafael Zalamena <rzalamena@opensourcerouting.org>
- Add BFDd support

* Sun May 20 2018 Martin Winter <mwinter@opensourcerouting.org>
- Fixed RPKI RPM build

* Sun Mar  4 2018 Martin Winter <mwinter@opensourcerouting.org>
- Add option to build with RPKI (default: disabled)

* Tue Feb 20 2018 Martin Winter <mwinter@opensourcerouting.org>
- Adapt to new documentation structure based on Sphinx

* Fri Oct 20 2017 Martin Winter <mwinter@opensourcerouting.org>
- Fix script location for watchfrr restart functions in daemon config
- Fix postun script to restart frr during upgrade

* Mon Jun  5 2017 Martin Winter <mwinter@opensourcerouting.org>
- added NHRP and EIGRP daemon

* Mon Apr 17 2017 Martin Winter <mwinter@opensourcerouting.org>
- new subpackage frr-pythontools with python 2.7 restart script
- remove PIMd from CentOS/RedHat 6 RPM packages (won't work - too old)
- converted to single frr init script (not per daemon) based on debian init script
- created systemd service file for systemd based systems (which uses init script)
- Various other RPM package fixes for FRR 2.0

* Fri Jan  6 2017 Martin Winter <mwinter@opensourcerouting.org>
- Renamed to frr for FRRouting fork of Quagga

* Thu Feb 11 2016 Paul Jakma <paul@jakma.org>
- remove with_ipv6 conditionals, always build v6
- Fix UTF-8 char in spec changelog
- remove quagga.pam.stack, long deprecated.

* Thu Oct 22 2015 Martin Winter <mwinter@opensourcerouting.org>
- Cleanup configure: remove --enable-ipv6 (default now), --enable-nssa,
    --enable-netlink
- Remove support for old fedora 4/5
- Fix for package nameing
- Fix Weekdays of previous changelogs (bogus dates)
- Add conditional logic to only build tex footnotes with supported texi2html
- Added pimd to files section and fix double listing of /var/lib*/quagga
- Numerous fixes to unify upstart/systemd startup into same spec file
- Only allow use of watchfrr for non-systemd systems. no need with systemd

* Fri Sep  4 2015 Paul Jakma <paul@jakma.org>
- buildreq updates
- add a default define for with_pimd

* Mon Sep 12 2005 Paul Jakma <paul@dishone.st>
- Steal some changes from Fedora spec file:
- Add with_rtadv variable
- Test for groups/users with getent before group/user adding
- Readline need not be an explicit prerequisite
- install-info delete should be postun, not preun

* Wed Jan 12 2005 Andrew J. Schorr <ajschorr@alumni.princeton.edu>
- on package upgrade, implement careful, phased restart logic
- use gcc -rdynamic flag when linking for better backtraces

* Wed Dec 22 2004 Andrew J. Schorr <ajschorr@alumni.princeton.edu>
- daemonv6_list should contain only IPv6 daemons

* Wed Dec 22 2004 Andrew J. Schorr <ajschorr@alumni.princeton.edu>
- watchfrr added
- on upgrade, all daemons should be condrestart'ed
- on removal, all daemons should be stopped

* Mon Nov 08 2004 Paul Jakma <paul@dishone.st>
- Use makeinfo --html to generate quagga.html

* Sun Nov 07 2004 Paul Jakma <paul@dishone.st>
- Fix with_ipv6 set to 0 build

* Sat Oct 23 2004 Paul Jakma <paul@dishone.st>
- Update to 0.97.2

* Sat Oct 23 2004 Andrew J. Schorr <aschorr@telemetry-investments.com>
- Make directories be owned by the packages concerned
- Update logrotate scripts to use correct path to killall and use pid files

* Fri Oct 08 2004 Paul Jakma <paul@dishone.st>
- Update to 0.97.0

* Wed Sep 15 2004 Paul Jakma <paul@dishone.st>
- build snmp support by default
- build irdp support
- build with shared libs
- devel subpackage for archives and headers

* Thu Jan 08 2004 Paul Jakma <paul@dishone.st>
- updated sysconfig files to specify local dir
- added ospf_dump.c crash quick fix patch
- added ospfd persistent interface configuration patch

* Tue Dec 30 2003 Paul Jakma <paul@dishone.st>
- sync to CVS
- integrate RH sysconfig patch to specify daemon options (RH)
- default to have vty listen only to 127.1 (RH)
- add user with fixed UID/GID (RH)
- create user with shell /sbin/nologin rather than /bin/false (RH)
- stop daemons on uninstall (RH)
- delete info file on preun, not postun to avoid deletion on upgrade. (RH)
- isisd added
- cleanup tasks carried out for every daemon

* Sun Nov 2 2003 Paul Jakma <paul@dishone.st>
- Fix -devel package to include all files
- Sync to 0.96.4

* Tue Aug 12 2003 Paul Jakma <paul@dishone.st>
- Renamed to Quagga
- Sync to Quagga release 0.96

* Thu Mar 20 2003 Paul Jakma <paul@dishone.st>
- zebra privileges support

* Tue Mar 18 2003 Paul Jakma <paul@dishone.st>
- Fix mem leak in 'show thread cpu'
- Ralph Keller's OSPF-API
- Amir: Fix configure.ac for net-snmp

* Sat Mar 1 2003 Paul Jakma <paul@dishone.st>
- ospfd IOS prefix to interface matching for 'network' statement
- temporary fix for PtP and IPv6
- sync to zebra.org CVS

* Mon Jan 20 2003 Paul Jakma <paul@dishone.st>
- update to latest cvs
- Yon's "show thread cpu" patch - 17217
- walk up tree - 17218
- ospfd NSSA fixes - 16681
- ospfd nsm fixes - 16824
- ospfd OLSA fixes and new feature - 16823
- KAME and ifindex fixes - 16525
- spec file changes to allow redhat files to be in tree

* Sat Dec 28 2002 Alexander Hoogerhuis <alexh@ihatent.com>
- Added conditionals for building with(out) IPv6, vtysh, RIP, BGP
- Fixed up some build requirements (patch)
- Added conditional build requirements for vtysh / snmp
- Added conditional to files for _bindir depending on vtysh

* Mon Nov 11 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add Greg Troxel's md5 buffer copy/dup fix
- add RIPv1 fix
- add Frank's multicast flag fix

* Wed Oct 09 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- timestamped crypt_seqnum patch
- oi->on_write_q fix

* Mon Sep 30 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add vtysh 'write-config (integrated|daemon)' patch
- always 'make rebuild' in vtysh/ to catch new commands

* Fri Sep 13 2002 Paul Jakma <paulj@alphyra.ie>
- update to 0.93b

* Wed Sep 11 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add "/sbin/ip route flush proto zebra" to zebra RH init on startup

* Sat Aug 24 2002 Paul Jakma <paulj@alphyra.ie>
- update to current CVS
- add OSPF point to multipoint patch
- add OSPF bugfixes
- add BGP hash optimisation patch

* Fri Jun 14 2002 Paul Jakma <paulj@alphyra.ie>
- update to 0.93-pre1 / CVS
- add link state detection support
- add generic PtP and RFC3021 support
- various bug fixes

* Thu Aug 09 2001 Elliot Lee <sopwith@redhat.com> 0.91a-6
- Fix bug #51336

* Wed Aug  1 2001 Trond Eivind Glomsrød <teg@redhat.com> 0.91a-5
- Use generic initscript strings instead of initscript specific
  ( "Starting foo: " -> "Starting $prog:" )

* Fri Jul 27 2001 Elliot Lee <sopwith@redhat.com> 0.91a-4
- Bump the release when rebuilding into the dist.

* Tue Feb  6 2001 Tim Powers <timp@redhat.com>
- built for Powertools

* Sun Feb  4 2001 Pekka Savola <pekkas@netcore.fi>
- Hacked up from PLD Linux 0.90-1, Mandrake 0.90-1mdk and one from zebra.org.
- Update to 0.91a
- Very heavy modifications to init.d/*, .spec, pam, i18n, logrotate, etc.
- Should be quite Red Hat'isque now.

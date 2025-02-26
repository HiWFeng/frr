.. _pimv6:

*****
PIMv6
*****

PIMv6 -- Protocol Independent Multicast for IPv6

*pim6d* supports pim-sm as well as MLD v1 and v2. PIMv6 is
vrf aware and can work within the context of vrf's in order to
do S,G mrouting.

.. _starting-and-stopping-pim6d:

Starting and Stopping pim6d
===========================

The default configuration file name of *pim6d*'s is :file:`pim6d.conf`. When
invoked *pim6d* searches directory |INSTALL_PREFIX_ETC|. If
:file:`pim6d.conf` is not there then next search current directory.

*pim6d* requires zebra for proper operation. Additionally *pim6d* depends on
routing properly setup and working in the network that it is working on.

::

   # zebra -d
   # pim6d -d


Please note that *zebra* must be invoked before *pim6d*.

To stop *pim6d* please use::

   kill `cat /var/run/pim6d.pid`

Certain signals have special meanings to *pim6d*.

+---------+---------------------------------------------------------------------+
| Signal  | Meaning                                                             |
+=========+=====================================================================+
| SIGUSR1 | Rotate the *pim6d* logfile                                          |
+---------+---------------------------------------------------------------------+
| SIGINT  | *pim6d* sweeps all installed PIM mroutes then terminates gracefully.|
| SIGTERM |                                                                     |
+---------+---------------------------------------------------------------------+

*pim6d* invocation options. Common options that can be specified
(:ref:`common-invocation-options`).

.. clicmd:: ipv6 pim rp X:X::X:X Y:Y::Y:Y/M

   In order to use pimv6, it is necessary to configure a RP for join messages to
   be sent to. Currently the only methodology to do this is via static rp
   commands. All routers in the pimv6 network must agree on these values. The
   first ipv6 address is the RP's address and the second value is the matching
   prefix of group ranges covered. This command is vrf aware, to configure for
   a vrf, enter the vrf submode.

.. clicmd:: ipv6 pim rp X:X::X:X prefix-list WORD

   This CLI helps in configuring RP address for a range of groups specified
   by the prefix-list.

.. clicmd:: ipv6 pim rp keep-alive-timer (1-65535)

   Modify the time out value for a S,G flow from 1-65535 seconds at RP.
   The normal keepalive period for the KAT(S,G) defaults to 210 seconds.
   However, at the RP, the keepalive period must be at least the
   Register_Suppression_Time, or the RP may time out the (S,G) state
   before the next Null-Register arrives. Thus, the KAT(S,G) is set to
   max(Keepalive_Period, RP_Keepalive_Period) when a Register-Stop is sent.
   If choosing a value below 31 seconds be aware that some hardware platforms
   cannot see data flowing in better than 30 second chunks. This command is
   vrf aware, to configure for a vrf, enter the vrf submode.

.. clicmd:: ipv6 pim spt-switchover infinity-and-beyond [prefix-list PLIST]

   On the last hop router if it is desired to not switch over to the SPT tree
   configure this command. Optional parameter prefix-list can be use to control
   which groups to switch or not switch. If a group is PERMIT as per the
   PLIST, then the SPT switchover does not happen for it and if it is DENY,
   then the SPT switchover happens.
   This command is vrf aware, to configure for a vrf,
   enter the vrf submode.

.. clicmd:: ipv6 pim join-prune-interval (1-65535)

   Modify the join/prune interval that pim uses to the new value. Time is
   specified in seconds. This command is vrf aware, to configure for a vrf,
   enter the vrf submode.  The default time is 60 seconds.  If you enter
   a value smaller than 60 seconds be aware that this can and will affect
   convergence at scale.

.. clicmd:: ipv6 pim keep-alive-timer (1-65535)

   Modify the time out value for a S,G flow from 1-65535 seconds. If choosing
   a value below 31 seconds be aware that some hardware platforms cannot see data
   flowing in better than 30 second chunks. This command is vrf aware, to
   configure for a vrf, enter the vrf submode.

.. clicmd:: ipv6 pim packets (1-255)

   When processing packets from a neighbor process the number of packets
   incoming at one time before moving on to the next task. The default value is
   3 packets.  This command is only useful at scale when you can possibly have
   a large number of pim control packets flowing. This command is vrf aware, to
   configure for a vrf, enter the vrf submode.

.. clicmd:: ipv6 pim register-suppress-time (1-65535)

   Modify the time that pim will register suppress a FHR will send register
   notifications to the kernel. This command is vrf aware, to configure for a
   vrf, enter the vrf submode.

.. clicmd:: ipv6 ssmpingd [X:X::X:X]

   Enable ipv6 ssmpingd configuration. A network level management tool
   to check whether one can receive multicast packets via SSM from host.
   The host target given to ssmping must run the ssmpingd daemon which listens
   for IPv4 and IPv6 unicast requests. When it receives one, it responds to a
   well known SSM multicast group which ssmping just have joined.

.. _pimv6-interface-configuration:

PIMv6 Interface Configuration
=============================

PIMv6 interface commands allow you to configure an interface as either a Receiver
or a interface that you would like to form pimv6 neighbors on. If the interface
is in a vrf, enter the interface command with the vrf keyword at the end.

.. clicmd:: ipv6 pim active-active

   Turn on pim active-active configuration for a Vxlan interface.  This
   command will not do anything if you do not have the underlying ability
   of a mlag implementation.

.. clicmd:: ipv6 pim drpriority (1-4294967295)

   Set the DR Priority for the interface. This command is useful to allow the
   user to influence what node becomes the DR for a lan segment.

.. clicmd:: ipv6 pim hello (1-65535) (1-65535)

   Set the pim hello and hold interval for a interface.

.. clicmd:: ipv6 pim

   Tell pim that we would like to use this interface to form pim neighbors
   over. Please note that this command does not enable the reception of MLD
   reports on the interface. Refer to the next ``ipv6 mld`` command for MLD
   management.

.. clicmd:: ipv6 pim use-source X:X::X:X

   If you have multiple addresses configured on a particular interface
   and would like pim to use a specific source address associated with
   that interface.

.. clicmd:: ipv6 mld

   Tell pim to receive MLD reports and Query on this interface. The default
   version is v2. This command is useful on a LHR.

.. clicmd:: ipv6 mld join X:X::X:X [Y:Y::Y:Y]

   Join multicast group or source-group on an interface.

.. clicmd:: ipv6 mld query-interval (1-65535)

   Set the MLD query interval that PIM will use.

.. clicmd:: ipv6 mld query-max-response-time (1-65535)

   Set the MLD query response timeout value. If an report is not returned in
   the specified time we will assume the S,G or \*,G has timed out.

.. clicmd:: ipv6 mld version (1-2)

   Set the MLD version used on this interface. The default value is 2.

.. clicmd:: ipv6 multicast boundary oil WORD

   Set a PIMv6 multicast boundary, based upon the WORD prefix-list. If a PIMv6
   join or MLD report is received on this interface and the Group is denied by
   the prefix-list, PIMv6 will ignore the join or report.

.. clicmd:: ipv6 mld last-member-query-count (1-255)

   Set the MLD last member query count. The default value is 2. 'no' form of
   this command is used to configure back to the default value.

.. clicmd:: ipv6 MLD last-member-query-interval (1-65535)

   Set the MLD last member query interval in deciseconds. The default value is
   10 deciseconds. 'no' form of this command is used to to configure back to the
   default value.

.. clicmd:: ipv6 mroute INTERFACE X:X::X:X [Y:Y::Y:Y]

   Set a static multicast route for a traffic coming on the current interface to
   be forwarded on the given interface if the traffic matches the group address
   and optionally the source address.

.. _show-pimv6-information:

Show PIMv6 Information
======================

All PIMv6 show commands are vrf aware and typically allow you to insert a
specified vrf command if information is desired about a specific vrf. If no
vrf is specified then the default vrf is assumed. Finally the special keyword
'all' allows you to look at all vrfs for the command. Naming a vrf 'all' will
cause great confusion.

.. clicmd:: show ipv6 pim [vrf NAME] group-type [json]

   Display SSM group ranges.

.. clicmd:: show ipv6 pim interface

   Display information about interfaces PIM is using.

.. clicmd:: show ipv6 pim [vrf NAME] join [X:X::X:X [X:X::X:X]] [json]
.. clicmd:: show ipv6 pim vrf all join [json]

   Display information about PIM joins received.  If one address is specified
   then we assume it is the Group we are interested in displaying data on.
   If the second address is specified then it is Source Group.

.. clicmd:: show ipv6 pim [vrf NAME] local-membership [json]

   Display information about PIM interface local-membership.

.. clicmd:: show ipv6 pim [vrf NAME] neighbor [detail|WORD] [json]
.. clicmd:: show ipv6 pim vrf all neighbor [detail|WORD] [json]

   Display information about PIM neighbors.

.. clicmd:: show ipv6 pim [vrf NAME] nexthop

   Display information about pim nexthops that are being used.

.. clicmd:: show ipv6 pim [vrf NAME] nexthop-lookup X:X::X:X X:X::X:X

   Display information about a S,G pair and how the RPF would be chosen. This
   is especially useful if there are ECMP's available from the RPF lookup.

.. clicmd:: show ipv6 pim [vrf NAME] rp-info [json]
.. clicmd:: show ipv6 pim vrf all rp-info [json]

   Display information about RP's that are configured on this router.

.. clicmd:: show ipv6 pim [vrf NAME] rpf [json]
.. clicmd:: show ipv6 pim vrf all rpf [json]

   Display information about currently being used S,G's and their RPF lookup
   information. Additionally display some statistics about what has been
   happening on the router.

.. clicmd:: show ipv6 pim [vrf NAME] secondary

   Display information about an interface and all the secondary addresses
   associated with it.

.. clicmd:: show ipv6 pim [vrf NAME] state [X:X::X:X [X:X::X:X]] [json]
.. clicmd:: show ipv6 pim vrf all state [X:X::X:X [X:X::X:X]] [json]

   Display information about known S,G's and incoming interface as well as the
   OIL and how they were chosen.

.. clicmd:: show ipv6 pim [vrf NAME] upstream [X:X::X:X [Y:Y::Y:Y]] [json]
.. clicmd:: show ipv6 pim vrf all upstream [json]

   Display upstream information about a S,G mroute.  Allow the user to
   specify sub Source and Groups that we are interested in.

.. clicmd:: show ipv6 pim [vrf NAME] upstream-join-desired [json]

   Display upstream information for S,G's and if we desire to
   join the multicast tree

.. clicmd:: show ipv6 pim [vrf NAME] upstream-rpf [json]

   Display upstream information for S,G's and the RPF data associated with them.

PIMv6 Debug Commands
====================

The debugging subsystem for PIMv6 behaves in accordance with how FRR handles
debugging. You can specify debugging at the enable CLI mode as well as the
configure CLI mode. If you specify debug commands in the configuration cli
mode, the debug commands can be persistent across restarts of the FRR pim6d if
the config was written out.


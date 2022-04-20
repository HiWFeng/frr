############### FRRouting (FRR) configure options #################
# with-feature options
%{!?with_pam:           %global  with_pam           0 }
%{!?with_ospfclient:    %global  with_ospfclient    1 }
%{!?with_ospfapi:       %global  with_ospfapi       1 }
%{!?with_irdp:          %global  with_irdp          1 }
%{!?with_rtadv:         %global  with_rtadv         1 }
%{!?with_ldpd:          %global  with_ldpd          1 }
%{!?with_nhrpd:         %global  with_nhrpd         1 }
%{!?with_eigrp:         %global  with_eigrpd        1 }
%{!?with_shared:        %global  with_shared        1 }
%{!?with_multipath:     %global  with_multipath     256 }
%{!?frr_user:           %global  frr_user           frr }
%{!?vty_group:          %global  vty_group          frrvty }
%{!?with_fpm:           %global  with_fpm           0 }
%{!?with_watchfrr:      %global  with_watchfrr      1 }
%{!?with_bgp_vnc:       %global  with_bgp_vnc       0 }
%{!?with_pimd:          %global  with_pimd          1 }
%{!?with_rpki:          %global  with_rpki          0 }

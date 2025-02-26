/*
 * PIM for IPv6 FRR
 * Copyright (C) 2022  Vmware, Inc.
 *		       Mobashshera Rasool <mrasool@vmware.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <zebra.h>

#include "lib/json.h"
#include "command.h"
#include "if.h"
#include "prefix.h"
#include "zclient.h"
#include "plist.h"
#include "hash.h"
#include "nexthop.h"
#include "vrf.h"
#include "ferr.h"
#include "lib/srcdest_table.h"
#include "lib/linklist.h"

#include "pimd.h"
#include "pim_vty.h"
#include "lib/northbound_cli.h"
#include "pim_errors.h"
#include "pim_nb.h"
#include "pim_mroute.h"
#include "pim_cmd.h"
#include "pim6_cmd.h"
#include "pim_cmd_common.h"
#include "pim_time.h"
#include "pim_zebra.h"
#include "pim_zlookup.h"
#include "pim_iface.h"
#include "pim_macro.h"
#include "pim_neighbor.h"
#include "pim_nht.h"
#include "pim_sock.h"
#include "pim_ssm.h"
#include "pim_addr.h"

/**
 * Get current node VRF name.
 *
 * NOTE:
 * In case of failure it will print error message to user.
 *
 * \returns name or NULL if failed to get VRF.
 */
const char *pim_cli_get_vrf_name(struct vty *vty)
{
	const struct lyd_node *vrf_node;

	/* Not inside any VRF context. */
	if (vty->xpath_index == 0)
		return VRF_DEFAULT_NAME;

	vrf_node = yang_dnode_get(vty->candidate_config->dnode, VTY_CURR_XPATH);
	if (vrf_node == NULL) {
		vty_out(vty, "%% Failed to get vrf dnode in configuration\n");
		return NULL;
	}

	return yang_dnode_get_string(vrf_node, "./name");
}

int pim_process_join_prune_cmd(struct vty *vty, const char *jpi_str)
{
	char xpath[XPATH_MAXLEN];

	snprintf(xpath, sizeof(xpath), FRR_PIM_ROUTER_XPATH,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(xpath, "/join-prune-interval", sizeof(xpath));

	nb_cli_enqueue_change(vty, xpath, NB_OP_MODIFY, jpi_str);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_join_prune_cmd(struct vty *vty)
{
	char xpath[XPATH_MAXLEN];

	snprintf(xpath, sizeof(xpath), FRR_PIM_ROUTER_XPATH,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(xpath, "/join-prune-interval", sizeof(xpath));

	nb_cli_enqueue_change(vty, xpath, NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_spt_switchover_infinity_cmd(struct vty *vty)
{
	const char *vrfname;
	char spt_plist_xpath[XPATH_MAXLEN];
	char spt_action_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(spt_plist_xpath, sizeof(spt_plist_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(spt_plist_xpath, "/spt-switchover/spt-infinity-prefix-list",
		sizeof(spt_plist_xpath));

	snprintf(spt_action_xpath, sizeof(spt_action_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(spt_action_xpath, "/spt-switchover/spt-action",
		sizeof(spt_action_xpath));

	if (yang_dnode_exists(vty->candidate_config->dnode, spt_plist_xpath))
		nb_cli_enqueue_change(vty, spt_plist_xpath, NB_OP_DESTROY,
				      NULL);
	nb_cli_enqueue_change(vty, spt_action_xpath, NB_OP_MODIFY,
			      "PIM_SPT_INFINITY");

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_spt_switchover_prefixlist_cmd(struct vty *vty,
					      const char *plist)
{
	const char *vrfname;
	char spt_plist_xpath[XPATH_MAXLEN];
	char spt_action_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(spt_plist_xpath, sizeof(spt_plist_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(spt_plist_xpath, "/spt-switchover/spt-infinity-prefix-list",
		sizeof(spt_plist_xpath));

	snprintf(spt_action_xpath, sizeof(spt_action_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(spt_action_xpath, "/spt-switchover/spt-action",
		sizeof(spt_action_xpath));

	nb_cli_enqueue_change(vty, spt_action_xpath, NB_OP_MODIFY,
			      "PIM_SPT_INFINITY");
	nb_cli_enqueue_change(vty, spt_plist_xpath, NB_OP_MODIFY,
			      plist);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_spt_switchover_cmd(struct vty *vty)
{
	const char *vrfname;
	char spt_plist_xpath[XPATH_MAXLEN];
	char spt_action_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(spt_plist_xpath, sizeof(spt_plist_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(spt_plist_xpath, "/spt-switchover/spt-infinity-prefix-list",
		sizeof(spt_plist_xpath));

	snprintf(spt_action_xpath, sizeof(spt_action_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(spt_action_xpath, "/spt-switchover/spt-action",
		sizeof(spt_action_xpath));

	nb_cli_enqueue_change(vty, spt_plist_xpath, NB_OP_DESTROY, NULL);
	nb_cli_enqueue_change(vty, spt_action_xpath, NB_OP_MODIFY,
			      "PIM_SPT_IMMEDIATE");

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_pim_packet_cmd(struct vty *vty, const char *packet)
{
	char xpath[XPATH_MAXLEN];

	snprintf(xpath, sizeof(xpath), FRR_PIM_ROUTER_XPATH,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(xpath, "/packets", sizeof(xpath));

	nb_cli_enqueue_change(vty, xpath, NB_OP_MODIFY, packet);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_pim_packet_cmd(struct vty *vty)
{
	char xpath[XPATH_MAXLEN];

	snprintf(xpath, sizeof(xpath), FRR_PIM_ROUTER_XPATH,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(xpath, "/packets", sizeof(xpath));

	nb_cli_enqueue_change(vty, xpath, NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_keepalivetimer_cmd(struct vty *vty, const char *kat)
{
	const char *vrfname;
	char ka_timer_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(ka_timer_xpath, sizeof(ka_timer_xpath), FRR_PIM_VRF_XPATH,
		 "frr-pim:pimd", "pim", vrfname, FRR_PIM_AF_XPATH_VAL);
	strlcat(ka_timer_xpath, "/keep-alive-timer", sizeof(ka_timer_xpath));

	nb_cli_enqueue_change(vty, ka_timer_xpath, NB_OP_MODIFY,
			      kat);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_keepalivetimer_cmd(struct vty *vty)
{
	const char *vrfname;
	char ka_timer_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(ka_timer_xpath, sizeof(ka_timer_xpath), FRR_PIM_VRF_XPATH,
		 "frr-pim:pimd", "pim", vrfname, FRR_PIM_AF_XPATH_VAL);
	strlcat(ka_timer_xpath, "/keep-alive-timer", sizeof(ka_timer_xpath));

	nb_cli_enqueue_change(vty, ka_timer_xpath, NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_rp_kat_cmd(struct vty *vty, const char *rpkat)
{
	const char *vrfname;
	char rp_ka_timer_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(rp_ka_timer_xpath, sizeof(rp_ka_timer_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(rp_ka_timer_xpath, "/rp-keep-alive-timer",
		sizeof(rp_ka_timer_xpath));

	nb_cli_enqueue_change(vty, rp_ka_timer_xpath, NB_OP_MODIFY,
			      rpkat);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_rp_kat_cmd(struct vty *vty)
{
	const char *vrfname;
	char rp_ka_timer[6];
	char rp_ka_timer_xpath[XPATH_MAXLEN];
	uint v;
	char rs_timer_xpath[XPATH_MAXLEN];

	snprintf(rs_timer_xpath, sizeof(rs_timer_xpath),
		 FRR_PIM_ROUTER_XPATH, FRR_PIM_AF_XPATH_VAL);
	strlcat(rs_timer_xpath, "/register-suppress-time",
		sizeof(rs_timer_xpath));

	/* RFC4601 */
	v = yang_dnode_get_uint16(vty->candidate_config->dnode,
				  rs_timer_xpath);
	v = 3 * v + PIM_REGISTER_PROBE_TIME_DEFAULT;
	if (v > UINT16_MAX)
		v = UINT16_MAX;
	snprintf(rp_ka_timer, sizeof(rp_ka_timer), "%u", v);

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(rp_ka_timer_xpath, sizeof(rp_ka_timer_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(rp_ka_timer_xpath, "/rp-keep-alive-timer",
		sizeof(rp_ka_timer_xpath));

	nb_cli_enqueue_change(vty, rp_ka_timer_xpath, NB_OP_MODIFY,
			      rp_ka_timer);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_register_suppress_cmd(struct vty *vty, const char *rst)
{
	char xpath[XPATH_MAXLEN];

	snprintf(xpath, sizeof(xpath), FRR_PIM_ROUTER_XPATH,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(xpath, "/register-suppress-time", sizeof(xpath));

	nb_cli_enqueue_change(vty, xpath, NB_OP_MODIFY, rst);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_register_suppress_cmd(struct vty *vty)
{
	char xpath[XPATH_MAXLEN];

	snprintf(xpath, sizeof(xpath), FRR_PIM_ROUTER_XPATH,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(xpath, "/register-suppress-time", sizeof(xpath));

	nb_cli_enqueue_change(vty, xpath, NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_ip_pim_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./pim-enable", NB_OP_MODIFY, "true");

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_no_ip_pim_cmd(struct vty *vty)
{
	const struct lyd_node *mld_enable_dnode;
	char mld_if_xpath[XPATH_MAXLEN];

	int printed =
		snprintf(mld_if_xpath, sizeof(mld_if_xpath),
			 "%s/frr-gmp:gmp/address-family[address-family='%s']",
			 VTY_CURR_XPATH, FRR_PIM_AF_XPATH_VAL);

	if (printed >= (int)(sizeof(mld_if_xpath))) {
		vty_out(vty, "Xpath too long (%d > %u)", printed + 1,
			XPATH_MAXLEN);
		return CMD_WARNING_CONFIG_FAILED;
	}

	mld_enable_dnode = yang_dnode_getf(vty->candidate_config->dnode,
					   FRR_GMP_ENABLE_XPATH, VTY_CURR_XPATH,
					   FRR_PIM_AF_XPATH_VAL);

	if (!mld_enable_dnode) {
		nb_cli_enqueue_change(vty, mld_if_xpath, NB_OP_DESTROY, NULL);
		nb_cli_enqueue_change(vty, ".", NB_OP_DESTROY, NULL);
	} else {
		if (!yang_dnode_get_bool(mld_enable_dnode, ".")) {
			nb_cli_enqueue_change(vty, mld_if_xpath, NB_OP_DESTROY,
					      NULL);
			nb_cli_enqueue_change(vty, ".", NB_OP_DESTROY, NULL);
		} else
			nb_cli_enqueue_change(vty, "./pim-enable", NB_OP_MODIFY,
					      "false");
	}

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_ip_pim_drprio_cmd(struct vty *vty, const char *drpriority_str)
{
	nb_cli_enqueue_change(vty, "./dr-priority", NB_OP_MODIFY,
			      drpriority_str);

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_no_ip_pim_drprio_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./dr-priority", NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_ip_pim_hello_cmd(struct vty *vty, const char *hello_str,
				 const char *hold_str)
{
	const struct lyd_node *mld_enable_dnode;

	mld_enable_dnode = yang_dnode_getf(vty->candidate_config->dnode,
					   FRR_GMP_ENABLE_XPATH, VTY_CURR_XPATH,
					   FRR_PIM_AF_XPATH_VAL);

	if (!mld_enable_dnode) {
		nb_cli_enqueue_change(vty, "./pim-enable", NB_OP_MODIFY,
				      "true");
	} else {
		if (!yang_dnode_get_bool(mld_enable_dnode, "."))
			nb_cli_enqueue_change(vty, "./pim-enable", NB_OP_MODIFY,
					      "true");
	}

	nb_cli_enqueue_change(vty, "./hello-interval", NB_OP_MODIFY, hello_str);

	if (hold_str)
		nb_cli_enqueue_change(vty, "./hello-holdtime", NB_OP_MODIFY,
				      hold_str);

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_no_ip_pim_hello_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./hello-interval", NB_OP_DESTROY, NULL);
	nb_cli_enqueue_change(vty, "./hello-holdtime", NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_ip_pim_activeactive_cmd(struct vty *vty, const char *no)
{
	if (no)
		nb_cli_enqueue_change(vty, "./active-active", NB_OP_MODIFY,
				      "false");
	else {
		nb_cli_enqueue_change(vty, "./pim-enable", NB_OP_MODIFY,
				      "true");

		nb_cli_enqueue_change(vty, "./active-active", NB_OP_MODIFY,
				      "true");
	}

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_ip_pim_boundary_oil_cmd(struct vty *vty, const char *oil)
{
	nb_cli_enqueue_change(vty, "./multicast-boundary-oil", NB_OP_MODIFY,
			      oil);

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_no_ip_pim_boundary_oil_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./multicast-boundary-oil", NB_OP_DESTROY,
			      NULL);

	return nb_cli_apply_changes(vty, FRR_PIM_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int pim_process_ip_mroute_cmd(struct vty *vty, const char *interface,
			      const char *group_str, const char *source_str)
{
	nb_cli_enqueue_change(vty, "./oif", NB_OP_MODIFY, interface);

	if (!source_str) {
		char buf[SRCDEST2STR_BUFFER];

		inet_ntop(AF_INET6, &in6addr_any, buf, sizeof(buf));
		return nb_cli_apply_changes(vty, FRR_PIM_MROUTE_XPATH,
					    FRR_PIM_AF_XPATH_VAL, buf,
					    group_str);
	}

	return nb_cli_apply_changes(vty, FRR_PIM_MROUTE_XPATH,
				    FRR_PIM_AF_XPATH_VAL, source_str,
				    group_str);
}

int pim_process_no_ip_mroute_cmd(struct vty *vty, const char *interface,
				 const char *group_str, const char *source_str)
{
	nb_cli_enqueue_change(vty, ".", NB_OP_DESTROY, NULL);

	if (!source_str) {
		char buf[SRCDEST2STR_BUFFER];

		inet_ntop(AF_INET6, &in6addr_any, buf, sizeof(buf));
		return nb_cli_apply_changes(vty, FRR_PIM_MROUTE_XPATH,
					    FRR_PIM_AF_XPATH_VAL, buf,
					    group_str);
	}

	return nb_cli_apply_changes(vty, FRR_PIM_MROUTE_XPATH,
				    FRR_PIM_AF_XPATH_VAL, source_str,
				    group_str);
}

int pim_process_rp_cmd(struct vty *vty, const char *rp_str,
		       const char *group_str)
{
	const char *vrfname;
	char rp_group_xpath[XPATH_MAXLEN];
	int result = 0;
	struct prefix group;
	pim_addr rp_addr;

	result = str2prefix(group_str, &group);
	if (result) {
		struct prefix temp;

		prefix_copy(&temp, &group);
		apply_mask(&temp);
		if (!prefix_same(&group, &temp)) {
			vty_out(vty, "%% Inconsistent address and mask: %s\n",
				group_str);
			return CMD_WARNING_CONFIG_FAILED;
		}
	}

	if (!result) {
		vty_out(vty, "%% Bad group address specified: %s\n", group_str);
		return CMD_WARNING_CONFIG_FAILED;
	}

	result = inet_pton(PIM_AF, rp_str, &rp_addr);
	if (result <= 0) {
		vty_out(vty, "%% Bad RP address specified: %s\n", rp_str);
		return CMD_WARNING_CONFIG_FAILED;
	}

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(rp_group_xpath, sizeof(rp_group_xpath),
		 FRR_PIM_STATIC_RP_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL, rp_str);
	strlcat(rp_group_xpath, "/group-list", sizeof(rp_group_xpath));

	nb_cli_enqueue_change(vty, rp_group_xpath, NB_OP_CREATE, group_str);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_rp_cmd(struct vty *vty, const char *rp_str,
			  const char *group_str)
{
	char group_list_xpath[XPATH_MAXLEN];
	char group_xpath[XPATH_MAXLEN];
	char rp_xpath[XPATH_MAXLEN];
	int printed;
	const char *vrfname;
	const struct lyd_node *group_dnode;

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(rp_xpath, sizeof(rp_xpath), FRR_PIM_STATIC_RP_XPATH,
		 "frr-pim:pimd", "pim", vrfname, FRR_PIM_AF_XPATH_VAL, rp_str);

	printed = snprintf(group_list_xpath, sizeof(group_list_xpath),
			   "%s/group-list", rp_xpath);

	if (printed >= (int)(sizeof(group_list_xpath))) {
		vty_out(vty, "Xpath too long (%d > %u)", printed + 1,
			XPATH_MAXLEN);
		return CMD_WARNING_CONFIG_FAILED;
	}

	printed = snprintf(group_xpath, sizeof(group_xpath), "%s[.='%s']",
			   group_list_xpath, group_str);

	if (printed >= (int)(sizeof(group_xpath))) {
		vty_out(vty, "Xpath too long (%d > %u)", printed + 1,
			XPATH_MAXLEN);
		return CMD_WARNING_CONFIG_FAILED;
	}

	group_dnode = yang_dnode_get(vty->candidate_config->dnode, group_xpath);
	if (!group_dnode) {
		vty_out(vty, "%% Unable to find specified RP\n");
		return NB_OK;
	}

	if (yang_is_last_list_dnode(group_dnode))
		nb_cli_enqueue_change(vty, rp_xpath, NB_OP_DESTROY, NULL);
	else
		nb_cli_enqueue_change(vty, group_list_xpath, NB_OP_DESTROY,
				      group_str);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_rp_plist_cmd(struct vty *vty, const char *rp_str,
			     const char *prefix_list)
{
	const char *vrfname;
	char rp_plist_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(rp_plist_xpath, sizeof(rp_plist_xpath),
		 FRR_PIM_STATIC_RP_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL, rp_str);
	strlcat(rp_plist_xpath, "/prefix-list", sizeof(rp_plist_xpath));

	nb_cli_enqueue_change(vty, rp_plist_xpath, NB_OP_MODIFY, prefix_list);

	return nb_cli_apply_changes(vty, NULL);
}

int pim_process_no_rp_plist_cmd(struct vty *vty, const char *rp_str,
				const char *prefix_list)
{
	char rp_xpath[XPATH_MAXLEN];
	char plist_xpath[XPATH_MAXLEN];
	const char *vrfname;
	const struct lyd_node *plist_dnode;
	const char *plist;

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(rp_xpath, sizeof(rp_xpath), FRR_PIM_STATIC_RP_XPATH,
		 "frr-pim:pimd", "pim", vrfname, FRR_PIM_AF_XPATH_VAL, rp_str);

	snprintf(plist_xpath, sizeof(plist_xpath), FRR_PIM_STATIC_RP_XPATH,
		 "frr-pim:pimd", "pim", vrfname, FRR_PIM_AF_XPATH_VAL, rp_str);
	strlcat(plist_xpath, "/prefix-list", sizeof(plist_xpath));

	plist_dnode = yang_dnode_get(vty->candidate_config->dnode, plist_xpath);
	if (!plist_dnode) {
		vty_out(vty, "%% Unable to find specified RP\n");
		return NB_OK;
	}

	plist = yang_dnode_get_string(plist_dnode, plist_xpath);
	if (strcmp(prefix_list, plist)) {
		vty_out(vty, "%% Unable to find specified RP\n");
		return NB_OK;
	}

	nb_cli_enqueue_change(vty, rp_xpath, NB_OP_DESTROY, NULL);

	return nb_cli_apply_changes(vty, NULL);
}

bool pim_sgaddr_match(pim_sgaddr item, pim_sgaddr match)
{
	return (pim_addr_is_any(match.grp) ||
		!pim_addr_cmp(match.grp, item.grp)) &&
	       (pim_addr_is_any(match.src) ||
		!pim_addr_cmp(match.src, item.src));
}

void json_object_pim_ifp_add(struct json_object *json, struct interface *ifp)
{
	struct pim_interface *pim_ifp;

	pim_ifp = ifp->info;
	json_object_string_add(json, "name", ifp->name);
	json_object_string_add(json, "state", if_is_up(ifp) ? "up" : "down");
	json_object_string_addf(json, "address", "%pPA",
				&pim_ifp->primary_address);
	json_object_int_add(json, "index", ifp->ifindex);

	if (if_is_multicast(ifp))
		json_object_boolean_true_add(json, "flagMulticast");

	if (if_is_broadcast(ifp))
		json_object_boolean_true_add(json, "flagBroadcast");

	if (ifp->flags & IFF_ALLMULTI)
		json_object_boolean_true_add(json, "flagAllMulticast");

	if (ifp->flags & IFF_PROMISC)
		json_object_boolean_true_add(json, "flagPromiscuous");

	if (PIM_IF_IS_DELETED(ifp))
		json_object_boolean_true_add(json, "flagDeleted");

	if (pim_if_lan_delay_enabled(ifp))
		json_object_boolean_true_add(json, "lanDelayEnabled");
}

void pim_print_ifp_flags(struct vty *vty, struct interface *ifp)
{
	vty_out(vty, "Flags\n");
	vty_out(vty, "-----\n");
	vty_out(vty, "All Multicast   : %s\n",
		(ifp->flags & IFF_ALLMULTI) ? "yes" : "no");
	vty_out(vty, "Broadcast       : %s\n",
		if_is_broadcast(ifp) ? "yes" : "no");
	vty_out(vty, "Deleted         : %s\n",
		PIM_IF_IS_DELETED(ifp) ? "yes" : "no");
	vty_out(vty, "Interface Index : %d\n", ifp->ifindex);
	vty_out(vty, "Multicast       : %s\n",
		if_is_multicast(ifp) ? "yes" : "no");
	vty_out(vty, "Promiscuous     : %s\n",
		(ifp->flags & IFF_PROMISC) ? "yes" : "no");
	vty_out(vty, "\n");
	vty_out(vty, "\n");
}

void json_object_pim_upstream_add(json_object *json, struct pim_upstream *up)
{
	json_object_boolean_add(
		json, "drJoinDesired",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_DR_JOIN_DESIRED));
	json_object_boolean_add(
		json, "drJoinDesiredUpdated",
		CHECK_FLAG(up->flags,
			   PIM_UPSTREAM_FLAG_MASK_DR_JOIN_DESIRED_UPDATED));
	json_object_boolean_add(
		json, "firstHopRouter",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_FHR));
	json_object_boolean_add(
		json, "sourceIgmp",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_IGMP));
	json_object_boolean_add(
		json, "sourcePim",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_PIM));
	json_object_boolean_add(
		json, "sourceStream",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_STREAM));
	/* XXX: need to print ths flag in the plain text display as well */
	json_object_boolean_add(
		json, "sourceMsdp",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_MSDP));
	json_object_boolean_add(
		json, "sendSGRptPrune",
		CHECK_FLAG(up->flags,
			   PIM_UPSTREAM_FLAG_MASK_SEND_SG_RPT_PRUNE));
	json_object_boolean_add(
		json, "lastHopRouter",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_LHR));
	json_object_boolean_add(
		json, "disableKATExpiry",
		CHECK_FLAG(up->flags,
			   PIM_UPSTREAM_FLAG_MASK_DISABLE_KAT_EXPIRY));
	json_object_boolean_add(
		json, "staticIncomingInterface",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_STATIC_IIF));
	json_object_boolean_add(
		json, "allowIncomingInterfaceinOil",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_ALLOW_IIF_IN_OIL));
	json_object_boolean_add(
		json, "noPimRegistrationData",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_NO_PIMREG_DATA));
	json_object_boolean_add(
		json, "forcePimRegistration",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_FORCE_PIMREG));
	json_object_boolean_add(
		json, "sourceVxlanOrigination",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_VXLAN_ORIG));
	json_object_boolean_add(
		json, "sourceVxlanTermination",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_SRC_VXLAN_TERM));
	json_object_boolean_add(
		json, "mlagVxlan",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_MLAG_VXLAN));
	json_object_boolean_add(
		json, "mlagNonDesignatedForwarder",
		CHECK_FLAG(up->flags, PIM_UPSTREAM_FLAG_MASK_MLAG_NON_DF));
}

static const char *
pim_upstream_state2brief_str(enum pim_upstream_state join_state,
			     char *state_str, size_t state_str_len)
{
	switch (join_state) {
	case PIM_UPSTREAM_NOTJOINED:
		strlcpy(state_str, "NotJ", state_str_len);
		break;
	case PIM_UPSTREAM_JOINED:
		strlcpy(state_str, "J", state_str_len);
		break;
	default:
		strlcpy(state_str, "Unk", state_str_len);
	}
	return state_str;
}

static const char *pim_reg_state2brief_str(enum pim_reg_state reg_state,
					   char *state_str,
					   size_t state_str_len)
{
	switch (reg_state) {
	case PIM_REG_NOINFO:
		strlcpy(state_str, "RegNI", state_str_len);
		break;
	case PIM_REG_JOIN:
		strlcpy(state_str, "RegJ", state_str_len);
		break;
	case PIM_REG_JOIN_PENDING:
	case PIM_REG_PRUNE:
		strlcpy(state_str, "RegP", state_str_len);
		break;
	}
	return state_str;
}

void pim_show_rpf_refresh_stats(struct vty *vty, struct pim_instance *pim,
				time_t now, json_object *json)
{
	char refresh_uptime[10];

	pim_time_uptime_begin(refresh_uptime, sizeof(refresh_uptime), now,
			      pim->rpf_cache_refresh_last);

	if (json) {
		json_object_int_add(json, "rpfCacheRefreshDelayMsecs",
				    router->rpf_cache_refresh_delay_msec);
		json_object_int_add(
			json, "rpfCacheRefreshTimer",
			pim_time_timer_remain_msec(pim->rpf_cache_refresher));
		json_object_int_add(json, "rpfCacheRefreshRequests",
				    pim->rpf_cache_refresh_requests);
		json_object_int_add(json, "rpfCacheRefreshEvents",
				    pim->rpf_cache_refresh_events);
		json_object_string_add(json, "rpfCacheRefreshLast",
				       refresh_uptime);
		json_object_int_add(json, "nexthopLookups",
				    pim->nexthop_lookups);
		json_object_int_add(json, "nexthopLookupsAvoided",
				    pim->nexthop_lookups_avoided);
	} else {
		vty_out(vty,
			"RPF Cache Refresh Delay:    %ld msecs\n"
			"RPF Cache Refresh Timer:    %ld msecs\n"
			"RPF Cache Refresh Requests: %lld\n"
			"RPF Cache Refresh Events:   %lld\n"
			"RPF Cache Refresh Last:     %s\n"
			"Nexthop Lookups:            %lld\n"
			"Nexthop Lookups Avoided:    %lld\n",
			router->rpf_cache_refresh_delay_msec,
			pim_time_timer_remain_msec(pim->rpf_cache_refresher),
			(long long)pim->rpf_cache_refresh_requests,
			(long long)pim->rpf_cache_refresh_events,
			refresh_uptime, (long long)pim->nexthop_lookups,
			(long long)pim->nexthop_lookups_avoided);
	}
}

void pim_show_rpf(struct pim_instance *pim, struct vty *vty, json_object *json)
{
	struct pim_upstream *up;
	time_t now = pim_time_monotonic_sec();
	json_object *json_group = NULL;
	json_object *json_row = NULL;

	pim_show_rpf_refresh_stats(vty, pim, now, json);

	if (!json) {
		vty_out(vty, "\n");
		vty_out(vty,
			"Source          Group           RpfIface         RpfAddress      RibNextHop      Metric Pref\n");
	}

	frr_each (rb_pim_upstream, &pim->upstream_head, up) {
		char rpf_addr_str[PREFIX_STRLEN];
		char rib_nexthop_str[PREFIX_STRLEN];
		const char *rpf_ifname;
		struct pim_rpf *rpf = &up->rpf;

		pim_addr_dump("<rpf?>", &rpf->rpf_addr, rpf_addr_str,
			      sizeof(rpf_addr_str));
		pim_addr_dump("<nexthop?>",
			      &rpf->source_nexthop.mrib_nexthop_addr,
			      rib_nexthop_str, sizeof(rib_nexthop_str));

		rpf_ifname =
			rpf->source_nexthop.interface ? rpf->source_nexthop
								.interface->name
						      : "<ifname?>";

		if (json) {
			char grp_str[PIM_ADDRSTRLEN];
			char src_str[PIM_ADDRSTRLEN];

			snprintfrr(grp_str, sizeof(grp_str), "%pPAs",
				   &up->sg.grp);
			snprintfrr(src_str, sizeof(src_str), "%pPAs",
				   &up->sg.src);

			json_object_object_get_ex(json, grp_str, &json_group);

			if (!json_group) {
				json_group = json_object_new_object();
				json_object_object_add(json, grp_str,
						       json_group);
			}

			json_row = json_object_new_object();
			json_object_string_add(json_row, "source", src_str);
			json_object_string_add(json_row, "group", grp_str);
			json_object_string_add(json_row, "rpfInterface",
					       rpf_ifname);
			json_object_string_add(json_row, "rpfAddress",
					       rpf_addr_str);
			json_object_string_add(json_row, "ribNexthop",
					       rib_nexthop_str);
			json_object_int_add(
				json_row, "routeMetric",
				rpf->source_nexthop.mrib_route_metric);
			json_object_int_add(
				json_row, "routePreference",
				rpf->source_nexthop.mrib_metric_preference);
			json_object_object_add(json_group, src_str, json_row);

		} else {
			vty_out(vty,
				"%-15pPAs %-15pPAs %-16s %-15s %-15s %6d %4d\n",
				&up->sg.src, &up->sg.grp, rpf_ifname,
				rpf_addr_str, rib_nexthop_str,
				rpf->source_nexthop.mrib_route_metric,
				rpf->source_nexthop.mrib_metric_preference);
		}
	}
}

void pim_show_neighbors_secondary(struct pim_instance *pim, struct vty *vty)
{
	struct interface *ifp;

	vty_out(vty,
		"Interface        Address         Neighbor        Secondary      \n");

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		struct pim_interface *pim_ifp;
		pim_addr ifaddr;
		struct listnode *neighnode;
		struct pim_neighbor *neigh;

		pim_ifp = ifp->info;

		if (!pim_ifp)
			continue;

		if (pim_ifp->pim_sock_fd < 0)
			continue;

		ifaddr = pim_ifp->primary_address;

		for (ALL_LIST_ELEMENTS_RO(pim_ifp->pim_neighbor_list, neighnode,
					  neigh)) {
			struct listnode *prefix_node;
			struct prefix *p;

			if (!neigh->prefix_list)
				continue;

			for (ALL_LIST_ELEMENTS_RO(neigh->prefix_list,
						  prefix_node, p))
				vty_out(vty,
					"%-16s %-15pPAs %-15pPAs %-15pFX\n",
					ifp->name, &ifaddr, &neigh->source_addr,
					p);
		}
	}
}

void pim_show_state(struct pim_instance *pim, struct vty *vty,
		    const char *src_or_group, const char *group,
		    json_object *json)
{
	struct channel_oil *c_oil;
	json_object *json_group = NULL;
	json_object *json_ifp_in = NULL;
	json_object *json_ifp_out = NULL;
	json_object *json_source = NULL;
	time_t now;
	int first_oif;

	now = pim_time_monotonic_sec();

	if (!json) {
		vty_out(vty,
			"Codes: J -> Pim Join, I -> IGMP Report, S -> Source, * -> Inherited from (*,G), V -> VxLAN, M -> Muted");
		vty_out(vty,
			"\nActive Source           Group            RPT  IIF               OIL\n");
	}

	frr_each (rb_pim_oil, &pim->channel_oil_head, c_oil) {
		char src_str[PIM_ADDRSTRLEN];
		char grp_str[PIM_ADDRSTRLEN];
		char in_ifname[INTERFACE_NAMSIZ + 1];
		char out_ifname[INTERFACE_NAMSIZ + 1];
		int oif_vif_index;
		struct interface *ifp_in;
		bool isRpt;

		first_oif = 1;

		if ((c_oil->up &&
		     PIM_UPSTREAM_FLAG_TEST_USE_RPT(c_oil->up->flags)) ||
		    pim_addr_is_any(*oil_origin(c_oil)))
			isRpt = true;
		else
			isRpt = false;

		snprintfrr(grp_str, sizeof(grp_str), "%pPAs",
			   oil_mcastgrp(c_oil));
		snprintfrr(src_str, sizeof(src_str), "%pPAs",
			   oil_origin(c_oil));
		ifp_in = pim_if_find_by_vif_index(pim, *oil_parent(c_oil));

		if (ifp_in)
			strlcpy(in_ifname, ifp_in->name, sizeof(in_ifname));
		else
			strlcpy(in_ifname, "<iif?>", sizeof(in_ifname));

		if (src_or_group) {
			if (strcmp(src_or_group, src_str) &&
			    strcmp(src_or_group, grp_str))
				continue;

			if (group && strcmp(group, grp_str))
				continue;
		}

		if (json) {

			/* Find the group, create it if it doesn't exist */
			json_object_object_get_ex(json, grp_str, &json_group);

			if (!json_group) {
				json_group = json_object_new_object();
				json_object_object_add(json, grp_str,
						       json_group);
			}

			/* Find the source nested under the group, create it if
			 * it doesn't exist
			 */
			json_object_object_get_ex(json_group, src_str,
						  &json_source);

			if (!json_source) {
				json_source = json_object_new_object();
				json_object_object_add(json_group, src_str,
						       json_source);
			}

			/* Find the inbound interface nested under the source,
			 * create it if it doesn't exist
			 */
			json_object_object_get_ex(json_source, in_ifname,
						  &json_ifp_in);

			if (!json_ifp_in) {
				json_ifp_in = json_object_new_object();
				json_object_object_add(json_source, in_ifname,
						       json_ifp_in);
				json_object_int_add(json_source, "Installed",
						    c_oil->installed);
				json_object_int_add(json_source, "installed",
						    c_oil->installed);
				json_object_boolean_add(json_source, "isRpt",
							isRpt);
				json_object_int_add(json_source, "RefCount",
						    c_oil->oil_ref_count);
				json_object_int_add(json_source, "refCount",
						    c_oil->oil_ref_count);
				json_object_int_add(json_source, "OilListSize",
						    c_oil->oil_size);
				json_object_int_add(json_source, "oilListSize",
						    c_oil->oil_size);
				json_object_int_add(
					json_source, "OilRescan",
					c_oil->oil_inherited_rescan);
				json_object_int_add(
					json_source, "oilRescan",
					c_oil->oil_inherited_rescan);
				json_object_int_add(json_source, "LastUsed",
						    c_oil->cc.lastused);
				json_object_int_add(json_source, "lastUsed",
						    c_oil->cc.lastused);
				json_object_int_add(json_source, "PacketCount",
						    c_oil->cc.pktcnt);
				json_object_int_add(json_source, "packetCount",
						    c_oil->cc.pktcnt);
				json_object_int_add(json_source, "ByteCount",
						    c_oil->cc.bytecnt);
				json_object_int_add(json_source, "byteCount",
						    c_oil->cc.bytecnt);
				json_object_int_add(json_source,
						    "WrongInterface",
						    c_oil->cc.wrong_if);
				json_object_int_add(json_source,
						    "wrongInterface",
						    c_oil->cc.wrong_if);
			}
		} else
			vty_out(vty, "%-6d %-15pPAs  %-15pPAs  %-3s  %-16s  ",
				c_oil->installed, oil_origin(c_oil),
				oil_mcastgrp(c_oil), isRpt ? "y" : "n",
				in_ifname);

		for (oif_vif_index = 0; oif_vif_index < MAXVIFS;
		     ++oif_vif_index) {
			struct interface *ifp_out;
			char oif_uptime[10];
			int ttl;

			ttl = oil_if_has(c_oil, oif_vif_index);
			if (ttl < 1)
				continue;

			ifp_out = pim_if_find_by_vif_index(pim, oif_vif_index);
			pim_time_uptime(
				oif_uptime, sizeof(oif_uptime),
				now - c_oil->oif_creation[oif_vif_index]);

			if (ifp_out)
				strlcpy(out_ifname, ifp_out->name,
					sizeof(out_ifname));
			else
				strlcpy(out_ifname, "<oif?>",
					sizeof(out_ifname));

			if (json) {
				json_ifp_out = json_object_new_object();
				json_object_string_add(json_ifp_out, "source",
						       src_str);
				json_object_string_add(json_ifp_out, "group",
						       grp_str);
				json_object_string_add(json_ifp_out,
						       "inboundInterface",
						       in_ifname);
				json_object_string_add(json_ifp_out,
						       "outboundInterface",
						       out_ifname);
				json_object_int_add(json_ifp_out, "installed",
						    c_oil->installed);

				json_object_object_add(json_ifp_in, out_ifname,
						       json_ifp_out);
			} else {
				if (first_oif) {
					first_oif = 0;
					vty_out(vty, "%s(%c%c%c%c%c)",
						out_ifname,
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_IGMP)
							? 'I'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_PIM)
							? 'J'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_VXLAN)
							? 'V'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_STAR)
							? '*'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_MUTE)
							? 'M'
							: ' ');
				} else
					vty_out(vty, ", %s(%c%c%c%c%c)",
						out_ifname,
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_IGMP)
							? 'I'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_PIM)
							? 'J'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_VXLAN)
							? 'V'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_PROTO_STAR)
							? '*'
							: ' ',
						(c_oil->oif_flags
							 [oif_vif_index] &
						 PIM_OIF_FLAG_MUTE)
							? 'M'
							: ' ');
			}
		}

		if (!json)
			vty_out(vty, "\n");
	}

	if (!json)
		vty_out(vty, "\n");
}

/* pim statistics - just adding only bsm related now.
 * We can continue to add all pim related stats here.
 */
void pim_show_statistics(struct pim_instance *pim, struct vty *vty,
			 const char *ifname, bool uj)
{
	json_object *json = NULL;
	struct interface *ifp;

	if (uj) {
		json = json_object_new_object();
		json_object_int_add(json, "bsmRx", pim->bsm_rcvd);
		json_object_int_add(json, "bsmTx", pim->bsm_sent);
		json_object_int_add(json, "bsmDropped", pim->bsm_dropped);
	} else {
		vty_out(vty, "BSM Statistics :\n");
		vty_out(vty, "----------------\n");
		vty_out(vty, "Number of Received BSMs : %" PRIu64 "\n",
			pim->bsm_rcvd);
		vty_out(vty, "Number of Forwared BSMs : %" PRIu64 "\n",
			pim->bsm_sent);
		vty_out(vty, "Number of Dropped BSMs  : %" PRIu64 "\n",
			pim->bsm_dropped);
	}

	vty_out(vty, "\n");

	/* scan interfaces */
	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		struct pim_interface *pim_ifp = ifp->info;

		if (ifname && strcmp(ifname, ifp->name))
			continue;

		if (!pim_ifp)
			continue;

		if (!uj) {
			vty_out(vty, "Interface : %s\n", ifp->name);
			vty_out(vty, "-------------------\n");
			vty_out(vty,
				"Number of BSMs dropped due to config miss : %u\n",
				pim_ifp->pim_ifstat_bsm_cfg_miss);
			vty_out(vty, "Number of unicast BSMs dropped : %u\n",
				pim_ifp->pim_ifstat_ucast_bsm_cfg_miss);
			vty_out(vty,
				"Number of BSMs dropped due to invalid scope zone : %u\n",
				pim_ifp->pim_ifstat_bsm_invalid_sz);
		} else {

			json_object *json_row = NULL;

			json_row = json_object_new_object();

			json_object_string_add(json_row, "If Name", ifp->name);
			json_object_int_add(json_row, "bsmDroppedConfig",
					    pim_ifp->pim_ifstat_bsm_cfg_miss);
			json_object_int_add(
				json_row, "bsmDroppedUnicast",
				pim_ifp->pim_ifstat_ucast_bsm_cfg_miss);
			json_object_int_add(json_row,
					    "bsmDroppedInvalidScopeZone",
					    pim_ifp->pim_ifstat_bsm_invalid_sz);
			json_object_object_add(json, ifp->name, json_row);
		}
		vty_out(vty, "\n");
	}

	if (uj)
		vty_json(vty, json);
}

void pim_show_upstream(struct pim_instance *pim, struct vty *vty,
		       pim_sgaddr *sg, json_object *json)
{
	struct pim_upstream *up;
	time_t now;
	json_object *json_group = NULL;
	json_object *json_row = NULL;

	now = pim_time_monotonic_sec();

	if (!json)
		vty_out(vty,
			"Iif             Source          Group           State       Uptime   JoinTimer RSTimer   KATimer   RefCnt\n");

	frr_each (rb_pim_upstream, &pim->upstream_head, up) {
		char uptime[10];
		char join_timer[10];
		char rs_timer[10];
		char ka_timer[10];
		char msdp_reg_timer[10];
		char state_str[PIM_REG_STATE_STR_LEN];

		if (!pim_sgaddr_match(up->sg, *sg))
			continue;

		pim_time_uptime(uptime, sizeof(uptime),
				now - up->state_transition);
		pim_time_timer_to_hhmmss(join_timer, sizeof(join_timer),
					 up->t_join_timer);

		/*
		 * If the upstream is not dummy and it has a J/P timer for the
		 * neighbor display that
		 */
		if (!up->t_join_timer && up->rpf.source_nexthop.interface) {
			struct pim_neighbor *nbr;

			nbr = pim_neighbor_find_prefix(
				up->rpf.source_nexthop.interface,
				&up->rpf.rpf_addr);
			if (nbr)
				pim_time_timer_to_hhmmss(join_timer,
							 sizeof(join_timer),
							 nbr->jp_timer);
		}

		pim_time_timer_to_hhmmss(rs_timer, sizeof(rs_timer),
					 up->t_rs_timer);
		pim_time_timer_to_hhmmss(ka_timer, sizeof(ka_timer),
					 up->t_ka_timer);
		pim_time_timer_to_hhmmss(msdp_reg_timer, sizeof(msdp_reg_timer),
					 up->t_msdp_reg_timer);

		pim_upstream_state2brief_str(up->join_state, state_str,
					     sizeof(state_str));
		if (up->reg_state != PIM_REG_NOINFO) {
			char tmp_str[PIM_REG_STATE_STR_LEN];
			char tmp[sizeof(state_str) + 1];

			snprintf(tmp, sizeof(tmp), ",%s",
				 pim_reg_state2brief_str(up->reg_state, tmp_str,
							 sizeof(tmp_str)));
			strlcat(state_str, tmp, sizeof(state_str));
		}

		if (json) {
			char grp_str[PIM_ADDRSTRLEN];
			char src_str[PIM_ADDRSTRLEN];

			snprintfrr(grp_str, sizeof(grp_str), "%pPAs",
				   &up->sg.grp);
			snprintfrr(src_str, sizeof(src_str), "%pPAs",
				   &up->sg.src);

			json_object_object_get_ex(json, grp_str, &json_group);

			if (!json_group) {
				json_group = json_object_new_object();
				json_object_object_add(json, grp_str,
						       json_group);
			}

			json_row = json_object_new_object();
			json_object_pim_upstream_add(json_row, up);
			json_object_string_add(
				json_row, "inboundInterface",
				up->rpf.source_nexthop.interface
				? up->rpf.source_nexthop.interface->name
				: "Unknown");

			/*
			 * The RPF address we use is slightly different
			 * based upon what we are looking up.
			 * If we have a S, list that unless
			 * we are the FHR, else we just put
			 * the RP as the rpfAddress
			 */
			if (up->flags & PIM_UPSTREAM_FLAG_MASK_FHR ||
			    pim_addr_is_any(up->sg.src)) {
				struct pim_rpf *rpg;

				rpg = RP(pim, up->sg.grp);
				json_object_string_addf(json_row, "rpfAddress",
							"%pFX", &rpg->rpf_addr);
			} else {
				json_object_string_add(json_row, "rpfAddress",
						       src_str);
			}

			json_object_string_add(json_row, "source", src_str);
			json_object_string_add(json_row, "group", grp_str);
			json_object_string_add(json_row, "state", state_str);
			json_object_string_add(
				json_row, "joinState",
				pim_upstream_state2str(up->join_state));
			json_object_string_add(
				json_row, "regState",
				pim_reg_state2str(up->reg_state, state_str,
						  sizeof(state_str)));
			json_object_string_add(json_row, "upTime", uptime);
			json_object_string_add(json_row, "joinTimer",
					       join_timer);
			json_object_string_add(json_row, "resetTimer",
					       rs_timer);
			json_object_string_add(json_row, "keepaliveTimer",
					       ka_timer);
			json_object_string_add(json_row, "msdpRegTimer",
					       msdp_reg_timer);
			json_object_int_add(json_row, "refCount",
					    up->ref_count);
			json_object_int_add(json_row, "sptBit", up->sptbit);
			json_object_object_add(json_group, src_str, json_row);
		} else {
			vty_out(vty,
				"%-16s%-15pPAs %-15pPAs %-11s %-8s %-9s %-9s %-9s %6d\n",
				up->rpf.source_nexthop.interface
				? up->rpf.source_nexthop.interface->name
				: "Unknown",
				&up->sg.src, &up->sg.grp, state_str, uptime,
				join_timer, rs_timer, ka_timer, up->ref_count);
		}
	}
}

static void pim_show_join_desired_helper(struct pim_instance *pim,
					 struct vty *vty,
					 struct pim_upstream *up,
					 json_object *json, bool uj)
{
	json_object *json_group = NULL;
	json_object *json_row = NULL;

	if (uj) {
		char grp_str[PIM_ADDRSTRLEN];
		char src_str[PIM_ADDRSTRLEN];

		snprintfrr(grp_str, sizeof(grp_str), "%pPAs", &up->sg.grp);
		snprintfrr(src_str, sizeof(src_str), "%pPAs", &up->sg.src);

		json_object_object_get_ex(json, grp_str, &json_group);

		if (!json_group) {
			json_group = json_object_new_object();
			json_object_object_add(json, grp_str, json_group);
		}

		json_row = json_object_new_object();
		json_object_pim_upstream_add(json_row, up);
		json_object_string_add(json_row, "source", src_str);
		json_object_string_add(json_row, "group", grp_str);

		if (pim_upstream_evaluate_join_desired(pim, up))
			json_object_boolean_true_add(json_row,
						     "evaluateJoinDesired");

		json_object_object_add(json_group, src_str, json_row);

	} else {
		vty_out(vty, "%-15pPAs %-15pPAs %-6s\n", &up->sg.src,
			&up->sg.grp,
			pim_upstream_evaluate_join_desired(pim, up) ? "yes"
								    : "no");
	}
}

void pim_show_join_desired(struct pim_instance *pim, struct vty *vty, bool uj)
{
	struct pim_upstream *up;

	json_object *json = NULL;

	if (uj)
		json = json_object_new_object();
	else
		vty_out(vty, "Source          Group           EvalJD\n");

	frr_each (rb_pim_upstream, &pim->upstream_head, up) {
		/* scan all interfaces */
		pim_show_join_desired_helper(pim, vty, up, json, uj);
	}

	if (uj)
		vty_json(vty, json);
}

void pim_show_upstream_rpf(struct pim_instance *pim, struct vty *vty, bool uj)
{
	struct pim_upstream *up;
	json_object *json = NULL;
	json_object *json_group = NULL;
	json_object *json_row = NULL;

	if (uj)
		json = json_object_new_object();
	else
		vty_out(vty,
			"Source          Group           RpfIface         RibNextHop      RpfAddress     \n");

	frr_each (rb_pim_upstream, &pim->upstream_head, up) {
		char rpf_nexthop_str[PREFIX_STRLEN];
		char rpf_addr_str[PREFIX_STRLEN];
		struct pim_rpf *rpf;
		const char *rpf_ifname;

		rpf = &up->rpf;

		pim_addr_dump("<nexthop?>",
			      &rpf->source_nexthop.mrib_nexthop_addr,
			      rpf_nexthop_str, sizeof(rpf_nexthop_str));
		pim_addr_dump("<rpf?>", &rpf->rpf_addr, rpf_addr_str,
			      sizeof(rpf_addr_str));

		rpf_ifname =
			rpf->source_nexthop.interface ? rpf->source_nexthop
								.interface->name
						      : "<ifname?>";

		if (uj) {
			char grp_str[PIM_ADDRSTRLEN];
			char src_str[PIM_ADDRSTRLEN];

			snprintfrr(grp_str, sizeof(grp_str), "%pPAs",
				   &up->sg.grp);
			snprintfrr(src_str, sizeof(src_str), "%pPAs",
				   &up->sg.src);
			json_object_object_get_ex(json, grp_str, &json_group);

			if (!json_group) {
				json_group = json_object_new_object();
				json_object_object_add(json, grp_str,
						       json_group);
			}

			json_row = json_object_new_object();
			json_object_pim_upstream_add(json_row, up);
			json_object_string_add(json_row, "source", src_str);
			json_object_string_add(json_row, "group", grp_str);
			json_object_string_add(json_row, "rpfInterface",
					       rpf_ifname);
			json_object_string_add(json_row, "ribNexthop",
					       rpf_nexthop_str);
			json_object_string_add(json_row, "rpfAddress",
					       rpf_addr_str);
			json_object_object_add(json_group, src_str, json_row);
		} else {
			vty_out(vty, "%-15pPAs %-15pPAs %-16s %-15s %-15s\n",
				&up->sg.src, &up->sg.grp, rpf_ifname,
				rpf_nexthop_str, rpf_addr_str);
		}
	}

	if (uj)
		vty_json(vty, json);
}

static void pim_show_join_helper(struct vty *vty, struct pim_interface *pim_ifp,
				 struct pim_ifchannel *ch, json_object *json,
				 time_t now)
{
	json_object *json_iface = NULL;
	json_object *json_row = NULL;
	json_object *json_grp = NULL;
	pim_addr ifaddr;
	char uptime[10];
	char expire[10];
	char prune[10];

	ifaddr = pim_ifp->primary_address;

	pim_time_uptime_begin(uptime, sizeof(uptime), now, ch->ifjoin_creation);
	pim_time_timer_to_mmss(expire, sizeof(expire),
			       ch->t_ifjoin_expiry_timer);
	pim_time_timer_to_mmss(prune, sizeof(prune),
			       ch->t_ifjoin_prune_pending_timer);

	if (json) {
		char ch_grp_str[PIM_ADDRSTRLEN];

		json_object_object_get_ex(json, ch->interface->name,
					  &json_iface);

		if (!json_iface) {
			json_iface = json_object_new_object();
			json_object_pim_ifp_add(json_iface, ch->interface);
			json_object_object_add(json, ch->interface->name,
					       json_iface);
		}

		json_row = json_object_new_object();
		json_object_string_addf(json_row, "source", "%pPAs",
					&ch->sg.src);
		json_object_string_addf(json_row, "group", "%pPAs",
					&ch->sg.grp);
		json_object_string_add(json_row, "upTime", uptime);
		json_object_string_add(json_row, "expire", expire);
		json_object_string_add(json_row, "prune", prune);
		json_object_string_add(
			json_row, "channelJoinName",
			pim_ifchannel_ifjoin_name(ch->ifjoin_state, ch->flags));
		if (PIM_IF_FLAG_TEST_S_G_RPT(ch->flags)) {
#if CONFDATE > 20230131
			CPP_NOTICE(
				"Remove JSON object commands with keys starting with capital")
#endif
			json_object_int_add(json_row, "SGRpt", 1);
			json_object_int_add(json_row, "sgRpt", 1);
		}
		if (PIM_IF_FLAG_TEST_PROTO_PIM(ch->flags))
			json_object_int_add(json_row, "protocolPim", 1);
		if (PIM_IF_FLAG_TEST_PROTO_IGMP(ch->flags))
			json_object_int_add(json_row, "protocolIgmp", 1);
		snprintfrr(ch_grp_str, sizeof(ch_grp_str), "%pPAs",
			   &ch->sg.grp);
		json_object_object_get_ex(json_iface, ch_grp_str, &json_grp);
		if (!json_grp) {
			json_grp = json_object_new_object();
			json_object_object_addf(json_grp, json_row, "%pPAs",
						&ch->sg.src);
			json_object_object_addf(json_iface, json_grp, "%pPAs",
						&ch->sg.grp);
		} else
			json_object_object_addf(json_grp, json_row, "%pPAs",
						&ch->sg.src);
	} else {
		vty_out(vty,
			"%-16s %-15pPAs %-15pPAs %-15pPAs %-10s %8s %-6s %5s\n",
			ch->interface->name, &ifaddr, &ch->sg.src, &ch->sg.grp,
			pim_ifchannel_ifjoin_name(ch->ifjoin_state, ch->flags),
			uptime, expire, prune);
	}
}

void pim_show_join(struct pim_instance *pim, struct vty *vty, pim_sgaddr *sg,
		   json_object *json)
{
	struct pim_interface *pim_ifp;
	struct pim_ifchannel *ch;
	struct interface *ifp;
	time_t now;

	now = pim_time_monotonic_sec();

	if (!json)
		vty_out(vty,
			"Interface        Address         Source          Group           State      Uptime   Expire Prune\n");

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;
		if (!pim_ifp)
			continue;

		RB_FOREACH (ch, pim_ifchannel_rb, &pim_ifp->ifchannel_rb) {
			if (!pim_sgaddr_match(ch->sg, *sg))
				continue;

			pim_show_join_helper(vty, pim_ifp, ch, json, now);
		} /* scan interface channels */
	}
}

static void pim_show_jp_agg_helper(struct vty *vty, struct interface *ifp,
				   struct pim_neighbor *neigh,
				   struct pim_upstream *up, int is_join)
{
	vty_out(vty, "%-16s %-15pPAs %-15pPAs %-15pPAs %5s\n", ifp->name,
		&neigh->source_addr, &up->sg.src, &up->sg.grp,
		is_join ? "J" : "P");
}

void pim_show_jp_agg_list(struct pim_instance *pim, struct vty *vty)
{
	struct interface *ifp;
	struct pim_interface *pim_ifp;
	struct listnode *n_node;
	struct pim_neighbor *neigh;
	struct listnode *jag_node;
	struct pim_jp_agg_group *jag;
	struct listnode *js_node;
	struct pim_jp_sources *js;

	vty_out(vty,
		"Interface        RPF Nbr         Source          Group           State\n");

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;
		if (!pim_ifp)
			continue;

		for (ALL_LIST_ELEMENTS_RO(pim_ifp->pim_neighbor_list, n_node,
					  neigh)) {
			for (ALL_LIST_ELEMENTS_RO(neigh->upstream_jp_agg,
						  jag_node, jag)) {
				for (ALL_LIST_ELEMENTS_RO(jag->sources, js_node,
							  js)) {
					pim_show_jp_agg_helper(vty, ifp, neigh,
							       js->up,
							       js->is_join);
				}
			}
		}
	}
}

static void pim_show_membership_helper(struct vty *vty,
				       struct pim_interface *pim_ifp,
				       struct pim_ifchannel *ch,
				       struct json_object *json)
{
	json_object *json_iface = NULL;
	json_object *json_row = NULL;

	json_object_object_get_ex(json, ch->interface->name, &json_iface);
	if (!json_iface) {
		json_iface = json_object_new_object();
		json_object_pim_ifp_add(json_iface, ch->interface);
		json_object_object_add(json, ch->interface->name, json_iface);
	}

	json_row = json_object_new_object();
	json_object_string_addf(json_row, "source", "%pPAs", &ch->sg.src);
	json_object_string_addf(json_row, "group", "%pPAs", &ch->sg.grp);
	json_object_string_add(json_row, "localMembership",
			       ch->local_ifmembership == PIM_IFMEMBERSHIP_NOINFO
				       ? "NOINFO"
				       : "INCLUDE");
	json_object_object_addf(json_iface, json_row, "%pPAs", &ch->sg.grp);
}

void pim_show_membership(struct pim_instance *pim, struct vty *vty, bool uj)
{
	struct pim_interface *pim_ifp;
	struct pim_ifchannel *ch;
	struct interface *ifp;
	enum json_type type;
	json_object *json = NULL;
	json_object *json_tmp = NULL;

	json = json_object_new_object();

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;
		if (!pim_ifp)
			continue;

		RB_FOREACH (ch, pim_ifchannel_rb, &pim_ifp->ifchannel_rb) {
			pim_show_membership_helper(vty, pim_ifp, ch, json);
		} /* scan interface channels */
	}

	if (uj) {
		vty_json(vty, json);
	} else {
		vty_out(vty,
			"Interface         Address          Source           Group            Membership\n");

		/*
		 * Example of the json data we are traversing
		 *
		 * {
		 *   "swp3":{
		 *     "name":"swp3",
		 *     "state":"up",
		 *     "address":"10.1.20.1",
		 *     "index":5,
		 *     "flagMulticast":true,
		 *     "flagBroadcast":true,
		 *     "lanDelayEnabled":true,
		 *     "226.10.10.10":{
		 *       "source":"*",
		 *       "group":"226.10.10.10",
		 *       "localMembership":"INCLUDE"
		 *     }
		 *   }
		 * }
		 */

		/* foreach interface */
		json_object_object_foreach(json, key, val)
		{

			/* Find all of the keys where the val is an object. In
			 * the example
			 * above the only one is 226.10.10.10
			 */
			json_object_object_foreach(val, if_field_key,
						   if_field_val)
			{
				type = json_object_get_type(if_field_val);

				if (type == json_type_object) {
					vty_out(vty, "%-16s  ", key);

					json_object_object_get_ex(
						val, "address", &json_tmp);
					vty_out(vty, "%-15s  ",
						json_object_get_string(
							json_tmp));

					json_object_object_get_ex(if_field_val,
								  "source",
								  &json_tmp);
					vty_out(vty, "%-15s  ",
						json_object_get_string(
							json_tmp));

					/* Group */
					vty_out(vty, "%-15s  ", if_field_key);

					json_object_object_get_ex(
						if_field_val, "localMembership",
						&json_tmp);
					vty_out(vty, "%-10s\n",
						json_object_get_string(
							json_tmp));
				}
			}
		}
		json_object_free(json);
	}
}

static void pim_show_channel_helper(struct pim_instance *pim, struct vty *vty,
				    struct pim_interface *pim_ifp,
				    struct pim_ifchannel *ch, json_object *json,
				    bool uj)
{
	struct pim_upstream *up = ch->upstream;
	json_object *json_group = NULL;
	json_object *json_row = NULL;

	if (uj) {
		char grp_str[PIM_ADDRSTRLEN];

		snprintfrr(grp_str, sizeof(grp_str), "%pPAs", &up->sg.grp);
		json_object_object_get_ex(json, grp_str, &json_group);

		if (!json_group) {
			json_group = json_object_new_object();
			json_object_object_add(json, grp_str, json_group);
		}

		json_row = json_object_new_object();
		json_object_pim_upstream_add(json_row, up);
		json_object_string_add(json_row, "interface",
				       ch->interface->name);
		json_object_string_addf(json_row, "source", "%pPAs",
					&up->sg.src);
		json_object_string_addf(json_row, "group", "%pPAs",
					&up->sg.grp);

		if (pim_macro_ch_lost_assert(ch))
			json_object_boolean_true_add(json_row, "lostAssert");

		if (pim_macro_chisin_joins(ch))
			json_object_boolean_true_add(json_row, "joins");

		if (pim_macro_chisin_pim_include(ch))
			json_object_boolean_true_add(json_row, "pimInclude");

		if (pim_upstream_evaluate_join_desired(pim, up))
			json_object_boolean_true_add(json_row,
						     "evaluateJoinDesired");

		json_object_object_addf(json_group, json_row, "%pPAs",
					&up->sg.src);

	} else {
		vty_out(vty,
			"%-16s %-15pPAs %-15pPAs %-10s %-5s %-10s %-11s %-6s\n",
			ch->interface->name, &up->sg.src, &up->sg.grp,
			pim_macro_ch_lost_assert(ch) ? "yes" : "no",
			pim_macro_chisin_joins(ch) ? "yes" : "no",
			pim_macro_chisin_pim_include(ch) ? "yes" : "no",
			PIM_UPSTREAM_FLAG_TEST_DR_JOIN_DESIRED(up->flags)
				? "yes"
				: "no",
			pim_upstream_evaluate_join_desired(pim, up) ? "yes"
								    : "no");
	}
}

void pim_show_channel(struct pim_instance *pim, struct vty *vty, bool uj)
{
	struct pim_interface *pim_ifp;
	struct pim_ifchannel *ch;
	struct interface *ifp;

	json_object *json = NULL;

	if (uj)
		json = json_object_new_object();
	else
		vty_out(vty,
			"Interface        Source          Group           LostAssert Joins PimInclude JoinDesired EvalJD\n");

	/* scan per-interface (S,G) state */
	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;
		if (!pim_ifp)
			continue;


		RB_FOREACH (ch, pim_ifchannel_rb, &pim_ifp->ifchannel_rb) {
			/* scan all interfaces */
			pim_show_channel_helper(pim, vty, pim_ifp, ch, json,
						uj);
		}
	}

	if (uj)
		vty_json(vty, json);
}

void pim_show_interfaces(struct pim_instance *pim, struct vty *vty, bool mlag,
			 json_object *json)
{
	struct interface *ifp;
	struct pim_interface *pim_ifp;
	struct pim_upstream *up;
	int fhr = 0;
	int pim_nbrs = 0;
	int pim_ifchannels = 0;
	bool uj = true;
	json_object *json_row = NULL;
	json_object *json_tmp;

	if (!json) {
		uj = false;
		json = json_object_new_object();
	}

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;

		if (!pim_ifp)
			continue;

		if (mlag == true && pim_ifp->activeactive == false)
			continue;

		pim_nbrs = pim_ifp->pim_neighbor_list->count;
		pim_ifchannels = pim_if_ifchannel_count(pim_ifp);
		fhr = 0;

		frr_each (rb_pim_upstream, &pim->upstream_head, up)
			if (ifp == up->rpf.source_nexthop.interface)
				if (up->flags & PIM_UPSTREAM_FLAG_MASK_FHR)
					fhr++;

		json_row = json_object_new_object();
		json_object_pim_ifp_add(json_row, ifp);
		json_object_int_add(json_row, "pimNeighbors", pim_nbrs);
		json_object_int_add(json_row, "pimIfChannels", pim_ifchannels);
		json_object_int_add(json_row, "firstHopRouterCount", fhr);
		json_object_string_addf(json_row, "pimDesignatedRouter",
					"%pPAs", &pim_ifp->pim_dr_addr);

		if (pim_addr_cmp(pim_ifp->pim_dr_addr,
				 pim_ifp->primary_address))
			json_object_boolean_true_add(
				json_row, "pimDesignatedRouterLocal");

		json_object_object_add(json, ifp->name, json_row);
	}

	if (!uj) {
		vty_out(vty,
			"Interface         State          Address  PIM Nbrs           PIM DR  FHR IfChannels\n");

		json_object_object_foreach(json, key, val)
		{
			vty_out(vty, "%-16s  ", key);

			json_object_object_get_ex(val, "state", &json_tmp);
			vty_out(vty, "%5s  ", json_object_get_string(json_tmp));

			json_object_object_get_ex(val, "address", &json_tmp);
			vty_out(vty, "%15s  ",
				json_object_get_string(json_tmp));

			json_object_object_get_ex(val, "pimNeighbors",
						  &json_tmp);
			vty_out(vty, "%8d  ", json_object_get_int(json_tmp));

			if (json_object_object_get_ex(
				    val, "pimDesignatedRouterLocal",
				    &json_tmp)) {
				vty_out(vty, "%15s  ", "local");
			} else {
				json_object_object_get_ex(
					val, "pimDesignatedRouter", &json_tmp);
				vty_out(vty, "%15s  ",
					json_object_get_string(json_tmp));
			}

			json_object_object_get_ex(val, "firstHopRouter",
						  &json_tmp);
			vty_out(vty, "%3d  ", json_object_get_int(json_tmp));

			json_object_object_get_ex(val, "pimIfChannels",
						  &json_tmp);
			vty_out(vty, "%9d\n", json_object_get_int(json_tmp));
		}
	}
}

void pim_show_interfaces_single(struct pim_instance *pim, struct vty *vty,
				const char *ifname, bool mlag,
				json_object *json)
{
	pim_addr ifaddr;
	struct interface *ifp;
	struct listnode *neighnode;
	struct pim_interface *pim_ifp;
	struct pim_neighbor *neigh;
	struct pim_upstream *up;
	time_t now;
	char dr_str[PIM_ADDRSTRLEN];
	char dr_uptime[10];
	char expire[10];
	char grp_str[PIM_ADDRSTRLEN];
	char hello_period[10];
	char hello_timer[10];
	char neigh_src_str[PIM_ADDRSTRLEN];
	char src_str[PIM_ADDRSTRLEN];
	char stat_uptime[10];
	char uptime[10];
	int found_ifname = 0;
	int print_header;
	json_object *json_row = NULL;
	json_object *json_pim_neighbor = NULL;
	json_object *json_pim_neighbors = NULL;
	json_object *json_group = NULL;
	json_object *json_group_source = NULL;
	json_object *json_fhr_sources = NULL;
	struct pim_secondary_addr *sec_addr;
	struct listnode *sec_node;

	now = pim_time_monotonic_sec();

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;

		if (!pim_ifp)
			continue;

		if (mlag == true && pim_ifp->activeactive == false)
			continue;

		if (strcmp(ifname, "detail") && strcmp(ifname, ifp->name))
			continue;

		found_ifname = 1;
		ifaddr = pim_ifp->primary_address;
		snprintfrr(dr_str, sizeof(dr_str), "%pPAs",
			   &pim_ifp->pim_dr_addr);
		pim_time_uptime_begin(dr_uptime, sizeof(dr_uptime), now,
				      pim_ifp->pim_dr_election_last);
		pim_time_timer_to_hhmmss(hello_timer, sizeof(hello_timer),
					 pim_ifp->t_pim_hello_timer);
		pim_time_mmss(hello_period, sizeof(hello_period),
			      pim_ifp->pim_hello_period);
		pim_time_uptime(stat_uptime, sizeof(stat_uptime),
				now - pim_ifp->pim_ifstat_start);

		if (json) {
			json_row = json_object_new_object();
			json_object_pim_ifp_add(json_row, ifp);

			if (!pim_addr_is_any(pim_ifp->update_source)) {
				json_object_string_addf(
					json_row, "useSource", "%pPAs",
					&pim_ifp->update_source);
			}
			if (pim_ifp->sec_addr_list) {
				json_object *sec_list = NULL;

				sec_list = json_object_new_array();
				for (ALL_LIST_ELEMENTS_RO(
					     pim_ifp->sec_addr_list, sec_node,
					     sec_addr)) {
					json_object_array_add(
						sec_list,
						json_object_new_stringf(
							"%pFXh",
							&sec_addr->addr));
				}
				json_object_object_add(json_row,
						       "secondaryAddressList",
						       sec_list);
			}

			/* PIM neighbors */
			if (pim_ifp->pim_neighbor_list->count) {
				json_pim_neighbors = json_object_new_object();

				for (ALL_LIST_ELEMENTS_RO(
					     pim_ifp->pim_neighbor_list,
					     neighnode, neigh)) {
					json_pim_neighbor =
						json_object_new_object();
					snprintfrr(neigh_src_str,
						   sizeof(neigh_src_str),
						   "%pPAs",
						   &neigh->source_addr);
					pim_time_uptime(uptime, sizeof(uptime),
							now - neigh->creation);
					pim_time_timer_to_hhmmss(
						expire, sizeof(expire),
						neigh->t_expire_timer);

					json_object_string_add(
						json_pim_neighbor, "address",
						neigh_src_str);
					json_object_string_add(
						json_pim_neighbor, "upTime",
						uptime);
					json_object_string_add(
						json_pim_neighbor, "holdtime",
						expire);

					json_object_object_add(
						json_pim_neighbors,
						neigh_src_str,
						json_pim_neighbor);
				}

				json_object_object_add(json_row, "neighbors",
						       json_pim_neighbors);
			}

			json_object_string_add(json_row, "drAddress", dr_str);
			json_object_int_add(json_row, "drPriority",
					    pim_ifp->pim_dr_priority);
			json_object_string_add(json_row, "drUptime", dr_uptime);
			json_object_int_add(json_row, "drElections",
					    pim_ifp->pim_dr_election_count);
			json_object_int_add(json_row, "drChanges",
					    pim_ifp->pim_dr_election_changes);

			/* FHR */
			frr_each (rb_pim_upstream, &pim->upstream_head, up) {
				if (ifp != up->rpf.source_nexthop.interface)
					continue;

				if (!(up->flags & PIM_UPSTREAM_FLAG_MASK_FHR))
					continue;

				if (!json_fhr_sources)
					json_fhr_sources =
						json_object_new_object();

				snprintfrr(grp_str, sizeof(grp_str), "%pPAs",
					   &up->sg.grp);
				snprintfrr(src_str, sizeof(src_str), "%pPAs",
					   &up->sg.src);
				pim_time_uptime(uptime, sizeof(uptime),
						now - up->state_transition);

				/*
				 * Does this group live in json_fhr_sources?
				 * If not create it.
				 */
				json_object_object_get_ex(json_fhr_sources,
							  grp_str, &json_group);

				if (!json_group) {
					json_group = json_object_new_object();
					json_object_object_add(json_fhr_sources,
							       grp_str,
							       json_group);
				}

				json_group_source = json_object_new_object();
				json_object_string_add(json_group_source,
						       "source", src_str);
				json_object_string_add(json_group_source,
						       "group", grp_str);
				json_object_string_add(json_group_source,
						       "upTime", uptime);
				json_object_object_add(json_group, src_str,
						       json_group_source);
			}

			if (json_fhr_sources) {
				json_object_object_add(json_row,
						       "firstHopRouter",
						       json_fhr_sources);
			}

			json_object_int_add(json_row, "helloPeriod",
					    pim_ifp->pim_hello_period);
			json_object_int_add(json_row, "holdTime",
					    PIM_IF_DEFAULT_HOLDTIME(pim_ifp));
			json_object_string_add(json_row, "helloTimer",
					       hello_timer);
			json_object_string_add(json_row, "helloStatStart",
					       stat_uptime);
			json_object_int_add(json_row, "helloReceived",
					    pim_ifp->pim_ifstat_hello_recv);
			json_object_int_add(json_row, "helloReceivedFailed",
					    pim_ifp->pim_ifstat_hello_recvfail);
			json_object_int_add(json_row, "helloSend",
					    pim_ifp->pim_ifstat_hello_sent);
			json_object_int_add(json_row, "hellosendFailed",
					    pim_ifp->pim_ifstat_hello_sendfail);
			json_object_int_add(json_row, "helloGenerationId",
					    pim_ifp->pim_generation_id);

			json_object_int_add(
				json_row, "effectivePropagationDelay",
				pim_if_effective_propagation_delay_msec(ifp));
			json_object_int_add(
				json_row, "effectiveOverrideInterval",
				pim_if_effective_override_interval_msec(ifp));
			json_object_int_add(
				json_row, "joinPruneOverrideInterval",
				pim_if_jp_override_interval_msec(ifp));

			json_object_int_add(
				json_row, "propagationDelay",
				pim_ifp->pim_propagation_delay_msec);
			json_object_int_add(
				json_row, "propagationDelayHighest",
				pim_ifp->pim_neighbors_highest_propagation_delay_msec);
			json_object_int_add(
				json_row, "overrideInterval",
				pim_ifp->pim_override_interval_msec);
			json_object_int_add(
				json_row, "overrideIntervalHighest",
				pim_ifp->pim_neighbors_highest_override_interval_msec);
			if (pim_ifp->bsm_enable)
				json_object_boolean_true_add(json_row,
							     "bsmEnabled");
			if (pim_ifp->ucast_bsm_accept)
				json_object_boolean_true_add(json_row,
							     "ucastBsmEnabled");
			json_object_object_add(json, ifp->name, json_row);

		} else {
			vty_out(vty, "Interface  : %s\n", ifp->name);
			vty_out(vty, "State      : %s\n",
				if_is_up(ifp) ? "up" : "down");
			if (!pim_addr_is_any(pim_ifp->update_source)) {
				vty_out(vty, "Use Source : %pPAs\n",
					&pim_ifp->update_source);
			}
			if (pim_ifp->sec_addr_list) {
				vty_out(vty, "Address    : %pPAs (primary)\n",
					&ifaddr);
				for (ALL_LIST_ELEMENTS_RO(
					     pim_ifp->sec_addr_list, sec_node,
					     sec_addr))
					vty_out(vty, "             %pFX\n",
						&sec_addr->addr);
			} else {
				vty_out(vty, "Address    : %pPAs\n", &ifaddr);
			}
			vty_out(vty, "\n");

			/* PIM neighbors */
			print_header = 1;

			for (ALL_LIST_ELEMENTS_RO(pim_ifp->pim_neighbor_list,
						  neighnode, neigh)) {

				if (print_header) {
					vty_out(vty, "PIM Neighbors\n");
					vty_out(vty, "-------------\n");
					print_header = 0;
				}

				snprintfrr(neigh_src_str, sizeof(neigh_src_str),
					   "%pPAs", &neigh->source_addr);
				pim_time_uptime(uptime, sizeof(uptime),
						now - neigh->creation);
				pim_time_timer_to_hhmmss(expire, sizeof(expire),
							 neigh->t_expire_timer);
				vty_out(vty,
					"%-15s : up for %s, holdtime expires in %s\n",
					neigh_src_str, uptime, expire);
			}

			if (!print_header) {
				vty_out(vty, "\n");
				vty_out(vty, "\n");
			}

			vty_out(vty, "Designated Router\n");
			vty_out(vty, "-----------------\n");
			vty_out(vty, "Address   : %s\n", dr_str);
			vty_out(vty, "Priority  : %u(%d)\n",
				pim_ifp->pim_dr_priority,
				pim_ifp->pim_dr_num_nondrpri_neighbors);
			vty_out(vty, "Uptime    : %s\n", dr_uptime);
			vty_out(vty, "Elections : %d\n",
				pim_ifp->pim_dr_election_count);
			vty_out(vty, "Changes   : %d\n",
				pim_ifp->pim_dr_election_changes);
			vty_out(vty, "\n");
			vty_out(vty, "\n");

			/* FHR */
			print_header = 1;
			frr_each (rb_pim_upstream, &pim->upstream_head, up) {
				if (!up->rpf.source_nexthop.interface)
					continue;

				if (strcmp(ifp->name,
					   up->rpf.source_nexthop
						   .interface->name) != 0)
					continue;

				if (!(up->flags & PIM_UPSTREAM_FLAG_MASK_FHR))
					continue;

				if (print_header) {
					vty_out(vty,
						"FHR - First Hop Router\n");
					vty_out(vty,
						"----------------------\n");
					print_header = 0;
				}

				pim_time_uptime(uptime, sizeof(uptime),
						now - up->state_transition);
				vty_out(vty,
					"%pPAs : %pPAs is a source, uptime is %s\n",
					&up->sg.grp, &up->sg.src, uptime);
			}

			if (!print_header) {
				vty_out(vty, "\n");
				vty_out(vty, "\n");
			}

			vty_out(vty, "Hellos\n");
			vty_out(vty, "------\n");
			vty_out(vty, "Period         : %d\n",
				pim_ifp->pim_hello_period);
			vty_out(vty, "HoldTime       : %d\n",
				PIM_IF_DEFAULT_HOLDTIME(pim_ifp));
			vty_out(vty, "Timer          : %s\n", hello_timer);
			vty_out(vty, "StatStart      : %s\n", stat_uptime);
			vty_out(vty, "Receive        : %d\n",
				pim_ifp->pim_ifstat_hello_recv);
			vty_out(vty, "Receive Failed : %d\n",
				pim_ifp->pim_ifstat_hello_recvfail);
			vty_out(vty, "Send           : %d\n",
				pim_ifp->pim_ifstat_hello_sent);
			vty_out(vty, "Send Failed    : %d\n",
				pim_ifp->pim_ifstat_hello_sendfail);
			vty_out(vty, "Generation ID  : %08x\n",
				pim_ifp->pim_generation_id);
			vty_out(vty, "\n");
			vty_out(vty, "\n");

			pim_print_ifp_flags(vty, ifp);

			vty_out(vty, "Join Prune Interval\n");
			vty_out(vty, "-------------------\n");
			vty_out(vty, "LAN Delay                    : %s\n",
				pim_if_lan_delay_enabled(ifp) ? "yes" : "no");
			vty_out(vty, "Effective Propagation Delay  : %d msec\n",
				pim_if_effective_propagation_delay_msec(ifp));
			vty_out(vty, "Effective Override Interval  : %d msec\n",
				pim_if_effective_override_interval_msec(ifp));
			vty_out(vty, "Join Prune Override Interval : %d msec\n",
				pim_if_jp_override_interval_msec(ifp));
			vty_out(vty, "\n");
			vty_out(vty, "\n");

			vty_out(vty, "LAN Prune Delay\n");
			vty_out(vty, "---------------\n");
			vty_out(vty, "Propagation Delay           : %d msec\n",
				pim_ifp->pim_propagation_delay_msec);
			vty_out(vty, "Propagation Delay (Highest) : %d msec\n",
				pim_ifp->pim_neighbors_highest_propagation_delay_msec);
			vty_out(vty, "Override Interval           : %d msec\n",
				pim_ifp->pim_override_interval_msec);
			vty_out(vty, "Override Interval (Highest) : %d msec\n",
				pim_ifp->pim_neighbors_highest_override_interval_msec);
			vty_out(vty, "\n");
			vty_out(vty, "\n");

			vty_out(vty, "BSM Status\n");
			vty_out(vty, "----------\n");
			vty_out(vty, "Bsm Enabled          : %s\n",
				pim_ifp->bsm_enable ? "yes" : "no");
			vty_out(vty, "Unicast Bsm Enabled  : %s\n",
				pim_ifp->ucast_bsm_accept ? "yes" : "no");
			vty_out(vty, "\n");
			vty_out(vty, "\n");
		}
	}

	if (!found_ifname)
		vty_out(vty, "%% No such interface\n");
}

void ip_pim_ssm_show_group_range(struct pim_instance *pim, struct vty *vty,
				 bool uj)
{
	struct pim_ssm *ssm = pim->ssm_info;
	const char *range_str =
		ssm->plist_name ? ssm->plist_name : PIM_SSM_STANDARD_RANGE;

	if (uj) {
		json_object *json;

		json = json_object_new_object();
		json_object_string_add(json, "ssmGroups", range_str);
		vty_json(vty, json);
	} else
		vty_out(vty, "SSM group range : %s\n", range_str);
}

struct pnc_cache_walk_data {
	struct vty *vty;
	struct pim_instance *pim;
};

static int pim_print_pnc_cache_walkcb(struct hash_bucket *bucket, void *arg)
{
	struct pim_nexthop_cache *pnc = bucket->data;
	struct pnc_cache_walk_data *cwd = arg;
	struct vty *vty = cwd->vty;
	struct pim_instance *pim = cwd->pim;
	struct nexthop *nh_node = NULL;
	ifindex_t first_ifindex;
	struct interface *ifp = NULL;
	char buf[PREFIX_STRLEN];

	for (nh_node = pnc->nexthop; nh_node; nh_node = nh_node->next) {
		first_ifindex = nh_node->ifindex;
		ifp = if_lookup_by_index(first_ifindex, pim->vrf->vrf_id);

		vty_out(vty, "%-15s ",
			inet_ntop(AF_INET, &pnc->rpf.rpf_addr.u.prefix4, buf,
				  sizeof(buf)));
		vty_out(vty, "%-16s ", ifp ? ifp->name : "NULL");
		vty_out(vty, "%pI4 ", &nh_node->gate.ipv4);
		vty_out(vty, "\n");
	}
	return CMD_SUCCESS;
}

void pim_show_nexthop(struct pim_instance *pim, struct vty *vty)
{
	struct pnc_cache_walk_data cwd;

	cwd.vty = vty;
	cwd.pim = pim;
	vty_out(vty, "Number of registered addresses: %lu\n",
		pim->rpf_hash->count);
	vty_out(vty, "Address         Interface        Nexthop\n");
	vty_out(vty, "---------------------------------------------\n");

	hash_walk(pim->rpf_hash, pim_print_pnc_cache_walkcb, &cwd);
}

void pim_show_neighbors_single(struct pim_instance *pim, struct vty *vty,
			       const char *neighbor, json_object *json)
{
	struct listnode *neighnode;
	struct interface *ifp;
	struct pim_interface *pim_ifp;
	struct pim_neighbor *neigh;
	time_t now;
	int found_neighbor = 0;
	int option_address_list;
	int option_dr_priority;
	int option_generation_id;
	int option_holdtime;
	int option_lan_prune_delay;
	int option_t_bit;
	char uptime[10];
	char expire[10];
	char neigh_src_str[PIM_ADDRSTRLEN];

	json_object *json_ifp = NULL;
	json_object *json_row = NULL;

	now = pim_time_monotonic_sec();

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;

		if (!pim_ifp)
			continue;

		if (pim_ifp->pim_sock_fd < 0)
			continue;

		for (ALL_LIST_ELEMENTS_RO(pim_ifp->pim_neighbor_list, neighnode,
					  neigh)) {
			snprintfrr(neigh_src_str, sizeof(neigh_src_str),
				   "%pPAs", &neigh->source_addr);

			/*
			 * The user can specify either the interface name or the
			 * PIM neighbor IP.
			 * If this pim_ifp matches neither then skip.
			 */
			if (strcmp(neighbor, "detail") &&
			    strcmp(neighbor, ifp->name) &&
			    strcmp(neighbor, neigh_src_str))
				continue;

			found_neighbor = 1;
			pim_time_uptime(uptime, sizeof(uptime),
					now - neigh->creation);
			pim_time_timer_to_hhmmss(expire, sizeof(expire),
						 neigh->t_expire_timer);

			option_address_list = 0;
			option_dr_priority = 0;
			option_generation_id = 0;
			option_holdtime = 0;
			option_lan_prune_delay = 0;
			option_t_bit = 0;

			if (PIM_OPTION_IS_SET(neigh->hello_options,
					      PIM_OPTION_MASK_ADDRESS_LIST))
				option_address_list = 1;

			if (PIM_OPTION_IS_SET(neigh->hello_options,
					      PIM_OPTION_MASK_DR_PRIORITY))
				option_dr_priority = 1;

			if (PIM_OPTION_IS_SET(neigh->hello_options,
					      PIM_OPTION_MASK_GENERATION_ID))
				option_generation_id = 1;

			if (PIM_OPTION_IS_SET(neigh->hello_options,
					      PIM_OPTION_MASK_HOLDTIME))
				option_holdtime = 1;

			if (PIM_OPTION_IS_SET(neigh->hello_options,
					      PIM_OPTION_MASK_LAN_PRUNE_DELAY))
				option_lan_prune_delay = 1;

			if (PIM_OPTION_IS_SET(
				    neigh->hello_options,
				    PIM_OPTION_MASK_CAN_DISABLE_JOIN_SUPPRESSION))
				option_t_bit = 1;

			if (json) {

				/* Does this ifp live in json? If not create it
				 */
				json_object_object_get_ex(json, ifp->name,
							  &json_ifp);

				if (!json_ifp) {
					json_ifp = json_object_new_object();
					json_object_pim_ifp_add(json_ifp, ifp);
					json_object_object_add(json, ifp->name,
							       json_ifp);
				}

				json_row = json_object_new_object();
				json_object_string_add(json_row, "interface",
						       ifp->name);
				json_object_string_add(json_row, "address",
						       neigh_src_str);
				json_object_string_add(json_row, "upTime",
						       uptime);
				json_object_string_add(json_row, "holdtime",
						       expire);
				json_object_int_add(json_row, "drPriority",
						    neigh->dr_priority);
				json_object_int_add(json_row, "generationId",
						    neigh->generation_id);

				if (option_address_list)
					json_object_boolean_true_add(
						json_row,
						"helloOptionAddressList");

				if (option_dr_priority)
					json_object_boolean_true_add(
						json_row,
						"helloOptionDrPriority");

				if (option_generation_id)
					json_object_boolean_true_add(
						json_row,
						"helloOptionGenerationId");

				if (option_holdtime)
					json_object_boolean_true_add(
						json_row,
						"helloOptionHoldtime");

				if (option_lan_prune_delay)
					json_object_boolean_true_add(
						json_row,
						"helloOptionLanPruneDelay");

				if (option_t_bit)
					json_object_boolean_true_add(
						json_row, "helloOptionTBit");

				json_object_object_add(json_ifp, neigh_src_str,
						       json_row);

			} else {
				vty_out(vty, "Interface : %s\n", ifp->name);
				vty_out(vty, "Neighbor  : %s\n", neigh_src_str);
				vty_out(vty,
					"    Uptime                         : %s\n",
					uptime);
				vty_out(vty,
					"    Holdtime                       : %s\n",
					expire);
				vty_out(vty,
					"    DR Priority                    : %d\n",
					neigh->dr_priority);
				vty_out(vty,
					"    Generation ID                  : %08x\n",
					neigh->generation_id);
				vty_out(vty,
					"    Override Interval (msec)       : %d\n",
					neigh->override_interval_msec);
				vty_out(vty,
					"    Propagation Delay (msec)       : %d\n",
					neigh->propagation_delay_msec);
				vty_out(vty,
					"    Hello Option - Address List    : %s\n",
					option_address_list ? "yes" : "no");
				vty_out(vty,
					"    Hello Option - DR Priority     : %s\n",
					option_dr_priority ? "yes" : "no");
				vty_out(vty,
					"    Hello Option - Generation ID   : %s\n",
					option_generation_id ? "yes" : "no");
				vty_out(vty,
					"    Hello Option - Holdtime        : %s\n",
					option_holdtime ? "yes" : "no");
				vty_out(vty,
					"    Hello Option - LAN Prune Delay : %s\n",
					option_lan_prune_delay ? "yes" : "no");
				vty_out(vty,
					"    Hello Option - T-bit           : %s\n",
					option_t_bit ? "yes" : "no");
				bfd_sess_show(vty, json_ifp,
					      neigh->bfd_session);
				vty_out(vty, "\n");
			}
		}
	}

	if (!found_neighbor)
		vty_out(vty, "%% No such interface or neighbor\n");
}

void pim_show_neighbors(struct pim_instance *pim, struct vty *vty,
			json_object *json)
{
	struct listnode *neighnode;
	struct interface *ifp;
	struct pim_interface *pim_ifp;
	struct pim_neighbor *neigh;
	time_t now;
	char uptime[10];
	char expire[10];
	char neigh_src_str[PIM_ADDRSTRLEN];
	json_object *json_ifp_rows = NULL;
	json_object *json_row = NULL;

	now = pim_time_monotonic_sec();

	if (!json) {
		vty_out(vty,
			"Interface                Neighbor    Uptime  Holdtime  DR Pri\n");
	}

	FOR_ALL_INTERFACES (pim->vrf, ifp) {
		pim_ifp = ifp->info;

		if (!pim_ifp)
			continue;

		if (pim_ifp->pim_sock_fd < 0)
			continue;

		if (json)
			json_ifp_rows = json_object_new_object();

		for (ALL_LIST_ELEMENTS_RO(pim_ifp->pim_neighbor_list, neighnode,
					  neigh)) {
			snprintfrr(neigh_src_str, sizeof(neigh_src_str),
				   "%pPAs", &neigh->source_addr);
			pim_time_uptime(uptime, sizeof(uptime),
					now - neigh->creation);
			pim_time_timer_to_hhmmss(expire, sizeof(expire),
						 neigh->t_expire_timer);

			if (json) {
				json_row = json_object_new_object();
				json_object_string_add(json_row, "interface",
						       ifp->name);
				json_object_string_add(json_row, "neighbor",
						       neigh_src_str);
				json_object_string_add(json_row, "upTime",
						       uptime);
				json_object_string_add(json_row, "holdTime",
						       expire);
				json_object_int_add(json_row, "holdTimeMax",
						    neigh->holdtime);
				json_object_int_add(json_row, "drPriority",
						    neigh->dr_priority);
				json_object_object_add(json_ifp_rows,
						       neigh_src_str, json_row);

			} else {
				vty_out(vty, "%-16s  %15s  %8s  %8s  %6d\n",
					ifp->name, neigh_src_str, uptime,
					expire, neigh->dr_priority);
			}
		}

		if (json) {
			json_object_object_add(json, ifp->name, json_ifp_rows);
			json_ifp_rows = NULL;
		}
	}
}

int gm_process_query_max_response_time_cmd(struct vty *vty,
					   const char *qmrt_str)
{
	const struct lyd_node *pim_enable_dnode;

	pim_enable_dnode = yang_dnode_getf(vty->candidate_config->dnode,
					   FRR_PIM_ENABLE_XPATH, VTY_CURR_XPATH,
					   FRR_PIM_AF_XPATH_VAL);

	if (!pim_enable_dnode) {
		nb_cli_enqueue_change(vty, "./enable", NB_OP_MODIFY, "true");
	} else {
		if (!yang_dnode_get_bool(pim_enable_dnode, "."))
			nb_cli_enqueue_change(vty, "./enable", NB_OP_MODIFY,
					      "true");
	}

	nb_cli_enqueue_change(vty, "./query-max-response-time", NB_OP_MODIFY,
			      qmrt_str);
	return nb_cli_apply_changes(vty, FRR_GMP_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int gm_process_no_query_max_response_time_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./query-max-response-time", NB_OP_DESTROY,
			      NULL);
	return nb_cli_apply_changes(vty, FRR_GMP_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int gm_process_last_member_query_count_cmd(struct vty *vty,
					   const char *lmqc_str)
{
	const struct lyd_node *pim_enable_dnode;

	pim_enable_dnode = yang_dnode_getf(vty->candidate_config->dnode,
					   FRR_PIM_ENABLE_XPATH, VTY_CURR_XPATH,
					   FRR_PIM_AF_XPATH_VAL);
	if (!pim_enable_dnode) {
		nb_cli_enqueue_change(vty, "./enable", NB_OP_MODIFY, "true");
	} else {
		if (!yang_dnode_get_bool(pim_enable_dnode, "."))
			nb_cli_enqueue_change(vty, "./enable", NB_OP_MODIFY,
					      "true");
	}

	nb_cli_enqueue_change(vty, "./robustness-variable", NB_OP_MODIFY,
			      lmqc_str);
	return nb_cli_apply_changes(vty, FRR_GMP_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int gm_process_no_last_member_query_count_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./robustness-variable", NB_OP_DESTROY,
			      NULL);
	return nb_cli_apply_changes(vty, FRR_GMP_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int gm_process_last_member_query_interval_cmd(struct vty *vty,
					      const char *lmqi_str)
{
	const struct lyd_node *pim_enable_dnode;

	pim_enable_dnode = yang_dnode_getf(vty->candidate_config->dnode,
					   FRR_PIM_ENABLE_XPATH, VTY_CURR_XPATH,
					   FRR_PIM_AF_XPATH_VAL);
	if (!pim_enable_dnode) {
		nb_cli_enqueue_change(vty, "./enable", NB_OP_MODIFY, "true");
	} else {
		if (!yang_dnode_get_bool(pim_enable_dnode, "."))
			nb_cli_enqueue_change(vty, "./enable", NB_OP_MODIFY,
					      "true");
	}

	nb_cli_enqueue_change(vty, "./last-member-query-interval", NB_OP_MODIFY,
			      lmqi_str);
	return nb_cli_apply_changes(vty, FRR_GMP_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}

int gm_process_no_last_member_query_interval_cmd(struct vty *vty)
{
	nb_cli_enqueue_change(vty, "./last-member-query-interval",
			      NB_OP_DESTROY, NULL);
	return nb_cli_apply_changes(vty, FRR_GMP_INTERFACE_XPATH,
				    FRR_PIM_AF_XPATH_VAL);
}


int pim_process_ssmpingd_cmd(struct vty *vty, enum nb_operation operation,
			     const char *src_str)
{
	const char *vrfname;
	char ssmpingd_ip_xpath[XPATH_MAXLEN];

	vrfname = pim_cli_get_vrf_name(vty);
	if (vrfname == NULL)
		return CMD_WARNING_CONFIG_FAILED;

	snprintf(ssmpingd_ip_xpath, sizeof(ssmpingd_ip_xpath),
		 FRR_PIM_VRF_XPATH, "frr-pim:pimd", "pim", vrfname,
		 FRR_PIM_AF_XPATH_VAL);
	strlcat(ssmpingd_ip_xpath, "/ssm-pingd-source-ip",
		sizeof(ssmpingd_ip_xpath));

	nb_cli_enqueue_change(vty, ssmpingd_ip_xpath, operation, src_str);

	return nb_cli_apply_changes(vty, NULL);
}

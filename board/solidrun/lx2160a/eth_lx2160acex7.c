// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 SolidRun ltd.
 *
 */

#include <common.h>
#include <command.h>
#include <netdev.h>
#include <malloc.h>
#include <fsl_mdio.h>
#include <miiphy.h>
#include <phy.h>
#include <fm_eth.h>
#include <asm/global_data.h>
#include <i2c.h>
#include <tlv_eeprom.h>
#include <asm/io.h>
#include <exports.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch-fsl-layerscape/svr.h>
#include <fsl-mc/fsl_mc.h>
#include <fsl-mc/ldpaa_wriop.h>
#include <fsl-mc/fsl_mc_private.h>

DECLARE_GLOBAL_DATA_PTR;

int select_i2c_ch_pca9547(u8 ch);
int select_i2c_ch_pca9547_n(int busnum, u8 ch);

void setup_retimer_25g(int chnum)
{
	int i, ret;
	u8 reg;
	struct udevice *dev;

	select_i2c_ch_pca9547(0xb); /* SMB_CLK / DATA interface */
	/*
	 * Assumption is that LX2 TX --> RT1 RX is at 0x22 and
	 * RT2 TX --> LX2 RX is at 0x23.
	 */
	ret = i2c_get_chip_for_busnum(0, 0x23, 1, &dev);
	if (ret) {
		/*
		 * On HoneyComb and ClearFog CX ver 1.1 / 1.2 there is no retimer
		 * assembled; silently return.
		 */
		return;
	}
	ret = dm_i2c_read(dev, 0xf1, &reg, 1); /* Get full device ID */
	if (ret) {
		printf ("ERROR: Could not get retimer device ID\n");
		return;
	}
	if (reg != 0x10) {
		printf ("ERROR : DS250DF410 retimer not found\n");
		return;
	}
	if (chnum > 0)
		printf ("Setting up retimer channels 1..%d as 25Gbps\n",chnum);
	if (chnum < 4)
		printf ("Setting up retimer channels %d..4 as 10Gbps\n",chnum+1);

	dm_i2c_reg_write(dev, 0xff, 0x1); /* Enable channel specific access */
	/*
	 * Setup 25Gbps channel on 0..chnum.
	 * Notice that the ingress retimer is mirrorly mapped with the SERDES
	 * number, so SERDES #0 is connected to channel #3, SERDES 1 to channel
	 * #2 ...
	 */
	for (i = 0 ; i < chnum; i++) { /* Setup channels 0..chnum as 25g */
		dm_i2c_reg_write(dev, 0xfc, 1 << i);
		dm_i2c_reg_write(dev, 0x00, 0x4); /* Reset channel registers */
		dm_i2c_reg_write(dev, 0x0a, 0xc); /* Assert CDR reset */
		dm_i2c_reg_write(dev, 0x3d, 0x8f); /* Enable pre/post and set main cursor to 0xf */
		dm_i2c_reg_write(dev, 0x3e, 0x44); /* Set pre-cursor to -4 */
		/* Set post-cursor of channel #0 to -4 */
		dm_i2c_reg_write(dev, 0x3f, 0x44);
		dm_i2c_reg_write(dev, 0x0a, 0x00); /* Release CDR */
	}
	if (chnum < 4) {
		/* Setup the rest of the channels as 10g */
		for (i = chnum ; i < 4; i++) {
			dm_i2c_reg_write(dev, 0xfc, 1 << i);
			dm_i2c_reg_write(dev, 0x00, 0x4); /* Reset channel registers */
			dm_i2c_reg_write(dev, 0x0a, 0xc); /* Assert CDR reset */
			dm_i2c_reg_write(dev, 0x3d, 0x8f); /* Enable pre/post and set main cursor to 0xf */
			dm_i2c_reg_write(dev, 0x3e, 0x44); /* Set pre-cursor to -4 */
			/* Set post-cursor of channel #0 to -4 */
			dm_i2c_reg_write(dev, 0x3f, 0x44);
			dm_i2c_reg_write(dev, 0x2f, 0x04); /* Set rate to 10.3125 Gbps */
			dm_i2c_reg_write(dev, 0x0a, 0x00); /* Release CDR */
		}
		ret = i2c_get_chip_for_busnum(0, 0x22, 1, &dev);
		if (ret) {
			printf ("ERROR: Retimer at address 0x22 not found\n");
			return;
		}
		ret = dm_i2c_read(dev, 0xf1, &reg, 1); /* Get full device ID */
		if (ret) {
			printf ("ERROR: Could not get retimer device ID\n");
			return;
		}
		if (reg != 0x10) {
			printf ("ERROR : DS250DF410 retimer not found\n");
			return;
		}
		dm_i2c_reg_write(dev, 0xff, 0x1); /* Enable channel specific access */
		for (i = chnum ; i < 4; i++) {
			dm_i2c_reg_write(dev, 0xfc, 1 << i);
			dm_i2c_reg_write(dev, 0x00, 0x4); /* Reset channel registers */
			dm_i2c_reg_write(dev, 0x0a, 0xc); /* Assert CDR reset */
			dm_i2c_reg_write(dev, 0x2f, 0x04); /* Set rate to 10.3125 Gbps */
			dm_i2c_reg_write(dev, 0x0a, 0x00); /* Release CDR */
		}
	}
}

void setup_retimer_lx2162_25g(int busnum, int address, int speed_is_25g)
{
	int i, ret;
	u8 reg;
	struct udevice *dev;

	select_i2c_ch_pca9547_n(busnum, 0xb); /* SMB_CLK / DATA interface */
	/*
	 * Retimer on address 0x22; first two channels are LX TX.
	 */
	ret = i2c_get_chip_for_busnum(busnum, address, 1, &dev);
	if (ret) {
		/*
		 * On HoneyComb and ClearFog CX ver 1.1 / 1.2 there is no retimer
		 * assembled; silently return.
		 */
		return;
	}
	ret = dm_i2c_read(dev, 0xf1, &reg, 1); /* Get full device ID */
	if (ret) {
		printf ("ERROR: Could not get retimer device ID\n");
		return;
	}
	if (reg != 0x10) {
		printf ("ERROR : DS250DF410 retimer not found\n");
		return;
	}
	printf ("Setting up retimer channels as %s\n",speed_is_25g?"25Gbps":"10Gbps");

	dm_i2c_reg_write(dev, 0xff, 0x1); /* Enable channel specific access */
	/*
	 * Setup 25Gbps channel on 0..chnum.
	 * Notice that the ingress retimer is mirrorly mapped with the SERDES
	 * number, so SERDES #0 is connected to channel #3, SERDES 1 to channel
	 * #2 ...
	 */
	if (speed_is_25g) for (i = 0 ; i < 4; i++) { /* Setup all channels as 25g */
			dm_i2c_reg_write(dev, 0xfc, 1 << i);
			dm_i2c_reg_write(dev, 0x00, 0x4); /* Reset channel registers */
			dm_i2c_reg_write(dev, 0x0a, 0xc); /* Assert CDR reset */
			dm_i2c_reg_write(dev, 0x3d, 0x8f); /* Enable pre/post and set main cursor to 0xf */
			dm_i2c_reg_write(dev, 0x3e, 0x44); /* Set pre-cursor to -4 */
			/* Set post-cursor of channel #0 to -4 */
			dm_i2c_reg_write(dev, 0x3f, 0x44);
			dm_i2c_reg_write(dev, 0x0a, 0x00); /* Release CDR */
		}
	else for (i = 0 ; i < 4; i++) {
			dm_i2c_reg_write(dev, 0xfc, 1 << i);
			dm_i2c_reg_write(dev, 0x00, 0x4); /* Reset channel registers */
			dm_i2c_reg_write(dev, 0x0a, 0xc); /* Assert CDR reset */
			dm_i2c_reg_write(dev, 0x2f, 0x04); /* Set rate to 10.3125 Gbps */
			dm_i2c_reg_write(dev, 0x0a, 0x00); /* Release CDR */
		}
}

#if defined(CONFIG_RESET_PHY_R)
void reset_phy(void)
{
#if defined(CONFIG_FSL_MC_ENET)
	mc_env_boot();
#endif
}
#endif /* CONFIG_RESET_PHY_R */

int fdt_fixup_board_phy(void *fdt)
{
	int ret;

	ret = 0;

	return ret;
}


#if defined(CONFIG_DM_ETH) && defined(CONFIG_MULTI_DTB_FIT)

/* Structure to hold SERDES protocols supported in case of
 * CONFIG_DM_ETH enabled (network interfaces are described in the DTS).
 *
 * @serdes_block: the index of the SERDES block
 * @serdes_protocol: the decimal value of the protocol supported
 * @dts_needed: DTS notes describing the current configuration are needed
 *
 * When dts_needed is true, the board_fit_config_name_match() function
 * will try to exactly match the current configuration of the block with a DTS
 * name provided.
 */
static struct serdes_configuration {
	u8 serdes_block;
	u32 serdes_protocol;
	bool dts_needed;
} supported_protocols[] = {
	/* Serdes block #1 */
	{1, 0, false},
	{1, 2, true},
	{1, 3, true},
	{1, 8, true},
	{1, 15, true},
	{1, 17, true},
	{1, 18, true},
	{1, 19, true},
	{1, 20, true},

	/* Serdes block #2 */
	{2, 0, false},
	{2, 2, false},
	{2, 3, false},
	{2, 5, false},
	{2, 9, true},
	{2, 10, false},
	{2, 11, true},
	{2, 12, true},
};

#define SUPPORTED_SERDES_PROTOCOLS ARRAY_SIZE(supported_protocols)

static bool protocol_supported(u8 serdes_block, u32 protocol)
{
	struct serdes_configuration serdes_conf;
	int i;

	for (i = 0; i < SUPPORTED_SERDES_PROTOCOLS; i++) {
		serdes_conf = supported_protocols[i];
		if (serdes_conf.serdes_block == serdes_block &&
		    serdes_conf.serdes_protocol == protocol)
			return true;
	}

	return false;
}

static void get_str_protocol(u8 serdes_block, u32 protocol, char *str)
{
	struct serdes_configuration serdes_conf;
	int i;

	for (i = 0; i < SUPPORTED_SERDES_PROTOCOLS; i++) {
		serdes_conf = supported_protocols[i];
		if (serdes_conf.serdes_block == serdes_block &&
		    serdes_conf.serdes_protocol == protocol) {
			if (serdes_conf.dts_needed == true)
				sprintf(str, "%u", protocol);
			else
				sprintf(str, "x");
			return;
		}
	}
}

int board_fit_config_name_match(const char *name)
{
	struct ccsr_gur *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 srds_s1, srds_s2;
	u32 rcw_status = in_le32(&gur->rcwsr[28]);

	srds_s1 = rcw_status & FSL_CHASSIS3_RCWSR28_SRDS1_PRTCL_MASK;
	srds_s1 >>= FSL_CHASSIS3_RCWSR28_SRDS1_PRTCL_SHIFT;

	srds_s2 = rcw_status & FSL_CHASSIS3_RCWSR28_SRDS2_PRTCL_MASK;
	srds_s2 >>= FSL_CHASSIS3_RCWSR28_SRDS2_PRTCL_SHIFT;
	if (get_svr() & 0x800) {
		if (!strncmp(name, "fsl-lx2162a-som", 15)) {
			return 0;
		}
	} else {
		/* Notice - order of matching is similar to order of CONFIG_OF_LIST */
		if (srds_s1 == 8) {
			if (srds_s2 == 9) { /* Half-twins board */
				if (!strncmp(name, "fsl-lx2160a-half-twins-8-9-x", 28)) {
					return 0;
				}
			}
			if (srds_s2 <= 5) { /* No network with SD2 <= 5 */
				if (!strncmp(name, "fsl-lx2160a-cex7-8-x-x", 22)) {
					return 0;
				}
			}
		}
		if (!strncmp(name, "fsl-lx2160a-cex7", 16)) return 0;
	}

	return -1;
}

int fsl_board_late_init(void) {
	struct ccsr_gur *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 rcw_status = in_le32(&gur->rcwsr[28]);
	char srds_s1_str[2], srds_s2_str[2];
	u32 srds_s1, srds_s2;
	char expected_dts[100];

	printf ("fsl_board_late_init\n");

	srds_s1 = rcw_status & FSL_CHASSIS3_RCWSR28_SRDS1_PRTCL_MASK;
	srds_s1 >>= FSL_CHASSIS3_RCWSR28_SRDS1_PRTCL_SHIFT;

	srds_s2 = rcw_status & FSL_CHASSIS3_RCWSR28_SRDS2_PRTCL_MASK;
	srds_s2 >>= FSL_CHASSIS3_RCWSR28_SRDS2_PRTCL_SHIFT;


	/* Check for supported protocols. The default DTS will be used
	 * in this case
	 */
	if (!protocol_supported(1, srds_s1) ||
	    !protocol_supported(2, srds_s2))
		return 0;

	get_str_protocol(1, srds_s1, srds_s1_str);
	get_str_protocol(2, srds_s2, srds_s2_str);

	if (get_svr() & 0x800) { /* LX2162A SOM variants */
		switch (srds_s1) {
			case 3:
			case 20:
				/* Setup retimer on lanes e,f for 10Gbps */
				setup_retimer_lx2162_25g(0, 0x22, 0);
				setup_retimer_lx2162_25g(1, 0x18, 0);
				break;
			case 15:
			case 16:
			case 17:
			case 18:
				/* Setup retimer on lanes e,f for 25Gbps */
				setup_retimer_lx2162_25g(0, 0x22, 1);
				setup_retimer_lx2162_25g(1, 0x18, 1);
				break;
			default:
				printf("SerDes1 protocol 0x%x is not supported on LX2162A-SOM\n",
				       srds_s1);
		}
		if ((srds_s1 == 18) && ((srds_s2 == 2) || (srds_s2 == 3)))
			/* SolidNet device tree */
			sprintf(expected_dts, "fsl-lx2162a-solidnet.dtb");
		else sprintf(expected_dts, "fsl-lx2162a-som-%s-%s.dtb",
			srds_s1_str, srds_s2_str);
	} else {
		switch (srds_s1) {
			case 8:
			case 20:
				/* Setup retimer on lanes e,f,g,h for 10Gbps */
				setup_retimer_25g(0);
				break;
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 21:
				/* Setup 25gb retimer on lanes e,f,g,h */
				setup_retimer_25g(4);
				break;
			case 18:
			case 19:
				/* Setup 25gb retimer on lanes e,f and 10g on g,h */
				setup_retimer_25g(2);
				break;
			default:
			printf("SerDes1 protocol 0x%x is not supported on LX2160ACEX7\n",
			       srds_s1);
		}
		if ((srds_s1 == 8) && ((srds_s2 == 9) || (srds_s2 == 2))) {
			sprintf(expected_dts, "fsl-lx2160a-half-twins.dtb");
		} else sprintf(expected_dts, "fsl-lx2160a-clearfog-cx.dtb");
	}
	if (!env_get("fdtfile"))
		env_set("fdtfile", expected_dts);
	return 0;
}
#endif

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 SolidRun ltd.
 */

#ifndef __LX2_CEX7_H
#define __LX2_CEX7_H

#include "lx2160a_common.h"

/*#define CONFIG_SYS_FSL_ESDHC_USE_PIO*/
/* VID */

#define I2C_MUX_CH_VOL_MONITOR		0xa /* Channel 2 */
/* Voltage monitor on channel 2*/
#define I2C_VOL_MONITOR_ADDR		0x5c
#define I2C_VOL_MONITOR_BUS_V_OFFSET	0x2
#define I2C_VOL_MONITOR_BUS_V_OVF	0x1
#define I2C_VOL_MONITOR_BUS_V_SHIFT	3
#define CONFIG_VID_FLS_ENV		"lx2160acex7_vdd_mv"
#define CONFIG_VID

/* The lowest and highest voltage allowed*/
#define VDD_MV_MIN			700
#define VDD_MV_MAX			855

/* PM Bus commands code for LTC3882*/
#define PMBUS_CMD_PAGE                  0x0
#define PMBUS_CMD_READ_VOUT             0x8B
#define PMBUS_CMD_PAGE_PLUS_WRITE       0x05
#define PMBUS_CMD_VOUT_COMMAND          0x21
#define PWM_CHANNEL0                    0x0

#define CONFIG_VOL_MONITOR_LTC3882_SET
#define CONFIG_VOL_MONITOR_LTC3882_READ

/* RTC */
#define CONFIG_SYS_RTC_BUS_NUM		4

/* MAC/PHY configuration */
#if defined(CONFIG_FSL_MC_ENET)
#define CONFIG_MII
#define CONFIG_ETHPRIME		"DPMAC17@rgmii-id"

#define RGMII_PHY_ADDR1		0x01

#endif


/* EEPROM */
#undef CONFIG_ID_EEPROM /* We use TLV with I2C DM */

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	EXTRA_ENV_SETTINGS			\
	"lx2160acex7_vdd_mv=800\0"		\
	"BOARD=lx2160acex7\0"			\
	"xspi_bootcmd=echo Trying load from flexspi..;"		\
		"sf probe 0:0 && sf read $load_addr $kernel_start $kernel_size;" \
		"bootm $load_addr\0"			  \
	"usb_bootcmd=echo Trying USB kernel...; "         \
		"usb start; usb dev 0 && usb info && "    \
		"usb read 0xa0000000 0x8000 0x18000 && "  \
		"source 0xa0000000:usb_bootscript && "    \
		"bootm 0xa0000000\0"                      \
	"sd_bootcmd=echo Trying SD-Card kernel...; "      \
		"mmc dev 0 && mmcinfo && "		  \
		"mmc read 0xa0000000 0x8000 0x18000 && "  \
		"source 0xa0000000:sd_bootscript && "     \
		"bootm 0xa0000000\0"                      \
	"emmc_bootcmd=echo Trying eMMC kernel...; "       \
		"mmc dev 1; mmcinfo; "			  \
		"mmc read 0xa0000000 0x8000 0x18000; "    \
		"source 0xa0000000:emmc_bootscript; "     \
		"bootm 0xa0000000\0"                      \
	"nvme_bootcmd=echo Trying NVMe kernel...; "       \
		"source 0xa0000000:nvme_bootscript; "     \
		"bootm 0xa0000000\0"                      \
	"dhcp_bootcmd=echo Trying DHCP kernel...; "       \
		"source 0xa0000000:dhcp_bootscript; "     \
		"bootm 0xa0000000\0"                      \
	"qm_boot=mmcinfo; "				  \
		"mmc read 0x80d00000 0x6800 0x800; "	  \
		"fsl_mc lazyapply dpl 0x80d00000; "	  \
		"run usb_bootcmd;"                        \
		"run sd_bootcmd;"                         \
		"run emmc_bootcmd;"                       \
		"run nvme_bootcmd;"                       \
		"run dhcp_bootcmd\0"

#include <asm/fsl_secure_boot.h>
#undef CONFIG_SYS_MEMTEST_START
#undef CONFIG_SYS_MEMTEST_END
#define CONFIG_SYS_MEMTEST_START		0x2080000000
#define CONFIG_SYS_MEMTEST_END			0x2400000000

#endif /* __LX2_CEX7_H */

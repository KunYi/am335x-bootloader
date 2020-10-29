/*
 * am335x_evm.h
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_SRG3352_H
#define __CONFIG_SRG3352_H

#include <configs/ti_am335x_common.h>
#include <linux/sizes.h>

/*
 *	remove TI default macro
 *  and re-definition them
 */
#undef DEFAULT_LINUX_BOOT_ENV
#define DEFAULT_LINUX_BOOT_ENV \
	"loadaddr=0x82000000\0" \
	"kernel_addr_r=0x82000000\0" \
	"fdtaddr=0x8c000000\0" \
	"fdt_addr_r=0x8c000000\0" \
	"rdaddr=0x8c080000\0" \
	"ramdisk_addr_r=0x8c080000\0" \
	"scriptaddr=0x80000000\0" \
	"pxefile_addr_r=0x80100000\0" \
	"bootm_size=0x10000000\0" \
	"boot_fdt=try\0"

#ifndef CONFIG_SPL_BUILD
# define CONFIG_TIMESTAMP
#endif

#define CONFIG_SYS_BOOTM_LEN		SZ_16M

#define CONFIG_MACH_TYPE		MACH_TYPE_SRG3352

/* Clock Defines */
#define V_OSCK				24000000  /* Clock output from T2 */
#define V_SCLK				(V_OSCK)

#define CONFIG_SYS_LDSCRIPT		"board/aaeon/srg3352/u-boot.lds"
/* enable EMMC_BOOT for enviroments */
#define CONFIG_EMMC_BOOT
/* Always 128 KiB env size */
#define CONFIG_ENV_SIZE			SZ_128K

#define BOOTENV_DEV_LEGACY_MMC(devtypeu, devtypel, instance) \
	"bootcmd_" #devtypel #instance "=" \
	"setenv mmcdev " #instance"; "\
	"setenv bootpart " #instance":2 ; "\
	"run mmcboot\0"

#define BOOTENV_DEV_NAME_LEGACY_MMC(devtypeu, devtypel, instance) \
	#devtypel #instance " "

#define BOOTENV_DEV_NAND(devtypeu, devtypel, instance) \
	"bootcmd_" #devtypel "=" \
	"run nandboot\0"

#define BOOTENV_DEV_NAME_NAND(devtypeu, devtypel, instance) \
	#devtypel #instance " "

#if CONFIG_IS_ENABLED(CMD_PXE)
# define BOOT_TARGET_PXE(func) func(PXE, pxe, na)
#else
# define BOOT_TARGET_PXE(func)
#endif

#if CONFIG_IS_ENABLED(CMD_DHCP)
# define BOOT_TARGET_DHCP(func) func(DHCP, dhcp, na)
#else
# define BOOT_TARGET_DHCP(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(LEGACY_MMC, legacy_mmc, 0) \
	func(MMC, mmc, 1) \
	func(LEGACY_MMC, legacy_mmc, 1) \
	BOOT_TARGET_PXE(func) \
	BOOT_TARGET_DHCP(func)

#include <config_distro_bootcmd.h>

#define SRG52_BOOTENV \
	BOOTENV_SHARED_HOST \
	BOOTENV_SHARED_MMC \
	BOOTENV_SHARED_USB \
	BOOTENV_SHARED_EFI \
	BOOTENV_SHARED_VIRTIO \
	"boot_prefixes=/boot/ /\0" \
	"boot_scripts=boot.scr.uimg boot.scr\0" \
	"boot_script_dhcp=boot.scr.uimg\0" \
	BOOTENV_BOOT_TARGETS \
	\
	"boot_a_script="                                                  \
		"load ${devtype} ${devnum}:${distro_bootpart} "           \
			"${scriptaddr} ${prefix}${script}; "              \
		"source ${scriptaddr}\0"                                  \
	\
	"scan_dev_for_scripts="                                           \
		"for script in ${boot_scripts}; do "                      \
			"if test -e ${devtype} "                          \
					"${devnum}:${distro_bootpart} "   \
					"${prefix}${script}; then "       \
				"echo Found U-Boot script "               \
					"${prefix}${script}; "            \
				"run boot_a_script; "                     \
				"echo SCRIPT FAILED: continuing...; "     \
			"fi; "                                            \
		"done\0"                                                  \
	\
	"scan_dev_for_boot="                                              \
		"echo debug -- scan_dev_for_boot"                           \
		"echo Scanning ${devtype} "                               \
				"${devnum}:${distro_bootpart}...; "       \
		"for prefix in ${boot_prefixes}; do "                     \
			"run scan_dev_for_scripts; "                      \
		"done;"                                                   \
		SCAN_DEV_FOR_EFI                                          \
		"\0"                                                      \
	\
	"scan_dev_for_boot_part="                                         \
		"part list ${devtype} ${devnum} -bootable devplist; "     \
		"env exists devplist || setenv devplist 1; "              \
		"for distro_bootpart in ${devplist}; do "                 \
			"if fstype ${devtype} "                           \
					"${devnum}:${distro_bootpart} "   \
					"bootfstype; then "               \
				"run scan_dev_for_boot; "                 \
			"fi; "                                            \
		"done\0"                                                  \
	\
	BOOT_TARGET_DEVICES(BOOTENV_DEV)                                  \
	\
	"distro_bootcmd="                       \
		"echo debug -- run distro_bootcmd; " \
		"for target in ${boot_targets}; do "                      \
			"run bootcmd_${target}; "                         \
		"done\0"

#ifndef CONFIG_SPL_BUILD
#include <environment/ti/dfu.h>
#include <environment/ti/mmc.h>

#define DEFAULT_MMC_SRG52_ARGS \
	"mmcdev=0\0" \
	"mmcrootfstype=ext4 rootwait\0" \
	"finduuid=" \
		"echo debug -- run finduuid; " \
		"mmc dev ${devnum}; mmc part;"	\
		"setexpr rootpart ${distro_bootpart} + 1;" \
		"part uuid mmc ${devnum}:${rootpart} uuid;" \
		"echo debug -- rootpart:${rootpart} uuid: ${uuid}\0" \
	"args_mmc=run finduuid;setenv bootargs console=${console} " \
		"${optargs} " \
		"root=PARTUUID=${uuid} rw " \
		"rootfstype=${mmcrootfstype}\0" \
	"loadbootscript="\
		"echo debug -- run loadbootscrip; " \
		"load mmc ${mmcdev} ${loadaddr} ${bootdir}/boot.scr\0" \
	"bootscript=echo Running bootscript from mmc${mmcdev} ...; " \
		"source ${loadaddr}\0" \
	"bootenvfile=${bootdir}/uEnv.txt\0" \
	"importbootenv=echo Importing environment from mmc${mmcdev} ...; " \
		"env import -t ${loadaddr} ${filesize}\0" \
	"loadbootenv=" \
		"echo debug -- run loadbootenv;" \
		"load mmc ${mmcdev} ${loadaddr} ${bootenvfile}\0" \
	"loadimage=load ${devtype} ${bootpart} ${loadaddr} ${bootdir}/${bootfile}\0" \
	"loadfdt=load ${devtype} ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile}\0" \
	"envboot=" \
		"echo debug -- run envboot, mmcdev:${mmcdev};" \
		"mmc dev ${mmcdev}; " \
		"if mmc rescan; then " \
			"echo SD/MMC found on device ${mmcdev};" \
			"if run loadbootscript; then " \
				"run bootscript;" \
			"else " \
				"if run loadbootenv; then " \
					"echo Loaded env from ${bootenvfile};" \
					"run importbootenv;" \
				"fi;" \
				"if test -n $uenvcmd; then " \
					"echo Running uenvcmd ...;" \
					"run uenvcmd;" \
				"fi;" \
			"fi;" \
		"fi;\0" \
	"mmcloados= " \
		"echo debug -- run mmcloados;" \
		"run args_mmc; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdtaddr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
	"mmcboot=" \
		"echo debug -- run mmcboot;" \
		"mmc dev ${mmcdev}; " \
		"setenv devnum ${mmcdev}; " \
		"setenv devtype mmc; " \
		"if mmc rescan; then " \
			"echo SD/MMC found on device ${mmcdev};" \
			"if run loadimage; then " \
				"if test ${boot_fit} -eq 1; then " \
					"run loadfit; " \
				"else " \
					"run mmcloados;" \
				"fi;" \
			"fi;" \
		"fi;\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	DEFAULT_LINUX_BOOT_ENV \
	DEFAULT_MMC_SRG52_ARGS \
	DEFAULT_FIT_TI_ARGS \
	"bootpart=0:2\0" \
	"bootdir=/boot\0" \
	"bootfile=zImage\0" \
	"fdtfile=undefined\0" \
	"console=ttyS0,115200n8\0" \
	"partitions=" \
		"uuid_disk=${uuid_gpt_disk};" \
		"name=bootloader,start=384K,size=1792K," \
			"uuid=${uuid_gpt_bootloader};" \
		"name=rootfs,start=128M,size=-,uuid=${uuid_gpt_rootfs}\0" \
	"optargs=\0" \
	"ramroot=/dev/ram0 rw\0" \
	"ramrootfstype=ext4\0" \
	"ramargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"root=${ramroot} " \
		"rootfstype=${ramrootfstype}\0" \
	"loadramdisk=load mmc ${mmcdev} ${rdaddr} ramdisk.gz\0" \
	"ramboot=echo Booting from ramdisk ...; " \
		"run ramargs; " \
		"bootz ${loadaddr} ${rdaddr} ${fdtaddr}\0" \
	"findfdt="\
		"echo board_name=$board_name ...; " \
		"if test $board_name = srg3352; then " \
			"setenv fdtfile am335x-srg3352.dtb; " \
		"elif test $board_name = srg-3352; then " \
			"setenv fdtfile am335x-srg3352.dtb; " \
		"elif test $board_name = SRG3352; then " \
			"setenv fdtfile am335x-srg3352.dtb; " \
		"elif test $board_name = SRG-3352; then " \
			"setenv fdtfile am335x-srg3352.dtb; " \
		"elif test $board_name = srg3352c; then " \
			"setenv fdtfile am335x-srg3352c.dtb; " \
		"elif test $board_name = srg-3352c; then " \
			"setenv fdtfile am335x-srg3352c.dtb; " \
		"elif test $board_name = SRG3352C; then " \
			"setenv fdtfile am335x-srg3352c.dtb; " \
		"elif test $board_name = SRG-3352C; then " \
			"setenv fdtfile am335x-srg3352c.dtb; " \
		"fi; " \
		"if test $fdtfile = undefined; then " \
			"echo WARNING: Could not determine device tree to use; fi; \0" \
	"init_console=" \
			"setenv console ttyS0,115200n8; \0" \
	NETARGS \
	DFUARGS \
	SRG52_BOOTENV
#endif

/* NS16550 Configuration */
#define CONFIG_SYS_NS16550_COM1		0x44e09000	/* Base EVM has UART0 */
#define CONFIG_SYS_NS16550_COM2		0x48022000	/* UART1 */
#define CONFIG_SYS_NS16550_COM3		0x48024000	/* UART2 */
#define CONFIG_SYS_NS16550_COM4		0x481a6000	/* UART3 */
#define CONFIG_SYS_NS16550_COM5		0x481a8000	/* UART4 */
#define CONFIG_SYS_NS16550_COM6		0x481aa000	/* UART5 */

#define CONFIG_ENV_EEPROM_IS_ON_I2C
#define CONFIG_SYS_I2C_EEPROM_ADDR      0x57	/* Main EEPROM */
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	1

/* PMIC support */
#define CONFIG_POWER_TPS65217
#define CONFIG_POWER_TPS65910

/* SPL */
/* Bootcount using the RTC block */
#define CONFIG_SYS_BOOTCOUNT_BE
/* USB gadget RNDIS */


/*
 * USB configuration.  We enable MUSB support, both for host and for
 * gadget.  We set USB0 as peripheral and USB1 as host, based on the
 * board schematic and physical port wired to each.  Then for host we
 * add mass storage support and for gadget we add both RNDIS ethernet
 * and DFU.
 */
#define CONFIG_USB_MUSB_DISABLE_BULK_COMBINE_SPLIT
#define CONFIG_AM335X_USB0
#define CONFIG_AM335X_USB0_MODE	MUSB_PERIPHERAL
#define CONFIG_AM335X_USB1
#define CONFIG_AM335X_USB1_MODE MUSB_HOST

/*
 * Disable MMC DM for SPL build and can be re-enabled after adding
 * DM support in SPL
 */
#ifdef CONFIG_SPL_BUILD
#undef CONFIG_DM_MMC
#undef CONFIG_TIMER
#endif

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_USB_ETHER)
/* Remove other SPL modes. */
/* disable host part of MUSB in SPL */
/* disable EFI partitions and partition UUID support */
#endif

/* USB Device Firmware Update support */
#ifndef CONFIG_SPL_BUILD
#define DFUARGS \
	DFU_ALT_INFO_EMMC \
	DFU_ALT_INFO_MMC \
	DFU_ALT_INFO_RAM
#endif

#ifndef CONFIG_SPL_BUILD
#define CONFIG_CMD_SETEXPR
#endif

#if defined(CONFIG_EMMC_BOOT)
#define CONFIG_SYS_MMC_ENV_DEV	1
#define CONFIG_SYS_MMC_ENV_PART	0
#define CONFIG_ENV_OFFSET		(1 * 1024 * 1024)
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_SYS_MMC_MAX_DEVICE	2
#endif

/* SPI flash. */
#define CONFIG_SF_DEFAULT_SPEED		24000000

/* Network. */
#define CONFIG_PHY_SMSC
/* Enable Atheros phy driver */
#define CONFIG_PHY_ATHEROS

#ifdef CONFIG_DRIVER_TI_CPSW
#define CONFIG_CLOCK_SYNTHESIZER
#define CLK_SYNTHESIZER_I2C_ADDR 0x65
#endif

#endif	/* ! __CONFIG_SRG3352_H */

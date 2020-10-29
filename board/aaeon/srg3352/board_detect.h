/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Library to support AAEON SRG3352 family
 *
 * Copyright (C) LYD/AAEON Technology Inc.
 */

#ifndef __BOARD_DETECT_H
#define __BOARD_DETECT_H

#define SRG52_EEPROM_HDR_MAGIC      0x544F4941	 /* 'AIOT'  */
#define SRG52_EEPROM_HDR_DEAD       0x44414544	 /* 'DEAD'  */
#define SRG52_EEPROM_HDR_REV01      0x3130       /* '01'    */
#define SRG52_EEPROM_HDR_REV        (SRG52_EEPROM_HDR_REV01)

#define SRG52_EEPROM_HDR_ETH_ALEN   (6)
#define SRG52_EEPROM_HDR_BRD_LEN    (32)
#define SRG52_EEPROM_HDR_REV_LEN    (2)
#define SRG52_EEPROM_HDR_SN_LEN     (16)
#define SRG52_EEPROM_HDR_MNF_LEN    (32)
#define SRG52_EEPROM_HDR_HWREV_LEN  (2)
#define SRG52_EEPROM_HDR_HWCFG_LEN  (4)
#define SRG52_EEPROM_HDR_SWVER_LEN  (2)
#define SRG52_EEPROM_HDR_SWCFG_LEN  (4)
#define SRG52_EEPROM_HDR_RVSCS_LEN  (32)
#define SRG52_EEPROM_HDR_BTCNT_LEN  (4)
#define SRG52_EEPROM_HDR_CRC16_LEN  (2)
#define SRG52_EEPROM_HDR_NO_OF_MAC_ADDR (4)

struct  srg52_eeprom_t {
	u32		magic;						/* header magic numver */
	u8		rev[2];						/* header revision, 01 */
	char		bname[SRG52_EEPROM_HDR_BRD_LEN];		/* board name */
	char		serial[SRG52_EEPROM_HDR_SN_LEN];		/* board serial number */
	char		manufacturer[SRG52_EEPROM_HDR_MNF_LEN];		/* manufacture name */
	u8		macEth0[SRG52_EEPROM_HDR_ETH_ALEN];		/* eth0 address */
	u8		macEth1[SRG52_EEPROM_HDR_ETH_ALEN];		/* eth1 address */
	u8		macBt[SRG52_EEPROM_HDR_ETH_ALEN];
	u8		macWlan[SRG52_EEPROM_HDR_ETH_ALEN];
	u8		hwRev[SRG52_EEPROM_HDR_HWREV_LEN];		/* hardware revision */
	u32		hwCfg;						/* hardware configuration */
	u8		swVer[SRG52_EEPROM_HDR_SWVER_LEN];		/* software verision */
	u32		swCfg;						/* software configuration */
	u8		reservedCus[SRG52_EEPROM_HDR_RVSCS_LEN];/* reserve for customer */
	u32		bootCount;					/* Boot counter */
	u16		crc16;
} __attribute__((__packed__));

#define SRG52_EEPROM_SIZE	sizeof(struct srg52_eeprom_t)
#define SRG52_EEPROM_DATA   ((struct srg52_eeprom_t *)\
                               TI_SRAM_SCRATCH_BOARD_EEPROM_START)
/**
 * set_project_info_env() - Setup default project info environment vars
 *
 */
void set_project_info_env(void);
/**
 * set_board_info_env() - Setup commonly used board information environment vars
 * @name:	Name of the board
 *
 * If name is NULL, default_name is used.
 */
void set_board_info_env(char *name);

/**
 *
 *
 */
int srg52_i2c_eeprom_get(int bus_addr, int dev_addr);
/**
 * board_srg52_set_ethaddr- Sets the ethaddr environment from EEPROM
 * @index: The first eth<index>addr environment variable to set
 *
 * EEPROM should be already read before calling this function.
 * The EEPROM contains 4 MAC addresses which define the MAC address
 * range (i.e. first and last MAC address).
 * This function sets the ethaddr environment variable for all
 * the available MAC addresses starting from eth<index>addr.
 */
void board_srg52_set_ethaddr(int index);

/**
 * board_srg52_was_eeprom_read() - Check to see if the eeprom contents have been read
 *
 * This function is useful to determine if the eeprom has already been read and
 * its contents have already been loaded into memory. It utiltzes the magic
 * number that the header value is set to upon successful eeprom read.
 */
bool board_srg52_was_eeprom_read(void);

#endif	/* __BOARD_DETECT_H */

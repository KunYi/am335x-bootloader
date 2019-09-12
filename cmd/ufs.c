// SPDX-License-Identifier: GPL-2.0+
/**
 * ufs.c - UFS specific U-boot commands
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com
 *
 */
#include <common.h>
#include <command.h>
#include <ufs.h>

static int do_ufs(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc >= 2) {
		if (!strcmp(argv[1], "init")) {
			ufs_probe();

			return CMD_RET_SUCCESS;
		}
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(ufs, 2, 1, do_ufs,
	   "UFS  sub system",
	   "init - init UFS subsystem\n"
);

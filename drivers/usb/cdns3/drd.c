// SPDX-License-Identifier: GPL-2.0
/*
 * Cadence USBSS DRD Driver.
 *
 * Copyright (C) 2018-2019 Cadence.
 *
 * Author: Pawel Laszczak <pawell@cadence.com>
 *
 */
#include <dm.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/usb/otg.h>

#include "drd.h"
#include "core.h"

/**
 * cdns3_drd_switch_host - start/stop host
 * @cdns: Pointer to controller context structure
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
int cdns3_drd_switch_host(struct cdns3 *cdns, int on)
{
	int ret;
	u32 reg = OTGCMD_OTG_DIS;

	/* switch OTG core */
	if (on) {
		writel(OTGCMD_HOST_BUS_REQ | reg, &cdns->otg_regs->cmd);

		dev_dbg(cdns->dev, "Waiting till Host mode is turned on\n");
		ret = cdns3_handshake(&cdns->otg_regs->sts, OTGSTS_XHCI_READY,
				      OTGSTS_XHCI_READY, 100000);

		if (ret) {
			dev_err(cdns->dev, "timeout waiting for xhci_ready\n");
			return ret;
		}
	} else {
		writel(OTGCMD_HOST_BUS_DROP | OTGCMD_DEV_BUS_DROP |
		       OTGCMD_DEV_POWER_OFF | OTGCMD_HOST_POWER_OFF,
		       &cdns->otg_regs->cmd);
		/* Waiting till H_IDLE state.*/
		cdns3_handshake(&cdns->otg_regs->state,
				OTGSTATE_HOST_STATE_MASK,
				0, 2000000);
	}

	return 0;
}

int cdns3_drd_init(struct cdns3 *cdns)
{
	void __iomem *regs;
	u32 state;

	regs = dev_remap_addr_name(cdns->dev, "otg");
	if (!regs)
		return -EINVAL;

	/* Detection of DRD version. Controller has been released
	 * in two versions. Both are similar, but they have same changes
	 * in register maps.
	 * The first register in old version is command register and it's read
	 * only, so driver should read 0 from it. On the other hand, in v1
	 * the first register contains device ID number which is not set to 0.
	 * Driver uses this fact to detect the proper version of
	 * controller.
	 */
	cdns->otg_v0_regs = regs;
	if (!readl(&cdns->otg_v0_regs->cmd)) {
		cdns->version  = CDNS3_CONTROLLER_V0;
		cdns->otg_v1_regs = NULL;
		cdns->otg_regs = regs;
		writel(1, &cdns->otg_v0_regs->simulate);
		dev_info(cdns->dev, "DRD version v0 (%08x)\n",
			 readl(&cdns->otg_v0_regs->version));
	} else {
		cdns->otg_v0_regs = NULL;
		cdns->otg_v1_regs = regs;
		cdns->otg_regs = (void *)&cdns->otg_v1_regs->cmd;
		cdns->version  = CDNS3_CONTROLLER_V1;
		writel(1, &cdns->otg_v1_regs->simulate);
		dev_info(cdns->dev, "DRD version v1 (ID: %08x, rev: %08x)\n",
			 readl(&cdns->otg_v1_regs->did),
			 readl(&cdns->otg_v1_regs->rid));
	}

	state = readl(&cdns->otg_regs->sts);
	if (OTGSTS_OTG_NRDY(state) != 0) {
		dev_err(cdns->dev, "Cadence USB3 OTG device not ready\n");
		return -ENODEV;
	}

	return 0;
}

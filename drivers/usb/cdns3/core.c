// SPDX-License-Identifier: GPL-2.0
/*
 * Cadence USBSS DRD Driver.
 *
 * Copyright (C) 2018-2019 Cadence.
 * Copyright (C) 2017-2018 NXP
 *
 * Author: Peter Chen <peter.chen@nxp.com>
 *         Pawel Laszczak <pawell@cadence.com>
 */

#include <dm.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <usb.h>
#include <usb/xhci.h>

#include "core.h"
#include "drd.h"

struct cdns3_host_priv {
	struct xhci_ctrl xhci_ctrl;
	struct cdns3 cdns;
};

/**
 * cdns3_handshake - spin reading  until handshake completes or fails
 * @ptr: address of device controller register to be read
 * @mask: bits to look at in result of read
 * @done: value of those bits when handshake succeeds
 * @usec: timeout in microseconds
 *
 * Returns negative errno, or zero on success
 *
 * Success happens when the "mask" bits have the specified value (hardware
 * handshake done). There are two failure modes: "usec" have passed (major
 * hardware flakeout), or the register reads as all-ones (hardware removed).
 */
int cdns3_handshake(void __iomem *ptr, u32 mask, u32 done, int usec)
{
	u32 result;

	do {
		result = readl(ptr);
		if (result == ~(u32)0)  /* card removed */
			return -ENODEV;

		result &= mask;
		if (result == done)
			return 0;

		udelay(1);
		usec--;
	} while (usec > 0);

	return -ETIMEDOUT;
}

static int cdns3_probe(struct udevice *dev)
{
	struct cdns3_host_priv *priv = dev_get_priv(dev);
	struct cdns3 *cdns = &priv->cdns;
	struct xhci_hcor *hcor;
	struct xhci_hccr *hccr;
	int ret;

	cdns->dev = dev;

	cdns->xhci_regs = dev_remap_addr_name(dev, "xhci");
	if (!cdns->xhci_regs)
		return -EINVAL;

	ret = cdns3_drd_init(cdns);
	if (ret)
		return ret;

	ret = cdns3_drd_switch_host(cdns, 1);
	if (ret)
		return ret;

	hccr = (struct xhci_hccr *)cdns->xhci_regs;
	hcor = (struct xhci_hcor *)(cdns->xhci_regs +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	return xhci_register(dev, hccr, hcor);
}

static int cdns3_remove(struct udevice *dev)
{
	struct cdns3_host_priv *priv = dev_get_priv(dev);
	struct cdns3 *cdns = &priv->cdns;

	xhci_deregister(dev);
	return cdns3_drd_switch_host(cdns, 0);
}

static const struct udevice_id cdns3_ids[] = {
	{ .compatible = "cdns,usb3-1.0.0" },
	{ .compatible = "cdns,usb3-1.0.1" },
	{ },
};

U_BOOT_DRIVER(cdns_usb3_host) = {
	.name	= "cdns-usb3-host",
	.id	= UCLASS_USB,
	.of_match = cdns3_ids,
	.probe = cdns3_probe,
	.remove = cdns3_remove,
	.priv_auto_alloc_size = sizeof(struct cdns3_host_priv),
	.ops = &xhci_usb_ops,
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};

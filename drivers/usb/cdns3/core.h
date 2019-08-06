/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Cadence USBSS DRD Header File.
 *
 * Copyright (C) 2017-2018 NXP
 * Copyright (C) 2018-2019 Cadence.
 *
 * Authors: Peter Chen <peter.chen@nxp.com>
 *          Pawel Laszczak <pawell@cadence.com>
 */
#include <common.h>
#include <linux/usb/otg.h>

#ifndef __LINUX_CDNS3_CORE_H
#define __LINUX_CDNS3_CORE_H

/**
 * struct cdns3 - Representation of Cadence USB3 DRD controller.
 * @dev: pointer to Cadence device struct
 * @xhci_regs: pointer to base of xhci registers
 * @xhci_res: the resource for xhci
 * @dev_regs: pointer to base of dev registers
 * @otg_res: the resource for otg
 * @otg_v0_regs: pointer to base of v0 otg registers
 * @otg_v1_regs: pointer to base of v1 otg registers
 * @otg_regs: pointer to base of otg registers
 * @otg_irq: irq number for otg controller
 * @dev_irq: irq number for device controller
 * @roles: array of supported roles for this controller
 * @role: current role
 * @host_dev: the child host device pointer for cdns3 core
 * @gadget_dev: the child gadget device pointer for cdns3 core
 * @usb: phy for this controller
 * @role_switch_wq: work queue item for role switch
 * @in_lpm: the controller in low power mode
 * @wakeup_int: the wakeup interrupt
 * @mutex: the mutex for concurrent code at driver
 * @dr_mode: supported mode of operation it can be only Host, only Device
 *           or OTG mode that allow to switch between Device and Host mode.
 *           This field based on firmware setting, kernel configuration
 *           and hardware configuration.
 * @current_dr_mode: current mode of operation when in dual-role mode
 * @desired_dr_mode: desired mode of operation when in dual-role mode.
 *           This value can be changed during runtime.
 *           Available options depends on  dr_mode:
 *           dr_mode                 |  desired_dr_mode and current_dr_mode
 *           ----------------------------------------------------------------
 *           USB_DR_MODE_HOST        | only USB_DR_MODE_HOST
 *           USB_DR_MODE_PERIPHERAL  | only USB_DR_MODE_PERIPHERAL
 *           USB_DR_MODE_OTG         | USB_DR_MODE_OTG or USB_DR_MODE_HOST or
 *                                   | USB_DR_MODE_PERIPHERAL
 *           Desired_dr_role can be changed by means of debugfs.
 * @root: debugfs root folder pointer
 * @debug_disable:
 */
struct cdns3 {
	struct udevice			*dev;
	void __iomem			*xhci_regs;
	struct cdns3_usb_regs __iomem	*dev_regs;

	struct cdns3_otg_legacy_regs	*otg_v0_regs;
	struct cdns3_otg_regs		*otg_v1_regs;
	struct cdns3_otg_common_regs	*otg_regs;
#define CDNS3_CONTROLLER_V0	0
#define CDNS3_CONTROLLER_V1	1
	u32				version;
};

int cdns3_handshake(void __iomem *ptr, u32 mask, u32 done, int usec);

#endif /* __LINUX_CDNS3_CORE_H */

// SPDX-License-Identifier: GPL-2.0+
/*
 * K3: Common Architecture initialization
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */

#include <common.h>
#include <spl.h>
#include "common.h"
#include <dm.h>
#include <remoteproc.h>
#include <asm/arch/sys_proto.h>
#include <linux/soc/ti/ti_sci_protocol.h>
#include <fdt_support.h>
#include <fs_loader.h>
#include <fs.h>
#include <environment.h>
#include <elf.h>

DECLARE_GLOBAL_DATA_PTR;

struct ti_sci_handle *get_ti_sci_handle(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_FIRMWARE, "dmsc", &dev);
	if (ret)
		panic("Failed to get SYSFW (%d)\n", ret);

	return (struct ti_sci_handle *)ti_sci_get_handle_from_sysfw(dev);
}

#ifdef CONFIG_K3_EARLY_CONS
int early_console_init(void)
{
	struct udevice *dev;
	int ret;

	gd->baudrate = CONFIG_BAUDRATE;

	ret = uclass_get_device_by_seq(UCLASS_SERIAL, CONFIG_K3_EARLY_CONS_IDX,
				       &dev);
	if (ret) {
		printf("Error getting serial dev for early console! (%d)\n",
		       ret);
		return ret;
	}

	gd->cur_serial_dev = dev;
	gd->flags |= GD_FLG_SERIAL_READY;
	gd->have_console = 1;

	return 0;
}
#endif

#ifdef CONFIG_SYS_K3_SPL_ATF

void init_env(void)
{
#ifdef CONFIG_SPL_ENV_SUPPORT
	char *part;

	env_init();
	env_load();
	switch (spl_boot_device()) {
	case BOOT_DEVICE_MMC2:
		part = env_get("bootpart");
		env_set("storage_interface", "mmc");
		env_set("fw_dev_part", part);
		break;
	case BOOT_DEVICE_SPI:
		env_set("storage_interface", "ubi");
		env_set("fw_ubi_mtdpart", "UBI");
		env_set("fw_ubi_volume", "UBI0");
		break;
	default:
		printf("%s from device %u not supported!\n",
		       __func__, spl_boot_device());
		return;
	}
#endif
}

#ifdef CONFIG_FS_LOADER
int load_firmware(char *name_fw, char *name_loadaddr, u32 *loadaddr)
{
	struct firmware *firmwarep = NULL;
	struct device_platdata *dp;
	struct udevice *fsdev;
	char *name = NULL;
	int size = 0;

	*loadaddr = 0;
#ifdef CONFIG_SPL_ENV_SUPPORT
	switch (spl_boot_device()) {
	case BOOT_DEVICE_MMC2:
		name = env_get(name_fw);
		*loadaddr = env_get_hex(name_loadaddr, *loadaddr);
		break;
	default:
		printf("Loading rproc fw image from device %u not supported!\n",
		       spl_boot_device());
		return 0;
	}
#endif
	if (!*loadaddr)
		return 0;

	if (!uclass_get_device(UCLASS_FS_FIRMWARE_LOADER, 0, &fsdev)) {
		dp = dev_get_platdata(fsdev);
		size = request_firmware_into_buf(dp, name, (void *)*loadaddr,
						 0, 0, &firmwarep);
	}

	release_firmware(firmwarep);
	return size;
}
#else
int load_firmware(char *name_fw, char *name_loadaddr, u32 *loadaddr)
{
	return 0;
}
#endif

__weak void start_non_linux_remote_cores(void)
{
}

void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_noargs_t)(void);
	struct ti_sci_handle *ti_sci = get_ti_sci_handle();
	u32 loadaddr = 0;
	int ret, size;

	/* Release all the exclusive devices held by SPL before starting ATF */
	ti_sci->ops.dev_ops.release_exclusive_devices(ti_sci);

	ret = rproc_init();
	if (ret)
		panic("rproc failed to be initialized (%d)\n", ret);

	init_env();
	start_non_linux_remote_cores();
	size = load_firmware("mcur5f0_0fwname", "mcur5f0_0loadaddr",
			     &loadaddr);

	/*
	 * It is assumed that remoteproc device 1 is the corresponding
	 * Cortex-A core which runs ATF. Make sure DT reflects the same.
	 */
	ret = rproc_load(1, spl_image->entry_point, 0x200);
	if (ret)
		panic("%s: ATF failed to load on rproc (%d)\n", __func__, ret);

	/* Add an extra newline to differentiate the ATF logs from SPL */
	printf("Starting ATF on ARM64 core...\n\n");

	ret = rproc_start(1);
	if (ret)
		panic("%s: ATF failed to start on rproc (%d)\n", __func__, ret);

	if (!(size > 0 && valid_elf_image(loadaddr))) {
		debug("Shutting down...\n");
		release_resources_for_core_shutdown();

		while (1)
			asm volatile("wfe");
	}

	image_entry_noargs_t image_entry =
		(image_entry_noargs_t)load_elf_image_phdr(loadaddr);

	image_entry();
}
#endif

#if defined(CONFIG_OF_LIBFDT)
int fdt_fixup_msmc_ram(void *blob, char *parent_path, char *node_name)
{
	u64 msmc_start = 0, msmc_end = 0, msmc_size, reg[2];
	struct ti_sci_handle *ti_sci = get_ti_sci_handle();
	int ret, node, subnode, len, prev_node;
	u32 range[4], addr, size;
	const fdt32_t *sub_reg;

	ti_sci->ops.core_ops.query_msmc(ti_sci, &msmc_start, &msmc_end);
	msmc_size = msmc_end - msmc_start + 1;
	debug("%s: msmc_start = 0x%llx, msmc_size = 0x%llx\n", __func__,
	      msmc_start, msmc_size);

	/* find or create "msmc_sram node */
	ret = fdt_path_offset(blob, parent_path);
	if (ret < 0)
		return ret;

	node = fdt_find_or_add_subnode(blob, ret, node_name);
	if (node < 0)
		return node;

	ret = fdt_setprop_string(blob, node, "compatible", "mmio-sram");
	if (ret < 0)
		return ret;

	reg[0] = cpu_to_fdt64(msmc_start);
	reg[1] = cpu_to_fdt64(msmc_size);
	ret = fdt_setprop(blob, node, "reg", reg, sizeof(reg));
	if (ret < 0)
		return ret;

	fdt_setprop_cell(blob, node, "#address-cells", 1);
	fdt_setprop_cell(blob, node, "#size-cells", 1);

	range[0] = 0;
	range[1] = cpu_to_fdt32(msmc_start >> 32);
	range[2] = cpu_to_fdt32(msmc_start & 0xffffffff);
	range[3] = cpu_to_fdt32(msmc_size);
	ret = fdt_setprop(blob, node, "ranges", range, sizeof(range));
	if (ret < 0)
		return ret;

	subnode = fdt_first_subnode(blob, node);
	prev_node = 0;

	/* Look for invalid subnodes and delete them */
	while (subnode >= 0) {
		sub_reg = fdt_getprop(blob, subnode, "reg", &len);
		addr = fdt_read_number(sub_reg, 1);
		sub_reg++;
		size = fdt_read_number(sub_reg, 1);
		debug("%s: subnode = %d, addr = 0x%x. size = 0x%x\n", __func__,
		      subnode, addr, size);
		if (addr + size > msmc_size ||
		    !strncmp(fdt_get_name(blob, subnode, &len), "sysfw", 5) ||
		    !strncmp(fdt_get_name(blob, subnode, &len), "l3cache", 7)) {
			fdt_del_node(blob, subnode);
			debug("%s: deleting subnode %d\n", __func__, subnode);
			if (!prev_node)
				subnode = fdt_first_subnode(blob, node);
			else
				subnode = fdt_next_subnode(blob, prev_node);
		} else {
			prev_node = subnode;
			subnode = fdt_next_subnode(blob, prev_node);
		}
	}

	return 0;
}

int fdt_disable_node(void *blob, char *node_path)
{
	int offs;
	int ret;

	offs = fdt_path_offset(blob, node_path);
	if (offs < 0) {
		debug("Node %s not found.\n", node_path);
		return 0;
	}
	ret = fdt_setprop_string(blob, offs, "status", "disabled");
	if (ret < 0) {
		printf("Could not add status property to node %s: %s\n",
		       node_path, fdt_strerror(ret));
		return ret;
	}
	return 0;
}

#endif

#ifndef CONFIG_SYSRESET
void reset_cpu(ulong ignored)
{
}
#endif

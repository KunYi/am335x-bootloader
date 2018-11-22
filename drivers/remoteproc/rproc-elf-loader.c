// SPDX-License-Identifier: GPL-2.0+
/*
 * Remote Processor Framework Elf loader
 *
 * Copyright (C) 2018-2019 Texas Instruments Incorporated - http://www.ti.com/
 *	Keerthy <j-keerthy@ti.com>
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */

#include <common.h>
#include <dm.h>
#include <elf.h>
#include <dm/of_access.h>
#include <fs_loader.h>
#include <remoteproc.h>
#include <errno.h>

void *rproc_da_to_va(struct udevice *dev, ulong da, ulong size)
{
	const struct dm_rproc_ops *ops = rproc_get_ops(dev);

	if (ops && ops->da_to_va)
		return ops->da_to_va(dev, da, size);

	/* If translation op is not provided assume 1:1 translation */
	return (void *)da;
}

/**
 * rproc_elf_sanity_check() - Sanity Check ELF firmware image
 * @addr:	Pointer to the elf image as passed by user.
 * @size:	Size of the elf image as passed by user.
 */
static int rproc_elf32_sanity_check(ulong addr, ulong size)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;
	char class;

	if (!IS_ELF(*ehdr)) {
		printf("## No elf image at address 0x%08lx\n", addr);
		return -EINVAL;
	}

	class = ehdr->e_ident[EI_CLASS];
	if (class != ELFCLASS32) {
		printf("Unsupported ELF class: %d\n", class);
		return -EINVAL;
	}

	/* We assume the firmware has the same endianness as the host */
# ifdef __LITTLE_ENDIAN
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
# else /* BIG ENDIAN */
	if (ehdr->e_ident[EI_DATA] != ELFDATA2MSB) {
# endif
		printf("Unsupported firmware endianness\n");
		return -EINVAL;
	}

	if (size < ehdr->e_shoff + sizeof(Elf32_Shdr)) {
		printf("Image is too small\n");
		return -EINVAL;
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
		printf("Image is corrupted (bad magic)\n");
		return -EINVAL;
	}

	if (ehdr->e_phnum == 0) {
		printf("No loadable segments\n");
		return -EINVAL;
	}

	if (ehdr->e_phoff > size) {
		printf("Firmware size is too small\n");
		return -EINVAL;
	}

	return 0;
}

static int rproc_elf64_sanity_check(ulong addr, ulong size)
{
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)addr;
	char class;

	if (!IS_ELF(*ehdr)) {
		printf("## No elf image at address 0x%08lx\n", addr);
		return -EINVAL;
	}

	class = ehdr->e_ident[EI_CLASS];
	if (class != ELFCLASS64) {
		printf("Unsupported ELF class: %d\n", class);
		return -EINVAL;
	}

	/* We assume the firmware has the same endianness as the host */
# ifdef __LITTLE_ENDIAN
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
# else /* BIG ENDIAN */
	if (ehdr->e_ident[EI_DATA] != ELFDATA2MSB) {
# endif
		printf("Unsupported firmware endianness\n");
		return -EINVAL;
	}

	if (size < ehdr->e_shoff + sizeof(Elf64_Shdr)) {
		printf("Image is too small\n");
		return -EINVAL;
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
		printf("Image is corrupted (bad magic)\n");
		return -EINVAL;
	}

	if (ehdr->e_phnum == 0) {
		printf("No loadable segments\n");
		return -EINVAL;
	}

	if (ehdr->e_phoff > size) {
		printf("Firmware size is too small\n");
		return -EINVAL;
	}

	return 0;
}

static int rproc_elf32_load_segments(struct udevice *dev, ulong addr,
				     ulong size)
{
	u32 da, memsz, filesz, offset;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	int i, ret = 0;
	void *ptr;

	dev_dbg(dev, "%s: addr = 0x%lx size = 0x%lx\n", __func__, addr, size);

	if (rproc_elf32_sanity_check(addr, size))
		return -EINVAL;

	ehdr = (Elf32_Ehdr *)addr;
	phdr = (Elf32_Phdr *)(addr + ehdr->e_phoff);

	/* go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		offset = phdr->p_offset;

		if (phdr->p_type != PT_LOAD)
			continue;

		dev_dbg(dev, "%s:phdr: type %d da 0x%x memsz 0x%x filesz 0x%x\n",
			__func__, phdr->p_type, da, memsz, filesz);

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%x memsz 0x%x\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > size) {
			dev_err(dev, "truncated fw: need 0x%x avail 0x%lx\n",
				offset + filesz, size);
			ret = -EINVAL;
			break;
		}

		/* get the cpu address for this device address */
		ptr = rproc_da_to_va(dev, da, memsz);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			ret = -EINVAL;
			break;
		}

		if (filesz)
			memcpy(ptr, (void *)addr + offset, filesz);
		if (filesz != memsz)
			memset(ptr + filesz, 0x00, memsz - filesz);

		flush_cache((ulong)ptr, filesz);
	}

	return ret;
}

static int rproc_elf64_load_segments(struct udevice *dev, ulong addr,
				     ulong size)
{
	u64 da, memsz, filesz, offset;
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;
	int i, ret = 0;
	void *ptr;

	dev_dbg(dev, "%s: addr = 0x%lx size = 0x%lx\n", __func__, addr, size);

	if (rproc_elf64_sanity_check(addr, size))
		return -EINVAL;

	ehdr = (Elf64_Ehdr *)addr;
	phdr = (Elf64_Phdr *)(addr + (ulong)ehdr->e_phoff);

	/* go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		offset = phdr->p_offset;

		if (phdr->p_type != PT_LOAD)
			continue;

		dev_dbg(dev, "%s:phdr: type %d da 0x%llx memsz 0x%llx filesz 0x%llx\n",
			__func__, phdr->p_type, da, memsz, filesz);

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%llx memsz 0x%llx\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > size) {
			dev_err(dev, "truncated fw: need 0x%llx avail 0x%lx\n",
				offset + filesz, size);
			ret = -EINVAL;
			break;
		}

		/* get the cpu address for this device address */
		ptr = rproc_da_to_va(dev, da, memsz);
		if (!ptr) {
			dev_err(dev, "bad da 0x%llx mem 0x%llx\n", da, memsz);
			ret = -EINVAL;
			break;
		}

		if (filesz)
			memcpy(ptr, (void *)addr + offset, filesz);
		if (filesz != memsz)
			memset(ptr + filesz, 0x00, memsz - filesz);

		flush_cache((ulong)ptr, filesz);
	}

	return ret;
}

int rproc_elf_load_segments(struct udevice *dev, ulong addr, ulong size)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;

	if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
		return rproc_elf64_load_segments(dev, addr, size);
	else
		return rproc_elf32_load_segments(dev, addr, size);
}

static ulong rproc_elf32_get_boot_addr(ulong addr)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;

	return ehdr->e_entry;
}

static ulong rproc_elf64_get_boot_addr(ulong addr)
{
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)addr;

	return ehdr->e_entry;
}

ulong rproc_elf_get_boot_addr(struct udevice *dev, ulong addr)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;

	if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
		return rproc_elf64_get_boot_addr(addr);
	else
		return rproc_elf32_get_boot_addr(addr);
}

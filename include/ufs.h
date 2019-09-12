/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _UFS_H
#define _UFS_H
/**
 * ufs_probe() - initialize all devices in the UFS uclass
 *
 * @return 0 if Ok, -ve on error
 */
int ufs_probe(void);

/*
 * ufs_scsi_bind() - Create a new scsi device as a child of the UFS device and
 *		     bind it to the ufs_scsi driver
 * @ufs_dev: UFS device
 * @scsi_devp: Pointer to scsi device
 *
 * @return 0 if Ok, -ve on error
 */
int ufs_scsi_bind(struct udevice *ufs_dev, struct udevice **scsi_devp);
#endif

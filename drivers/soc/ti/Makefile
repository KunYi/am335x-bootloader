# SPDX-License-Identifier: GPL-2.0+

obj-$(CONFIG_TI_K3_NAVSS_RINGACC)	+= k3-navss-ringacc.o

ifeq ($(CONFIG_SPL_BUILD)$(CONFIG_TPL_BUILD),)
obj-$(CONFIG_TI_KEYSTONE_SERDES)	+= keystone_serdes.o
obj-$(CONFIG_TI_PRUSS)	+= pruss.o
endif

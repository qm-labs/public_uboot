# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) SolidRun

obj-$(CONFIG_TARGET_CLEARFOG) += tlv_data.o
obj-$(CONFIG_VID)		+= vid.o
ifdef CONFIG_SECURE_BOOT
obj-$(CONFIG_CMD_ESBC_VALIDATE) += fsl_validate.o cmd_esbc_validate.o
endif
obj-$(CONFIG_CHAIN_OF_TRUST) += fsl_chain_of_trust.o

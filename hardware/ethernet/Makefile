TOPDIR ?= ../..
include $(TOPDIR)/.config

ifneq ($(TEENSY_SUPPORT),y)
$(ETHERNET_SUPPORT)_ECMD_SRC += hardware/ethernet/ethernet_ecmd.c
endif

$(ETHERNET_SUPPORT)_SRC += \
	hardware/ethernet/ethernet_config.c	\
	hardware/ethernet/ethernet_common.c

$(ENC28J60_SUPPORT)_SRC += \
	hardware/ethernet/enc28j60.c		\
	hardware/ethernet/enc28j60_process.c	\
	hardware/ethernet/enc28j60_transmit.c

$(W5100_SUPPORT)_SRC += \
	hardware/ethernet/w5100.c

##############################################################################
# generic fluff
include $(TOPDIR)/scripts/rules.mk

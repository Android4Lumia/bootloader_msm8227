LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH    := arm
ARM_CPU := cortex-a8
CPU     := generic

DEFINES += ARM_CPU_CORE_KRAIT

MMC_SLOT         := 1

DEFINES += PERIPH_BLK_BLSP=1
DEFINES += WITH_CPU_EARLY_INIT=0 WITH_CPU_WARM_BOOT=0 \
	   MMC_SLOT=$(MMC_SLOT)

INCLUDES += -I$(LOCAL_DIR)/include

DEVS += fbcon
MODULES += dev/fbcon

OBJS += \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/acpuclock.o \
	$(LOCAL_DIR)/msm8996-clock.o \
	$(LOCAL_DIR)/gpio.o

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include platform/msm_shared/rules.mk

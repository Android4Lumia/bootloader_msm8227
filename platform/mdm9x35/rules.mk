LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH    := arm
ARM_CPU := cortex-a8
CPU     := generic

DEFINES += ARM_CPU_CORE_A7

INCLUDES += -I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/gpio.o \
	$(LOCAL_DIR)/acpuclock.o \
	$(LOCAL_DIR)/mdm9x35-clock.o

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include platform/msm_shared/rules.mk


LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/msm_shared

PLATFORM := mdm9640

MEMBASE                             ?= 0x87C00000
MEMSIZE                             ?= 0x00100000 # 1MB
BASE_ADDR                           := 0x80000000
SCRATCH_ADDR                        := 0x80000000
SCRATCH_REGION1                     := 0x81300000
SCRATCH_REGION1_SIZE                := 0x06900000 # 105MB
SCRATCH_REGION2                     := 0x88000000
SCRATCH_REGION2_SIZE                := 0x08000000 # 128MB
KERNEL_REGION                       := 0x80000000
KERNEL_REGION_SIZE                  := 0x01200000 # 18MB

DEFINES += DISPLAY_SPLASH_SCREEN=0
DEFINES += NO_KEYPAD_DRIVER=1
DEFINES += PERIPH_BLK_BLSP=1

DEVS += fbcon
MODULES += \
	dev/keys \
	lib/ptable \
	dev/pmic/pm8x41 \
	lib/libfdt \
	dev/fbcon

DEFINES += \
	MEMBASE=$(MEMBASE) \
	BASE_ADDR=$(BASE_ADDR) \
	SCRATCH_ADDR=$(SCRATCH_ADDR) \
	SCRATCH_REGION1=$(SCRATCH_REGION1) \
	SCRATCH_REGION2=$(SCRATCH_REGION2) \
	MEMSIZE=$(MEMSIZE) \
	SCRATCH_REGION1_SIZE=$(SCRATCH_REGION1_SIZE) \
	SCRATCH_REGION2_SIZE=$(SCRATCH_REGION2_SIZE) \
	KERNEL_REGION=$(KERNEL_REGION) \
	KERNEL_REGION_SIZE=$(KERNEL_REGION_SIZE) \


OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/meminfo.o \
	$(LOCAL_DIR)/target_display.o \
	$(LOCAL_DIR)/qpic_panel_drv.o \
	$(LOCAL_DIR)/keypad.o

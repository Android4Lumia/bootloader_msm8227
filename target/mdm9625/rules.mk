LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/msm_shared

PLATFORM := mdm9x25

MEMBASE                             ?= 0x07E00000
MEMSIZE                             ?= 0x00100000 # 1MB
SCRATCH_ADDR                        := 0x00300000
SCRATCH_REGION1                     := 0x00300000
SCRATCH_REGION1_SIZE                := 0x01000000 #16 MB
SCRATCH_REGION2                     := 0x06800000
SCRATCH_REGION2_SIZE                := 0x01600000 #22 MB

DEFINES += NO_KEYPAD_DRIVER=1
DEFINES += PERIPH_BLK_BLSP=1

MODULES += \
	dev/keys \
	lib/ptable \
    dev/pmic/pm8x41 \
	lib/libfdt

DEFINES += \
	MEMBASE=$(MEMBASE) \
	SCRATCH_ADDR=$(SCRATCH_ADDR) \
	SCRATCH_REGION1=$(SCRATCH_REGION1) \
	SCRATCH_REGION2=$(SCRATCH_REGION2) \
	MEMSIZE=$(MEMSIZE) \
	SCRATCH_REGION1_SIZE=$(SCRATCH_REGION1_SIZE) \
	SCRATCH_REGION2_SIZE=$(SCRATCH_REGION2_SIZE) \

OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/meminfo.o \
	$(LOCAL_DIR)/keypad.o

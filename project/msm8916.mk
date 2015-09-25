# top level project rules for the msm8916 project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := msm8916

MODULES += app/aboot

ifeq ($(TARGET_BUILD_VARIANT),user)
DEBUG := 0
else
DEBUG := 1
endif

EMMC_BOOT := 1

#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_LOG_BUF=1
DEFINES += WITH_DEBUG_UART=1
#DEFINES += WITH_DEBUG_FBCON=1
DEFINES += ENABLE_FBCON_LOGGING=1
DEFINES += DEVICE_TREE=1
#DEFINES += MMC_BOOT_BAM=1
DEFINES += CRYPTO_BAM=1
DEFINES += SPMI_CORE_V2=1
DEFINES += BAM_V170=1
DEFINES += ABOOT_IGNORE_BOOT_HEADER_ADDRS=1

#Enable the feature of long press power on
DEFINES += LONG_PRESS_POWER_ON=1

#Disable thumb mode
ENABLE_THUMB := false
ENABLE_PWM_SUPPORT := true

ENABLE_SDHCI_SUPPORT := 1

ifeq ($(ENABLE_SDHCI_SUPPORT),1)
DEFINES += MMC_SDHCI_SUPPORT=1
endif

#enable power on vibrator feature
ENABLE_PON_VIB_SUPPORT := true

ifeq ($(EMMC_BOOT),1)
DEFINES += _EMMC_BOOT=1
endif

ifeq ($(ENABLE_PON_VIB_SUPPORT),true)
DEFINES += PON_VIB_SUPPORT=1
endif

#enable user force reset feature
DEFINES += USER_FORCE_RESET_SUPPORT=1

#SCM call before entering DLOAD mode
DEFINES += PLATFORM_USE_SCM_DLOAD=1

#Enable the external reboot functions
ENABLE_REBOOT_MODULE := 1
#Use PON register for reboot reason
DEFINES += USE_PON_REBOOT_REG=1

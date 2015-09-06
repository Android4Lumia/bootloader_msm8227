LOCAL_DIR := $(GET_LOCAL_DIR)

DEFINES += ASSERT_ON_TAMPER=1

OBJS += \
	$(LOCAL_DIR)/ufs_rpmb.o \
	$(LOCAL_DIR)/qseecom_lk_test.o

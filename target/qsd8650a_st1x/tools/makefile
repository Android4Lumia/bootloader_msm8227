#Makefile to generate appsboot.mbn

APPSBOOTHEADER_DIR  := $(BUILDDIR)

SRC_DIR  := target/$(TARGET)/tools
COMPILER := gcc

ifeq ($(BUILD_NANDWRITE), 1)
  APPSBOOTHDR_FILES :=
else
  APPSBOOTHDR_FILES := appsboot.mbn
endif

APPSBOOTHEADER: $(APPSBOOTHDR_FILES)


appsboot.mbn: appsboothd.mbn $(OUTBIN)
	cat $(APPSBOOTHEADER_DIR)/appsboothd.mbn $(OUTBIN) > $(APPSBOOTHEADER_DIR)/appsboot.mbn
	rm -rf $(APPSBOOTHEADER_DIR)/appsboothd.mbn 

appsboothd.mbn: mkheader $(OUTBIN)
	$(SRC_DIR)/mkheader $(OUTBIN) $(APPSBOOTHEADER_DIR)/appsboothd.mbn

mkheader: $(SRC_DIR)/mkheader.c
	${COMPILER} $(SRC_DIR)/mkheader.c -o $(SRC_DIR)/mkheader


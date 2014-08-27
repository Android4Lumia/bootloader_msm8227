#Makefile to generate appsboot.mbn

APPSBOOTOUT_DIR  := $(BUILDDIR)

ABOOTMBN := emmc_appsboot.mbn
APPSBOOTHEADER: $(ABOOTMBN)

$(ABOOTMBN): $(OUTELF_STRIP)
	$(hide) cp -f $(OUTELF_STRIP) $(APPSBOOTOUT_DIR)/$(ABOOTMBN)

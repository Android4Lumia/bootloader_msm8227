#Makefile to generate appsboot.mbn

APPSBOOTOUT_DIR  := $(BUILDDIR)

APPSBOOTHEADER: emmc_appsboot.mbn

emmc_appsboot.mbn: $(OUTELF_STRIP)
	$(hide) cp -f $(OUTELF_STRIP) $(APPSBOOTOUT_DIR)/emmc_appsboot.mbn

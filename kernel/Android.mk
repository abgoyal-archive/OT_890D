ifeq ($(MTK_PROJECT), gw616)
  KERNEL_CONFIG_FILE := config-mt6516-phone
else
  ifeq ($(MTK_PROJECT), ds269)
    KERNEL_CONFIG_FILE := config-mt6516-gemini
  else
    ifeq ($(MTK_PROJECT), oppo)
      KERNEL_CONFIG_FILE := config-mt6516-oppo
    else
      ifeq ($(MTK_PROJECT), mt6516_evb)
        KERNEL_CONFIG_FILE := config-mt6516-evb
      else
        KERNEL_CONFIG_FILE := config-mt6516-$(MTK_PROJECT)
      endif
    endif
  endif
endif

ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
  KERNEL_CONFIG_FILE := $(strip $(KERNEL_CONFIG_FILE))-user
endif

$(info using $(KERNEL_CONFIG_FILE) .... )
ifeq ($(TARGET_KMODULES),true)
ALL_PREBUILT += $(TARGET_OUT)/lib/modules/2.6.29/modules.order
$(BUILT_SYSTEMIMAGE): kernel_modules
$(TARGET_OUT)/lib/modules/2.6.29/modules.order: kernel_modules
kernel_modules:
	@echo "building linux kernel modules..."
ifneq (,$(KERNEL_CONFIG_FILE))
	@cat kernel/$(KERNEL_CONFIG_FILE) > kernel/.config
endif
	make MTK_PROJECT=$(MTK_PROJECT) -C  kernel modules
	INSTALL_MOD_STRIP=1 MTK_PROJECT=$(MTK_PROJECT) INSTALL_MOD_PATH=../$(TARGET_OUT) INSTALL_MOD_DIR=../$(TARGET_OUT) make -C kernel android_modules_install
endif

KPD_AUTOTEST := true

ifeq ($(KPD_AUTOTEST), true)
include kernel/trace32/kpd_test/Android.mk
endif

################################################################################
#
# ax-sdk
#
################################################################################

AX_SDK_VERSION = main
AX_SDK_SITE = $(call github,galaxywarriordad,ax-sdk,$(AX_SDK_VERSION))
AX_SDK_LICENSE = GPL-3.0
AX_SDK_INSTALL_STAGING = YES

define AX_SDK_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/ax $(STAGING_DIR)/usr/src/ax
	cp -a $(@D)/include/ax/* $(STAGING_DIR)/usr/include/ax/ 2>/dev/null || true
	cp -a $(@D)/src/input.cpp $(STAGING_DIR)/usr/src/ax/input.cpp 2>/dev/null || true
endef

$(eval $(generic-package))

################################################################################
#
# flac-ax
#
################################################################################

FLAC_AX_VERSION = main
FLAC_AX_SITE = $(call github,galaxywarriordad,FLAC_AX,$(FLAC_AX_VERSION))
FLAC_AX_LICENSE = GPL-3.0
FLAC_AX_DEPENDENCIES = ax-sdk alsa-lib flac libopenmpt

define FLAC_AX_BUILD_CMDS
	$(TARGET_CXX) $(TARGET_CXXFLAGS) -std=c++20 -O2 -Wall -Wextra \
		-I$(STAGING_DIR)/usr/include \
		$(@D)/src/main.cpp -o $(@D)/flac_ax \
		-lasound -lFLAC -lopenmpt -lpthread
	$(TARGET_CXX) $(TARGET_CXXFLAGS) -std=c++20 -O2 -Wall -Wextra \
		-I$(STAGING_DIR)/usr/include -I$(@D)/src/tui \
		$(@D)/src/tui/appliance_tui_main.cpp $(STAGING_DIR)/usr/src/ax/input.cpp \
		-o $(@D)/ax_appliance_tui
endef

define FLAC_AX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/flac_ax $(TARGET_DIR)/usr/bin/flac_ax
	$(INSTALL) -D -m 0755 $(@D)/ax_appliance_tui $(TARGET_DIR)/usr/bin/ax_appliance_tui
endef

$(eval $(generic-package))

################################################################################
#
# flac-ax
#
################################################################################

FLAC_AX_VERSION = main
FLAC_AX_SITE = $(call github,galaxywarriordad,FLAC_AX,$(FLAC_AX_VERSION))
FLAC_AX_LICENSE = GPL-3.0
FLAC_AX_DEPENDENCIES = host-cmake host-ninja ax-sdk alsa-lib flac libopenmpt

FLAC_AX_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DFLAC_AX_USE_AX_SDK=ON \
	-DFLAC_AX_BUILD_PLAYER=ON \
	-DFLAC_AX_BUILD_STUDIO=OFF \
	-DFLAC_AX_BUILD_UPSCALER=OFF \
	-DFLAC_AX_BUILD_ENCODE_2GO=OFF \
	-DFLAC_AX_BUILD_LV2_HOST=OFF \
	-DFLAC_AX_BUILD_CLAP_HOST=OFF \
	-DFLAC_AX_ENABLE_NATIVE_OPT=OFF \
	-DFLAC_AX_ENABLE_LTO=ON

define FLAC_AX_BUILD_CMDS
	cmake -S $(@D) -B $(@D)/build -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(HOST_DIR)/share/buildroot/toolchainfile.cmake \
		$(FLAC_AX_CONF_OPTS)
	ninja -C $(@D)/build -j 4 flac_ax
endef

define FLAC_AX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/build/flac_ax $(TARGET_DIR)/usr/bin/flac_ax
endef

$(eval $(generic-package))

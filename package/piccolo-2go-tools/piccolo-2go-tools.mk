################################################################################
#
# piccolo-2go-tools
#
################################################################################

PICCOLO_2GO_TOOLS_VERSION = 1.0.0
PICCOLO_2GO_TOOLS_SITE = $(BR2_EXTERNAL_PICCOLO_2GO_PATH)/package/piccolo-2go-tools/src
PICCOLO_2GO_TOOLS_SITE_METHOD = local
PICCOLO_2GO_TOOLS_LICENSE = GPL-3.0

define PICCOLO_2GO_TOOLS_BUILD_CMDS
	rm -f $(@D)/dtb-select $(@D)/ax-powerd $(@D)/ax-boot-recovery
	$(TARGET_CC) $(TARGET_CFLAGS) -O2 -Wall -Wextra -Werror \
		$(@D)/dtb-select.c -o $(@D)/dtb-select
	$(TARGET_CC) $(TARGET_CFLAGS) -O2 -Wall -Wextra -Werror \
		$(@D)/ax-powerd.c -o $(@D)/ax-powerd
	$(TARGET_CC) $(TARGET_CFLAGS) -O2 -Wall -Wextra -Werror \
		$(@D)/ax-boot-recovery.c -o $(@D)/ax-boot-recovery
endef

define PICCOLO_2GO_TOOLS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dtb-select $(TARGET_DIR)/usr/bin/dtb-select
	$(INSTALL) -D -m 0755 $(@D)/ax-powerd $(TARGET_DIR)/usr/sbin/ax-powerd
	$(INSTALL) -D -m 0755 $(@D)/ax-boot-recovery $(TARGET_DIR)/usr/sbin/ax-boot-recovery
endef

$(eval $(generic-package))

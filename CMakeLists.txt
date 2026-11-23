cmake_minimum_required(VERSION 3.20)
project(piccolo_2go NONE)

set(ARKOS4CLONE_DIR "" CACHE PATH "Path to local lcdyk0517/arkos4clone checkout or extracted subset")
set(PICCOLO_CONSOLE "g350" CACHE STRING "Console directory under boot/dArkOS/consoles")
set(PICCOLO_KERNEL "5.10" CACHE STRING "Kernel DTB/overlay variant (5.10 or 4.4)")
set_property(CACHE PICCOLO_KERNEL PROPERTY STRINGS 5.10 4.4)

set(PICCOLO_PROFILE "default" CACHE STRING "Profile: safe, default, or oc_boost")
set_property(CACHE PICCOLO_PROFILE PROPERTY STRINGS safe default oc_boost)

set(PICCOLO_GPU_MHZ "" CACHE STRING "Optional GPU clock override in MHz (200..650; empty uses profile)")
set(PICCOLO_DDR_MHZ "" CACHE STRING "Optional DDR clock override in MHz (194..1040; empty uses profile)")
set(PICCOLO_CPU_MAX_MHZ "" CACHE STRING "Optional CPU max clock override in MHz (408..1608; empty uses profile)")
set(PICCOLO_CPU_BOOT_MHZ "" CACHE STRING "Optional CPU boot clock override in MHz (defaults to CPU max)")
set(PICCOLO_APPLY_OC_DTBO OFF CACHE BOOL "Stage and apply rk3326-oc-voltage.dtbo in staged boot.ini")
set(PICCOLO_STAGE_OVERLAY OFF CACHE BOOL "Stage repurposed udev/dma-heap rules into stage_overlay")
set(PICCOLO_STAGE_DIR "${CMAKE_BINARY_DIR}/stage_boot" CACHE PATH "Output BOOT staging directory")
set(PICCOLO_OVERLAY_DIR "${CMAKE_BINARY_DIR}/stage_overlay" CACHE PATH "Output rootfs overlay staging directory")

if(NOT ARKOS4CLONE_DIR)
  message(FATAL_ERROR "Set -DARKOS4CLONE_DIR=/path/to/arkos4clone")
endif()

set(CONSOLES_ROOT "${ARKOS4CLONE_DIR}/boot/dArkOS/consoles")
set(CONSOLE_SRC_DIR "${CONSOLES_ROOT}/${PICCOLO_CONSOLE}")
if(NOT EXISTS "${CONSOLE_SRC_DIR}")
  message(FATAL_ERROR "Console directory not found: ${CONSOLE_SRC_DIR}")
endif()

if(PICCOLO_KERNEL STREQUAL "5.10")
  set(DTB_SOURCE_DIR "${CONSOLE_SRC_DIR}/5.10")
  set(OC_DTBO_FILE "${CONSOLES_ROOT}/dtbo/5.10/rk3326-oc-voltage.dtbo")
else()
  set(DTB_SOURCE_DIR "${CONSOLE_SRC_DIR}")
  set(OC_DTBO_FILE "${CONSOLES_ROOT}/dtbo/rk3326-oc-voltage.dtbo")
endif()

file(GLOB DTB_FILES "${DTB_SOURCE_DIR}/*.dtb")
if(NOT DTB_FILES)
  message(FATAL_ERROR "No .dtb files found in ${DTB_SOURCE_DIR}")
endif()

# Base profile definitions (RK3326 + RK817 12.5 mV / 12500 uV VDD_LOGIC steps)
if(PICCOLO_PROFILE STREQUAL "safe")
  set(DEF_CPU_MAX "1296")
  set(DEF_CPU_BOOT "1296")
  set(DEF_GPU_MAX "520")
  set(DEF_DDR_MAX "666")
  set(PROFILE_VDD_LOGIC_UV "1100000")
elseif(PICCOLO_PROFILE STREQUAL "default")
  set(DEF_CPU_MAX "1344")
  set(DEF_CPU_BOOT "1344")
  set(DEF_GPU_MAX "600")
  set(DEF_DDR_MAX "720")
  set(PROFILE_VDD_LOGIC_UV "1137500")
elseif(PICCOLO_PROFILE STREQUAL "oc_boost")
  set(DEF_CPU_MAX "1392")
  set(DEF_CPU_BOOT "1344")
  set(DEF_GPU_MAX "620")
  set(DEF_DDR_MAX "786")
  set(PROFILE_VDD_LOGIC_UV "1162500")
else()
  message(FATAL_ERROR "Unsupported PICCOLO_PROFILE: ${PICCOLO_PROFILE}")
endif()

if(NOT PICCOLO_CPU_MAX_MHZ STREQUAL "")
  set(EFF_CPU_MAX "${PICCOLO_CPU_MAX_MHZ}")
else()
  set(EFF_CPU_MAX "${DEF_CPU_MAX}")
endif()

if(NOT PICCOLO_CPU_BOOT_MHZ STREQUAL "")
  set(EFF_CPU_BOOT "${PICCOLO_CPU_BOOT_MHZ}")
elseif(NOT PICCOLO_CPU_MAX_MHZ STREQUAL "")
  set(EFF_CPU_BOOT "${PICCOLO_CPU_MAX_MHZ}")
else()
  set(EFF_CPU_BOOT "${DEF_CPU_BOOT}")
endif()

if(NOT PICCOLO_GPU_MHZ STREQUAL "")
  set(EFF_GPU_MAX "${PICCOLO_GPU_MHZ}")
else()
  set(EFF_GPU_MAX "${DEF_GPU_MAX}")
endif()

if(NOT PICCOLO_DDR_MHZ STREQUAL "")
  set(EFF_DDR_MAX "${PICCOLO_DDR_MHZ}")
else()
  set(EFF_DDR_MAX "${DEF_DDR_MAX}")
endif()

# Map voltage step guidance across 520 / 550 / 580 / 600 / 620 MHz
if(EFF_GPU_MAX LESS_EQUAL 520)
  set(EFF_VDD_LOGIC_UV "1100000")
elseif(EFF_GPU_MAX LESS_EQUAL 550)
  set(EFF_VDD_LOGIC_UV "1112500")
elseif(EFF_GPU_MAX LESS_EQUAL 580)
  set(EFF_VDD_LOGIC_UV "1125000")
elseif(EFF_GPU_MAX LESS_EQUAL 600)
  set(EFF_VDD_LOGIC_UV "1137500")
else()
  set(EFF_VDD_LOGIC_UV "1162500")
endif()

# Logo resolution mapping per console (matches arkos4clone consoles/config.go)
set(LOGO_SUBDIR "480P")
if(PICCOLO_CONSOLE STREQUAL "a10miniv4")
  set(LOGO_SUBDIR "540P")
elseif(PICCOLO_CONSOLE MATCHES "^(r36ultra|r36tmax|r36splus|mini40|r36max type[123]|rf40h|dc40v|xf40v)$")
  set(LOGO_SUBDIR "720P")
elseif(PICCOLO_CONSOLE MATCHES "^(r36ultrax|h7|go2|r46h|r40xxpromax|r40xx|r36hpromax|r45h|r36max2|xf45v|dc45v|rf45v|rf45h|t16max)$")
  set(LOGO_SUBDIR "768P")
elseif(PICCOLO_CONSOLE MATCHES "^(rgb10max1|rgb10max2|r50s)$")
  set(LOGO_SUBDIR "854x480P")
elseif(PICCOLO_CONSOLE STREQUAL "r50h")
  set(LOGO_SUBDIR "720x1280P")
elseif(PICCOLO_CONSOLE STREQUAL "rf55h")
  set(LOGO_SUBDIR "720x1280P-90")
elseif(PICCOLO_CONSOLE MATCHES "^(rp1|dr28s)$")
  set(LOGO_SUBDIR "480P-270")
elseif(PICCOLO_CONSOLE STREQUAL "xf28")
  set(LOGO_SUBDIR "480P-1")
elseif(PICCOLO_CONSOLE MATCHES "^(u8 panel1|u8 panel2|r40s)$")
  set(LOGO_SUBDIR "480P5-3")
endif()

set(LOGO_SRC_DIR "${CONSOLES_ROOT}/logo/${LOGO_SUBDIR}")
find_program(DTC_EXECUTABLE dtc)

set(BOOT_INI_SRC "${CONSOLE_SRC_DIR}/boot.ini")
set(BOOT_INI_OUT "${PICCOLO_STAGE_DIR}/boot.ini")
set(PROFILE_ENV_OUT "${PICCOLO_STAGE_DIR}/piccolo-profile.env")

add_custom_target(stage_piccolo_2go ALL
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${PICCOLO_STAGE_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${DTB_FILES} "${PICCOLO_STAGE_DIR}/"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BOOT_INI_SRC}" "${BOOT_INI_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_CONSOLE=${PICCOLO_CONSOLE}" > "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_KERNEL=${PICCOLO_KERNEL}" >> "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_PROFILE=${PICCOLO_PROFILE}" >> "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_CPU_MAX_MHZ=${EFF_CPU_MAX}" >> "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_CPU_BOOT_MHZ=${EFF_CPU_BOOT}" >> "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_GPU_MAX_MHZ=${EFF_GPU_MAX}" >> "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_DDR_MAX_MHZ=${EFF_DDR_MAX}" >> "${PROFILE_ENV_OUT}"
  COMMAND "${CMAKE_COMMAND}" -E echo "PICCOLO_VDD_LOGIC_UV=${EFF_VDD_LOGIC_UV}" >> "${PROFILE_ENV_OUT}"
  COMMENT "Staging Piccolo-2go (${PICCOLO_CONSOLE}, kernel=${PICCOLO_KERNEL}, profile=${PICCOLO_PROFILE})"
)

if(EXISTS "${LOGO_SRC_DIR}")
  add_custom_command(TARGET stage_piccolo_2go POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${LOGO_SRC_DIR}" "${PICCOLO_STAGE_DIR}/logo"
    COMMENT "Staging boot logo assets from logo/${LOGO_SUBDIR}"
  )
endif()

if(PICCOLO_APPLY_OC_DTBO AND EXISTS "${OC_DTBO_FILE}")
  set(DTBO_DST_REL "dtbo/rk3326-oc-voltage.dtbo")
  add_custom_command(TARGET stage_piccolo_2go POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PICCOLO_STAGE_DIR}/dtbo"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${OC_DTBO_FILE}" "${PICCOLO_STAGE_DIR}/${DTBO_DST_REL}"
    COMMENT "Staging ${OC_DTBO_FILE}"
  )
endif()

if(PICCOLO_STAGE_OVERLAY)
  set(UDEV_SRC "${ARKOS4CLONE_DIR}/rootfs/dArkOS/etc/udev/rules.d")
  add_custom_command(TARGET stage_piccolo_2go POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PICCOLO_OVERLAY_DIR}/etc/udev/rules.d"
    COMMENT "Preparing staged udev rules directory"
  )
  if(EXISTS "${UDEV_SRC}/99-odroidgo3.rules")
    add_custom_command(TARGET stage_piccolo_2go POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${UDEV_SRC}/99-odroidgo3.rules"
        "${PICCOLO_OVERLAY_DIR}/etc/udev/rules.d/99-odroidgo3.rules"
    )
  endif()
  if(EXISTS "${UDEV_SRC}/70-mali-dma-heap.rules")
    add_custom_command(TARGET stage_piccolo_2go POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${UDEV_SRC}/70-mali-dma-heap.rules"
        "${PICCOLO_OVERLAY_DIR}/etc/udev/rules.d/70-mali-dma-heap.rules"
    )
  endif()
endif()

if(DTC_EXECUTABLE)
  foreach(dtb_file IN LISTS DTB_FILES)
    get_filename_component(dtb_name "${dtb_file}" NAME)
    add_custom_command(TARGET stage_piccolo_2go POST_BUILD
      COMMAND "${DTC_EXECUTABLE}" -I dtb -O dts -o "${CMAKE_BINARY_DIR}/${dtb_name}.check.dts" "${PICCOLO_STAGE_DIR}/${dtb_name}"
      COMMENT "Decompiling ${dtb_name} with dtc for inspection"
    )
  endforeach()
endif()

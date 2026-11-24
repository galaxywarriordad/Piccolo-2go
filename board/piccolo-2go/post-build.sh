#!/bin/sh
set -eu

BOARD_DIR="$(cd "$(dirname "$0")" && pwd)"
BOOT_STAGE_DIR="${BINARIES_DIR}"

mkdir -p "${BOOT_STAGE_DIR}" "${TARGET_DIR}/boot/dtbs"

for script in \
    "${TARGET_DIR}/etc/init.d/S01piccolo-2go-isolate" \
    "${TARGET_DIR}/etc/init.d/S02piccolo-2go-storage" \
    "${TARGET_DIR}/etc/init.d/S03piccolo-2go-powerd" \
    "${TARGET_DIR}/usr/sbin/ax-audio-route" \
    "${TARGET_DIR}/usr/bin/ax-movie"
do
    [ -e "$script" ] && chmod 0755 "$script"
done

for conf in "${BOARD_DIR}/boot.ini" "${BOARD_DIR}/ax-2go.conf" "${BOARD_DIR}/logo.bmp"; do
    if [ -e "$conf" ]; then
        cp -f "$conf" "${BOOT_STAGE_DIR}/"
    fi
done

if [ -d "${BOARD_DIR}/dtbs" ]; then
    cp -f "${BOARD_DIR}/dtbs/"*.dtb "${BOOT_STAGE_DIR}/" 2>/dev/null || true
    cp -f "${BOARD_DIR}/dtbs/"*.dtb "${TARGET_DIR}/boot/dtbs/" 2>/dev/null || true
fi

if [ -e "${BOOT_STAGE_DIR}/rk3326-g350-linux.dtb" ]; then
    cp -f "${BOOT_STAGE_DIR}/rk3326-g350-linux.dtb" "${TARGET_DIR}/boot/rk3326-g350-linux.dtb"
fi

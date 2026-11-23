#!/bin/sh
set -eu

TARGET_DIR="$1"
BOARD_DIR="$(dirname "$0")"

echo "[Piccolo-2go] Staging boot.ini, ax-2go.conf, U-Boot logo.bmp, DTBs & permissions..."
install -D -m 0644 "${BOARD_DIR}/boot.ini" "${BINARIES_DIR}/boot.ini"
install -D -m 0644 "${BOARD_DIR}/ax-2go.conf" "${BINARIES_DIR}/ax-2go.conf"
if [ -f "${BOARD_DIR}/logo.bmp" ]; then
    install -D -m 0644 "${BOARD_DIR}/logo.bmp" "${BINARIES_DIR}/logo.bmp"
fi

# Stage bundled verified DTBs (G350, R36H, R46H, R36S Plus & top 4 R36S panel revisions)
mkdir -p "${TARGET_DIR}/boot/dtbs"
if [ -d "${BOARD_DIR}/dtbs" ]; then
    cp -f "${BOARD_DIR}/dtbs"/*.dtb "${BINARIES_DIR}/" 2>/dev/null || true
    cp -f "${BOARD_DIR}/dtbs"/*.dtb "${TARGET_DIR}/boot/dtbs/" 2>/dev/null || true
fi

if [ ! -f "${BINARIES_DIR}/rk3326-g350-linux.dtb" ]; then
    if [ -f "${BINARIES_DIR}/rk3326-odroid-go3.dtb" ]; then
        cp -f "${BINARIES_DIR}/rk3326-odroid-go3.dtb" "${BINARIES_DIR}/rk3326-g350-linux.dtb"
    elif [ -f "${BINARIES_DIR}/rk3326-odroid-go2.dtb" ]; then
        cp -f "${BINARIES_DIR}/rk3326-odroid-go2.dtb" "${BINARIES_DIR}/rk3326-g350-linux.dtb"
    fi
fi
cp -f "${BINARIES_DIR}"/*.dtb "${TARGET_DIR}/boot/dtbs/" 2>/dev/null || true

chmod 0755 "${TARGET_DIR}/etc/init.d/S01piccolo-2go-isolate" \
           "${TARGET_DIR}/etc/init.d/S02piccolo-2go-storage" \
           "${TARGET_DIR}/etc/init.d/S03piccolo-2go-powerd" \
           "${TARGET_DIR}/usr/sbin/ax-audio-route" 2>/dev/null || true

# Purge software Vulkan (lvp/lavapipe) ICD JSON so LLVM isn't dragged into memory
if [ -d "${TARGET_DIR}/usr/share/vulkan/icd.d" ]; then
    find "${TARGET_DIR}/usr/share/vulkan/icd.d" -name 'lvp_icd*.json' -delete
fi

mkdir -p "${TARGET_DIR}/boot" "${TARGET_DIR}/userdata" "${TARGET_DIR}/mnt/tf2"
echo "PICCOLO_2GO_ARCH=aarch64-rk3326" > "${TARGET_DIR}/etc/piccolo-2go-release"

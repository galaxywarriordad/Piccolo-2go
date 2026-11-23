#!/bin/sh
set -eu

TARGET_DIR="$1"
BOARD_DIR="$(dirname "$0")"

mkdir -p "${BINARIES_DIR}" "${TARGET_DIR}/boot/dtbs" "${TARGET_DIR}/boot" "${TARGET_DIR}/userdata" "${TARGET_DIR}/mnt/tf2"

echo "[Piccolo-2go] Staging boot.ini, ax-2go.conf, U-Boot logo.bmp, DTBs & permissions..."
install -D -m 0644 "${BOARD_DIR}/boot.ini" "${BINARIES_DIR}/boot.ini"
install -D -m 0644 "${BOARD_DIR}/ax-2go.conf" "${BINARIES_DIR}/ax-2go.conf"
if [ -f "${BOARD_DIR}/logo.bmp" ]; then
    install -D -m 0644 "${BOARD_DIR}/logo.bmp" "${BINARIES_DIR}/logo.bmp"
fi

# Stage bundled DTBs (G350, R36H, R46H, R36S Plus & R36S panel revisions)
if [ -d "${BOARD_DIR}/dtbs" ]; then
    cp -f "${BOARD_DIR}/dtbs"/*.dtb "${BINARIES_DIR}/" 2>/dev/null || true
fi

if [ ! -f "${BINARIES_DIR}/rk3326-g350-linux.dtb" ]; then
    if [ -f "${BINARIES_DIR}/rk3326-odroid-go3.dtb" ]; then
        cp -f "${BINARIES_DIR}/rk3326-odroid-go3.dtb" "${BINARIES_DIR}/rk3326-g350-linux.dtb"
    elif [ -f "${BINARIES_DIR}/rk3326-odroid-go2.dtb" ]; then
        cp -f "${BINARIES_DIR}/rk3326-odroid-go2.dtb" "${BINARIES_DIR}/rk3326-g350-linux.dtb"
    fi
fi

cp -f "${BINARIES_DIR}"/*.dtb "${TARGET_DIR}/boot/dtbs/" 2>/dev/null || true

if [ -f "${TARGET_DIR}/etc/udev/rules.d/99-ax-2go-otg.rules" ]; then
    chmod 0644 "${TARGET_DIR}/etc/udev/rules.d/99-ax-2go-otg.rules"
fi

chmod 0755 "${TARGET_DIR}/etc/init.d/S01piccolo-2go-isolate" \
           "${TARGET_DIR}/etc/init.d/S02piccolo-2go-storage" \
           "${TARGET_DIR}/etc/init.d/S03piccolo-2go-powerd" \
           "${TARGET_DIR}/usr/sbin/ax-audio-route" 2>/dev/null || true

# Purge software Vulkan (lvp/lavapipe) ICD JSON so LLVM isn't dragged into memory
if [ -d "${TARGET_DIR}/usr/share/vulkan/icd.d" ]; then
    find "${TARGET_DIR}/usr/share/vulkan/icd.d" -name 'lvp_icd*.json' -delete
fi

cat <<'EOF' > "${TARGET_DIR}/etc/piccolo-2go-release"
PICCOLO_2GO_ARCH=aarch64-rk3326
PICCOLO_2GO_VERSION=1.1
PICCOLO_2GO_DEFAULT_DTB=/boot/rk3326-g350-linux.dtb
EOF

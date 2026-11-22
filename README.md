# Piccolo-2go
Piccolo-2go — Deterministic ARM64 Buildroot handheld OS for BATLEXP G350 &amp; R36S (RK3326). Features 1:2:1 Cortex-A35 core isolation, 384 MB zstd ZRAM, KMS/DRM + Panfrost GLES, dual SanDisk Extreme PRO (64 GB + 128 GB) storage, Amiberry/Hatari/BlastEm [AX], and bit-perfect FLAC_AX audio tuned for early HiBy FC3 1.3 Vrms USB-C OTG DACs.

# Piccolo-2go

[![Buildroot CI](https://img.shields.io/badge/CI-Buildroot%20aarch64-brightgreen?style=flat-square)](https://github.com/galaxywarriordad/Piccolo-2go/actions)
[![Architecture](https://img.shields.io/badge/Arch-AArch64%20Cortex--A35%20%7C%20RK3326-blue?style=flat-square)](#1-121-rk3326-core--irq-topology)
[![Graphics](https://img.shields.io/badge/Display-KMS%2FDRM%20%26%20Panfrost%20GLES%203.0-orange?style=flat-square)](#1-121-rk3326-core--irq-topology)
[![Audio](https://img.shields.io/badge/Audio-Bit--Perfect%20S32__LE%20%7C%20HiBy%20FC3-purple?style=flat-square)](#3-bit-perfect-audio-subsystem)
[![Image Footprint](https://img.shields.io/badge/Footprint-%3C%20180%20MB%20SquashFS-informational?style=flat-square)](#2-dual-sandisk-storage--memory-discipline)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-red?style=flat-square)](LICENSE)

**Piccolo-2go** is an ultra-lean (`< 180 MB`), deterministic Buildroot `aarch64` bare-metal operating system engineered for the **BATLEXP G350** and **R36S** handheld consoles. Part of **The Silicon Preservation Society** ecosystem, it strips away desktop daemons, background bloat, and audio resampling layers in favor of strict CPU core isolation, IRQ pinning, zero-copy KMS/DRM & Panfrost rendering, and bit-perfect `S32_LE` audio output.

---

## 🎯 Target Hardware

* **Supported Consoles:** BATLEXP G350 & R36S (`dtb-select` runtime Device Tree resolution)
* **SoC:** Rockchip RK3326 (Quad-core ARM Cortex-A35 @ 1.5 GHz, in-order pipeline)
* **GPU:** ARM Mali-G31 MP2 (Open-source Mesa Panfrost GLES 3.0 + KMS/DRM atomic scanout)
* **Audio DAC:** External HiBy FC3 USB-C DAC (`1.3 Vrms`, ESS ES9281AC Pro, USB VID `0x32BB`) with automatic fallback to internal Rockchip RK817 codec

---

## ⚙️ Architecture & Topology

### 1. `1:2:1` RK3326 Core & IRQ Topology

The four in-order ARM Cortex-A35 cores are partitioned at boot via kernel command-line isolation (`isolcpus=0,1,2 nohz_full=0,1,2`) and runtime affinity enforcement (`S01piccolo-2go-isolate`) so real-time audio/input and frame scanout never contend with storage DMA or housekeeping tasks:

| Core | Affinity Mask | Scheduler & Governor | Pinned Subsystems & IRQs |
| :--- | :--- | :--- | :--- |
| **Core 0** | `0x01` | `SCHED_FIFO` (prio `80`) / `performance` | Real-time audio & input (`HiBy FC3` USB-C audio, `DWC2` OTG controller, `I2S0`, `GPIO` / `SARADC` controls) |
| **Core 1** | `0x02` | `SCHED_OTHER` / `performance` | Zero-copy `KMS/DRM` & `Panfrost` GLES 3.0 video scanout on Mali-G31 MP2 |
| **Core 2** | `0x04` | `SCHED_OTHER` / `performance` | Deterministic emulation JIT & DSP execution (`Amiberry`, `Hatari`, `BlastEm`, `PCSX-ReARMed`) |
| **Core 3** | `0x08` | `SCHED_OTHER` (`nice +5`) / `ondemand` | Housekeeping core: SD/eMMC storage DMA, `ZRAM` compression, RAM preloader, and system daemons |

### 2. Dual SanDisk Storage & Memory Discipline

Piccolo-2go enforces strict separation between immutable OS binaries and portable user libraries across dual microSD slots:

* **TF1 — OS Slot (64 GB SanDisk):**
  * `p1` (`FAT32`): Boot partition housing U-Boot artifacts, kernel `Image`, and multi-board Device Tree Blobs (`dtb-select`).
  * `p2` (`SquashFS zstd`): Immutable, read-only root filesystem (`< 180 MB` footprint) immune to sudden power-loss corruption.
  * `p3` (`ext4`): Auto-expanded persistent overlay/configuration partition initialized at first boot by `S02piccolo-2go-storage`.
* **TF2 — Media Slot (128 GB SanDisk):**
  * `p1` (`exFAT` / `ext4`): Dedicated portable library partition for ROMs, bit-perfect FLAC/DSD audio libraries, and system BIOS files.
* **Memory & Pruned Kernel Fragment:**
  * Configures a `384 MB` `zstd`-compressed `ZRAM` swap device (`vm.swappiness=100`) to eliminate physical flash wear.
  * Uses a stripped RK3326 kernel fragment (`board/piccolo-2go/linux-rk3326.fragment`) that drops unused SoC blocks (`VPU`, `ISP`, `RGA`, legacy codecs) and compiles USB networking drivers strictly as loadable modules.

### 3. Bit-Perfect Audio Subsystem

* **Direct Hardware Routing:** Bypasses PulseAudio and PipeWire completely, routing straight to ALSA `hw:` endpoints in native `S32_LE` format.
* **Dynamic Gain Staging & Hotplug (`99-ax-2go-otg.rules`):** Detects USB-C OTG attachment of the early **HiBy FC3 (`1.3 Vrms`)** DAC (`VID 0x32BB`) and applies safe gain staging profiles tailored for:
  1. Portable console speakers
  2. Full-size wired headphones
  3. Sensitive In-Ear Monitors (IEMs)
* **Automatic Internal Fallback:** Seamlessly reverts audio routing and hardware volume curves to the internal **RK817** codec when the USB-C DAC is disconnected.

---

## 📁 Repository Structure

```text
Piccolo-2go/
├── Config.in                          # Buildroot external Kconfig menu entries
├── external.desc                      # Buildroot external tree descriptor (PICCOLO_2GO)
├── external.mk                        # External package & board Makefile hooks
├── configs/
│   └── piccolo_2go_rk3326_defconfig   # Master < 180 MB aarch64 defconfig
├── board/
│   └── piccolo-2go/
│       ├── linux-rk3326.fragment      # Pruned RK3326 kernel configuration fragment
│       ├── post-build.sh              # Rootfs staging, permissions, and version stamping
│       ├── post-image.sh              # Genimage TF1 FAT32 + SquashFS + ext4 image assembly
│       ├── genimage.cfg               # Partition table layout for TF1 boot/rootfs/storage
│       └── rootfs-overlay/
│           ├── etc/
│           │   ├── init.d/
│           │   │   ├── S01piccolo-2go-isolate   # Core 0-2 isolcpus, IRQ affinity & governor setup
│           │   │   └── S02piccolo-2go-storage   # TF1 p3 auto-expansion & TF2 media mount
│           │   └── udev/rules.d/
│           │       └── 99-ax-2go-otg.rules      # HiBy FC3 OTG detection, gain staging & RK817 fallback
└── package/
    └── piccolo-2go-tools/
        ├── Config.in                  # System utilities package selection
        ├── piccolo-2go-tools.mk       # Build instructions for native aarch64 C/C++ tools
        └── src/
            ├── dtb-select.c           # Hardware revision & panel DTB selector (G350 / R36S)
            └── ax-powerd.c            # Low-overhead battery, thermal & power button daemon
---

## 🚀 Building from Source

# Clone Buildroot and the Piccolo-2go external tree
git clone [https://github.com/buildroot/buildroot.git](https://github.com/buildroot/buildroot.git)
git clone [https://github.com/galaxywarriordad/Piccolo-2go.git](https://github.com/galaxywarriordad/Piccolo-2go.git)

# Configure out-of-tree build with BR2_EXTERNAL
cd buildroot
make O=../build-piccolo-2go BR2_EXTERNAL=../Piccolo-2go piccolo_2go_rk3326_defconfig

# Compile the < 180 MB SquashFS image
cd ../build-piccolo-2go
make -j$(nproc)

The resulting flashable TF1 image (sdcard.img) is written to build-piccolo-2go/images/sdcard.img.

🌐 Silicon Preservation Society Ecosystem
Core Repositories (8)
Shared Silicon SDK (GitHub): galaxywarriordad/ax-sdk — Shared dependency-free C++20 deterministic hardware runtime (ax::sdk / v1_36_2), dynamic Intel SMT / AMD CCX topology resolver, platform shims, and master architecture specs.

Linux USB Console & A/V Appliance (GitHub): galaxywarriordad/Piccolo-AX — Standalone < 300 MB toram EROFS Batocera / Buildroot Linux 6.12-rt x86_64 appliance (batocera-ax-external) with direct ALSA SCHED_FIFO and iHD VA-API dma_buf.

Linux Handheld Appliance (GitHub): galaxywarriordad/Piccolo-2go — Ultra-lean (< 180 MB) deterministic Buildroot aarch64 (RK3326 Cortex-A35 1:2:1 topology, KMS/DRM & Panfrost GLES 3.0, bit-perfect S32_LE HiBy FC3 / RK817 audio) retro handheld OS for BATLEXP G350 and R36S.

Optional Desktop Tier (GitHub): galaxywarriordad/piccolo-ax-desktop — On-demand C/C++ LXQt/X11 desktop workbench (< 42 MB budget).

Commodore Amiga Engine (GitHub): galaxywarriordad/Amiberry-AX — Deterministic Commodore Amiga 500 / 1200 / CD32 engine linked against ax-sdk.

Bare-Metal Audio & A/V Engine (GitHub): galaxywarriordad/FLAC_AX — RAM-resident bit-perfect audio player, ax_av_studio MIDI/DSP synth workstation, and ax_video_upscaler stream pipeline.

Retro Multi-System Engine (GitHub): galaxywarriordad/Generation-X-AX — Deterministic multi-console [AX] integration across Flycast and RMG.

Wii U Engine (GitHub): galaxywarriordad/Cemu-Latte-AX — Deterministic Nintendo Wii U (Latte GPU) AX engine powered by ax::sdk.

Sister & Downstream Forks
Main Switch Engine Fork (Forgejo): galaxywarriordad/Eden-Espresso-Wave-AX — Deterministic Nintendo Switch fork tuned for ax::sdk core topology, Vulkan telemetry, and low-latency audio, kept aligned with upstream for frictionless PR cherry-picks.

Upstream Ecosystem Hub (Forgejo): [suspicious link removed] — Upstream Eden Switch emulator reference repository.

📜 License
Released under the GNU General Public License v3.0 (GPL-3.0). See LICENSE for full terms.

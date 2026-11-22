# Piccolo-2go

[![Buildroot CI](https://img.shields.io/badge/CI-Buildroot%20aarch64-brightgreen?style=flat-square)](https://github.com/galaxywarriordad/Piccolo-2go/actions)
[![Architecture](https://img.shields.io/badge/Arch-AArch64%20Cortex--A35%20%7C%20RK3326-blue?style=flat-square)](#1-121-rk3326-core--irq-topology)
[![Graphics](https://img.shields.io/badge/Display-KMS%2FDRM%20%26%20Panfrost%20GLES%203.0-orange?style=flat-square)](#1-121-rk3326-core--irq-topology)
[![Audio](https://img.shields.io/badge/Audio-Bit--Perfect%20S32__LE%20%7C%20HiBy%20FC3-purple?style=flat-square)](#3-bit-perfect-audio-subsystem)
[![Image Footprint](https://img.shields.io/badge/Footprint-%3C%20180%20MB%20SquashFS-informational?style=flat-square)](#2-dual-sandisk-storage--memory-discipline)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-red?style=flat-square)](LICENSE)

**Piccolo-2go** is a deterministic `aarch64` Buildroot operating system (`< 180 MB` SquashFS rootfs) engineered for the **BATLEXP G350** and **R36S** handheld consoles on **RK3326** silicon, **optimised with AX tech**. Part of **The Silicon Preservation Society** ecosystem, it combines `1:2:1` Cortex-A35 core isolation, `384 MB` `zstd` ZRAM, zero-copy Mali-G31 MP2 KMS/DRM + Panfrost GLES 3.0 rendering, dual SanDisk Extreme PRO (`64 GB` + `128 GB`) storage discipline, `Amiberry` / `Hatari` / `BlastEm` emulation, and bit-perfect `FLAC_AX` `S32_LE` audio tuned for early **HiBy FC3 (`1.3 Vrms`)** USB-C OTG DACs.

---

## 🎯 Target Hardware

* **Supported Consoles:** BATLEXP G350 & R36S (`dtb-select` runtime Device Tree resolution)
* **SoC:** Rockchip RK3326 (Quad-core ARM Cortex-A35 @ 1.5 GHz, in-order pipeline)
* **GPU:** ARM Mali-G31 MP2 (Open-source Mesa Panfrost GLES 3.0 + KMS/DRM atomic scanout)
* **Storage Media:** Dual SanDisk Extreme PRO cards (`64 GB` OS slot + `128 GB` portable media slot)
* **Audio DAC:** External HiBy FC3 USB-C DAC (`1.3 Vrms`, ESS ES9281AC Pro, USB VID `0x32BB`) with automatic fallback to internal Rockchip RK817 codec

---

## ⚙️ Architecture & Topology (Optimised with AX Tech)

### 1. `1:2:1` RK3326 Silicon & IRQ Topology

The Rockchip **RK3326** pairs four dual-issue **in-order ARM Cortex-A35 (`1.5 GHz`)** cores with a shared `256 KB` L2 cache and a `32-bit` `1 GB LPDDR3-1600` (`~6.4 GB/s`) memory bus. Because in-order cores stall immediately on cache thrashing and unpinned interrupts, `Piccolo-2go` partitions the SoC at boot via tickless kernel isolation (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate`:

| Core | Affinity Mask | Scheduler & Governor | Pinned Subsystems & Hardware IRQs |
| :--- | :--- | :--- | :--- |
| **Core 0** | `0x01` | `SCHED_FIFO` (prio `80`) / `performance` | **Real-Time Audio Clock & Input Plane:** Direct ALSA `S32_LE` writer (`192`-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`), `I2S0` (`RK817`), and `GPIO` / `SARADC` control IRQs |
| **Core 1** | `0x02` | `SCHED_OTHER` / `performance` | **GPU & Zero-Copy Display Plane:** Direct `KMS/DRM` page-flip submission and `Mali-G31 MP2` (`Panfrost` GLES 3.0) draw calls to the `640×480 @ 60 Hz` MIPI DSI panel |
| **Core 2** | `0x04` | `SCHED_OTHER` / `performance` | **Deterministic Emulation JIT & DSP Plane:** 68020/AGA JIT, Z80/68k/MIPS cycle-accurate execution (`Amiberry`, `Hatari`, `BlastEm`, `PCSX-ReARMed`) with hot `32 KB` L1 caches |
| **Core 3** | `0x08` | `SCHED_OTHER` (`nice +5`) / `schedutil` | **Housekeeping & Storage DMA Plane:** `dwmmc` (`TF1`/`TF2` SanDisk SD) IRQs, `384 MB` `zstd` ZRAM, `FLAC_AX` `64 MiB` `SingleFile` RAM preloader, `ax_appliance_tui`, and `ax-powerd` |

### 2. GPU & Zero-Copy Display Pipeline (`Mali-G31 MP2` → `VOP` → `MIPI DSI`)

* **Direct KMS/DRM + Panfrost GLES 3.0 (`SDL_VIDEODRIVER=kmsdrm`):** Eliminates X11 and Wayland compositors completely, saving `~35–50 MB` of LPDDR3 RAM and avoiding extra `640×480×4` framebuffer composition copies.
* **Exact `2×` Integer Scaling (`640×480` 4:3 OCA IPS):** Classic `320×240` and `320×200` systems (`Amiga`, `Atari ST`, `Mega Drive`, `PS1`) map onto the `640×480` panel with razor-sharp integer pixels, zero aspect distortion, and deterministic VBlank pacing (`60.00 Hz` / `50.00 Hz`).
* **Pruned Multimedia Silicon (`linux-rk3326.fragment`):** Disables unused `VPU`, `ISP`, and `RGA` 2D blocks and shrinks CMA reservation to `64 MB`, freeing `~110 MB` of physical LPDDR3 for emulation and audio buffers.

### 3. Bit-Perfect Audio Architecture (`HiBy FC3 1.3 Vrms` & `RK817` Fallback)

* **Zero-Resampling `S32_LE` & Native DoP Pipeline:** Bypasses PulseAudio and PipeWire completely (`snd_pcm_hw_params_set_rate_resample(0)`), locking the DAC hardware clock directly to native sample rates (`44.1–384 kHz` PCM and `DSD64` `176.4 kHz` / `DSD128` `352.8 kHz` DoP) with zero USB autosuspend (`power/control="on"`).
* **Dynamic Gain Staging & Hotplug (`99-ax-2go-otg.rules`):** Detects USB-C OTG attachment of the early **HiBy FC3 (`1.3 Vrms / 70 mW`, ESS ES9281AC Pro, VID `0x32BB`)** and applies hardware attenuator profiles (`fc3_1v3`) tailored for:
  1. **Portable Powered Speakers / Line-Out:** `--volume 0.95` (`~1.24 Vrms` clean headroom)
  2. **Wired Headphones (e.g., Grado SR325x, Koss Porta Pro):** `--volume 0.65`
  3. **Sensitive In-Ear Monitors (`16–32 Ω`):** `--volume 0.35`
* **Automatic Internal Fallback & Rail Isolation:** Hot-plugging the HiBy FC3 powers down the internal `RK817` Class-D speaker amplifier to eliminate PMIC switching noise; disconnecting the USB-C DAC seamlessly reverts ALSA routing to the internal **RK817** codec (`3.5 mm` jack + mono speaker).

### 4. Dual SanDisk Extreme PRO Storage & `384 MB` ZRAM Discipline

* **TF1 — OS Slot (`64 GB` SanDisk Extreme PRO `A2`/`V30`):**
  * `p1` (`FAT32`, `256 MB`): Boot partition housing U-Boot artifacts, kernel `Image`, `boot.ini`, and multi-board Device Tree Blobs (`dtb-select`).
  * `p2` (`SquashFS zstd`, `< 180 MB`): Immutable, read-only root filesystem immune to sudden power-loss corruption.
  * `p3` (`ext4`, `~59 GB`): Auto-expanded persistent save/configuration partition initialized at first boot by `S02piccolo-2go-storage`.
* **TF2 — Media Slot (`128 GB` SanDisk Extreme PRO `A2`/`V30`):**
  * `p1` (`exFAT` / `ext4`, `~119 GB`): Dedicated portable library partition automatically bound to `/userdata/{roms,music,bios}` on boot for ROMs, bit-perfect `FLAC_AX` / `.MOD` libraries, and system BIOS files.
* **`384 MB` `zstd` ZRAM (`vm.swappiness=100`, `vm.page-cluster=0`):** Compresses cold anonymous pages in RAM at `~2.8:1` (`~1.05 GB` effective swap) with single-page (`4 KB`) latency and zero microSD flash wear.


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
```     
---

## Silicon Preservation Society Ecosystem

### Core Repositories (8)

1. **Shared Silicon SDK (GitHub):** [`galaxywarriordad/ax-sdk`](https://github.com/galaxywarriordad/ax-sdk.git) — Shared dependency-free C++20 deterministic hardware runtime (`ax::sdk` / `v1_36_2`), dynamic Intel SMT / AMD CCX topology resolver, platform shims, and master architecture specs.
2. **Linux USB Console & A/V Appliance (GitHub):** [`galaxywarriordad/Piccolo-AX`](https://github.com/galaxywarriordad/Piccolo-AX.git) — Standalone `< 300 MB` `toram` EROFS Batocera / Buildroot `Linux 6.12-rt` `x86_64` appliance (`batocera-ax-external`) with direct ALSA `SCHED_FIFO` and `iHD` VA-API `dma_buf`.
3. **Linux Handheld Appliance (GitHub):** [`galaxywarriordad/Piccolo-2go`](https://github.com/galaxywarriordad/Piccolo-2go.git) — Deterministic Buildroot `aarch64` (`RK3326` `1:2:1`) retro handheld OS for **BATLEXP G350** and **R36S**.
4. **Optional Desktop Tier (GitHub):** [`galaxywarriordad/piccolo-ax-desktop`](https://github.com/galaxywarriordad/piccolo-ax-desktop.git) — On-demand C/C++ LXQt/X11 desktop workbench (`< 42 MB` budget).
5. **Commodore Amiga Engine (GitHub):** [`galaxywarriordad/Amiberry-AX`](https://github.com/galaxywarriordad/Amiberry-AX.git) — Deterministic Commodore Amiga 500 / 1200 / CD32 engine linked against `ax-sdk`.
6. **Bare-Metal Audio & A/V Engine (GitHub):** [`galaxywarriordad/FLAC_AX`](https://github.com/galaxywarriordad/FLAC_AX.git) — RAM-resident bit-perfect audio player, `ax_av_studio` MIDI/DSP synth workstation, and `ax_video_upscaler` stream pipeline.
7. **Retro Multi-System Engine (GitHub):** [`galaxywarriordad/Generation-X-AX`](https://github.com/galaxywarriordad/Generation-X-AX.git) — Deterministic multi-console `[AX]` integration across `Flycast` and `RMG`.
8. **Wii U Engine (GitHub):** [`galaxywarriordad/Cemu-Latte-AX`](https://github.com/galaxywarriordad/Cemu-Latte-AX.git) — Deterministic Nintendo Wii U (Latte GPU) AX engine powered by `ax::sdk`.

### Sister & Downstream Forks

- **Main Switch Engine Fork (Forgejo):** [`galaxywarriordad/Eden-Espresso-Wave-AX`](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) — Deterministic Nintendo Switch fork tuned for `ax::sdk` core topology, Vulkan telemetry, and low-latency audio, kept aligned with upstream for frictionless PR cherry-picks.
- **Upstream Ecosystem Hub (Forgejo):** [`eden-emu/eden`](https://git.eden-emu.dev/eden-emu/eden/) — Upstream Eden Switch emulator reference repository.

---

## 🤝 Contributing, License & Acknowledgements

* **Contributing:** See [CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md) for Buildroot hygiene and hardware-ownership policies.
* **License & Acknowledgements:** Licensed under **GPL-3.0** ([LICENSE](LICENSE)). Branched from [Eden Espresso Wave `[AX]`](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX) and built on [Batocera Linux](https://github.com/batocera-linux/batocera.linux) and Buildroot. **Contains no proprietary console firmware, decryption keys, or games.**

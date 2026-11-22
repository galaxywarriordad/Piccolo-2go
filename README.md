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

## ⚙️ Fixed-Hardware Topology & Silicon Specification (Optimised with AX Tech)

`Piccolo-2go` treats the **BATLEXP G350** and **R36S** as a closed, deterministic fixed-hardware appliance rather than a general-purpose Linux PC—mapping every physical bus, interrupt line, and execution plane onto the **Rockchip RK3326** silicon:

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                  FIXED-HARDWARE APPLIANCE TOPOLOGY — BATLEXP G350 / R36S               │
│                             Rockchip RK3326 (1.5 GHz) + 1 GB LPDDR3                    │
│                                                                                        │
│  [ Core 0 (0x01) ] ──► SCHED_FIFO 80 ALSA S32_LE + DWC2 OTG / I2S0 / GPIO+SARADC IRQs  │
│  [ Core 1 (0x02) ] ──► Mali-G31 MP2 (Panfrost GLES 3.0) ──► VOP ──► 640×480 DSI LCD    │
│  [ Core 2 (0x04) ] ──► Deterministic 68k / Z80 / MIPS JIT & DSP Worker (L1 Cache Hot)  │
│  [ Core 3 (0x08) ] ──► SDMMC0/1 DMA IRQs + 384 MB zstd ZRAM + 64 MiB FLAC Preload      │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│        AUDIO & USB-C OTG SUBSYSTEM        │        DUAL SANDISK EXTREME PRO STORAGE    │
│  • DWC2 USB-C OTG (Core 0 IRQ)            │  • TF1 (64 GB A2/V30 — OS & Save Slot)     │
│    └─► Early HiBy FC3 (ES9281AC Pro)      │     ├── p1: BOOT (FAT32, 256 MB, DTBs)     │
│        1.3 Vrms / 70 mW | S32_LE & DoP    │     ├── p2: ROOTFS (SquashFS zstd, <180MB) │
│  • I2S0 Fallback ──► RK817 PMIC Codec     │     └── p3: STORAGE (ext4, ~59 GB auto)    │
│    (3.5 mm Stereo Jack + 1W Mono Speaker) │  • TF2 (128 GB A2/V30 — Media Library)     │
│  • Controllers: GPIO Matrix + 10-bit ADC  │     └── p1: PICCOLO_MEDIA (exFAT/ext4)     │
│    (D-Pad, ABXY, L1/L2, R1/R2, Dual Stick)│         /roms, /music (FLAC/MOD), /bios    │
└───────────────────────────────────────────┴────────────────────────────────────────────┘
```

### 1. CPU & `1:2:1` Core/IRQ Partition (`4× ARM Cortex-A35 @ 1.5 GHz` + `1 GB LPDDR3`)

The Rockchip **RK3326** pairs four dual-issue **in-order ARM Cortex-A35** cores (`32 KB` L1-I / `32 KB` L1-D per core, `256 KB` shared L2) with a `32-bit` `1 GB LPDDR3-1600` (`~6.4 GB/s`) memory bus. Tickless kernel boot flags (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate` lock each core to a single hardware domain:

| Core | Affinity Mask | Scheduler & Governor | Fixed Hardware Responsibility |
| :--- | :--- | :--- | :--- |
| **Core 0** | `0x01` | `SCHED_FIFO` (prio `80`) / `performance` | **Real-Time Audio Clock & Controller IRQ Plane:** Direct ALSA `S32_LE` writer (`192`-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`), `I2S0` (`RK817`), and `GPIO` / `SARADC` control IRQs |
| **Core 1** | `0x02` | `SCHED_OTHER` / `performance` | **GPU Submission & Display Scanout Plane:** Drives **Mali-G31 MP2** (`Panfrost` OpenGL ES 3.0) draw calls and atomic `KMS/DRM` page flips to the `640×480` panel |
| **Core 2** | `0x04` | `SCHED_OTHER` / `performance` | **Deterministic Emulation JIT & DSP Plane:** Runs 68020/AGA JIT, Z80/68k/MIPS emulation (`Amiberry`, `Hatari`, `BlastEm`, `PCSX-ReARMed`) with zero cache eviction |
| **Core 3** | `0x08` | `SCHED_OTHER` (`nice +5`) / `schedutil` | **Housekeeping & SD Storage DMA Plane:** `dwmmc` (`TF1`/`TF2` SanDisk SD) IRQs, `384 MB` `zstd` ZRAM (`vm.swappiness=100`, `vm.page-cluster=0`), `64 MiB` `FLAC_AX` RAM preloader, `ax_appliance_tui`, and `ax-powerd` |

### 2. GPU & LCD Display Pipeline (`Mali-G31 MP2` → `VOP` → `3.5" 640×480 MIPI DSI IPS`)

* **Hardware-Accelerated GPU Rendering (`ARM Mali-G31 MP2`):** Uses the open-source Mesa **Panfrost** OpenGL ES 3.0 driver (`BR2_PACKAGE_MESA3D_GALLIUM_DRIVER_PANFROST=y`) to execute hardware texture uploads, sharp-bilinear / `2×` integer scaling shaders, and frame composition directly on the Bifrost GPU—never falling back to CPU software rendering.
* **Zero-Compositor Atomic Scanout (`KMS/DRM`):** Renders directly via `SDL_VIDEODRIVER=kmsdrm` into the Rockchip **Video Output Processor (VOP)** over a **4-lane MIPI DSI** bus (`640×480 @ 60.00 Hz`, `4:3` OCA full-laminated IPS display), bypassing X11/Wayland completely (`~35–50 MB` LPDDR3 saved, zero extra buffer copy).
* **Multi-Panel Device Tree Resolution (`dtb-select`):** Resolves MIPI DSI panel initialization sequences across **BATLEXP G350** and **R36S** (`Panel 0`–`Panel 4`) hardware revisions at runtime.

### 3. Physical Controls & Haptic Feedback (`GPIO Matrix` + `SARADC`)

* **Zero-Latency Input Polling (`Core 0` IRQ):** Dedicated GPIO lines handle the D-pad, quiet face buttons (`A/B/X/Y`), sloped rear shoulder triggers (`L1/L2`, `R1/R2`), `Start`, `Select`, `FN`, and `KEY_POWER` (intercepted by `ax-powerd` for `1.5s`-hold safe shutdown).
* **Dual Switch-Style Analog Thumbsticks:** Sampled via the RK3326's 4-channel `10-bit` **SARADC** controller with hardware-calibrated deadzones, alongside PWM control for the internal vibration motor.

### 4. Bit-Perfect Audio DAC Subsystem (`Early HiBy FC3 1.3 Vrms` + `RK817` Fallback)

* **Primary USB-C OTG DAC — Early HiBy FC3 (`ESS ES9281AC Pro`, `1.3 Vrms / 70 mW @ 32 Ω`, VID `0x32BB`):**
  * Hot-plugged via the `DWC2` USB-C OTG port (`99-ax-2go-otg.rules`), locking `power/control="on"` (zero USB autosuspend clicks) and bypassing PulseAudio/PipeWire/software resampling (`snd_pcm_hw_params_set_rate_resample(0)`).
  * Streams native `32-bit` integer **`S32_LE` (`44.1–384 kHz`)** and hardware **`DoP` (`DSD64` `176.4 kHz` / `DSD128` `352.8 kHz`)** with hardware attenuator presets (`fc3_1v3`): `--volume 0.95` (portable speakers/line-out), `0.65` (wired headphones), and `0.35` (sensitive `16–32 Ω` IEMs).
* **Internal Codec Fallback — Rockchip `RK817` (`I2S0`):**
  * Automatically powers down its Class-D speaker amp when the HiBy FC3 is attached (eliminating PMIC rail noise), and seamlessly resumes driving the `3.5 mm` stereo headphone jack and bottom-firing mono speaker when the USB-C DAC is unplugged.

### 5. Dual SanDisk Extreme PRO Storage Architecture (`64 GB TF1` + `128 GB TF2`)

* **TF1 — OS & Save Slot (`64 GB` SanDisk Extreme PRO `A2` / `V30`):**
  * `p1` (`FAT32`, `256 MB`): Boot partition housing U-Boot artifacts, `LZ4`-compressed pruned `linux-rk3326.fragment` kernel `Image` (`< 180 ms` decompression, `~110 MB` LPDDR3 reclaimed), `boot.ini`, and `/boot/dtbs/`.
  * `p2` (`SquashFS zstd`, `128 KB` blocks, `< 160 MB`): Immutable, read-only root filesystem—immune to corruption on sudden power loss.
  * `p3` (`ext4`, `~59 GB`): Auto-expanded on first boot (`S02piccolo-2go-storage`) for persistent save states, shader caches, and local fallback `/userdata`.
* **TF2 — Portable Media Slot (`128 GB` SanDisk Extreme PRO `A2` / `V30`):**
  * `p1` (`exFAT` / `ext4`, `~119 GB`): Dedicated media card automatically bound to `/userdata/{roms,music,bios}` on boot—streaming compressed `.chd` (CD32/PS1), `.lha`/`.zip`, lossless `.flac` / `DoP` albums, and `.mod`/`.xm`/`.s3m`/`.it` trackers with `~50 MB/s` effective throughput.
* **Memory & `512 MB` `LZ4` ZRAM Discipline (`1 GB LPDDR3`):**
  * Configures a **`512 MB` `lz4`-compressed ZRAM swap device** (`~650 MB/s` in-order Cortex-A35 decompression, `vm.swappiness=120`, `vm.page-cluster=0`, `vm.watermark_boost_factor=0`), bounded `/tmp` (`32 MB`) and `/run` (`16 MB`) `tmpfs`, and a `64 MiB` `FLAC_AX` `SingleFile` preload cap—delivering `~1.18 GB` effective working memory with zero microSD flash wear.

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

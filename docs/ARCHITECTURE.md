## Piccolo-2go v1.1 Audio & Movie Subsystem

* **Primary Audio:** HiBy FC3 (£29.61, ES9281 PRO / ES9281AC Pro, 1.3 Vrms) \+ 3.5mm Moondrop Chu  
* **Backup USB-C DACs:** JCALLY JM6 Pro (£12.99, CX31993 \+ MAX97220) and HiBy Digital D1 (£11.11, CS46L41)  
* **Internal Fallback:** RK817 audio codec  
* **Video Playback:** 640x480 mpv / /userdata/movies support

## Piccolo-2go — System Architecture & Fixed-Hardware Topology (v1.1)

* **Platform:** BATLEXP G350 & R36S Handheld Consoles  
* **SoC & PMIC:** Rockchip RK3326 (*4x ARM Cortex-A35*, *ARM Mali-G31 MP2*, *1 GB LPDDR3*) \+ Rockchip *RK817*  
* **Firmware:** *Piccolo-2go v1.1* (*aarch64* Buildroot *BR2\_EXTERNAL*, *128 KB*\-block *zstd* SquashFS *\< 140 MB*, Default DTB: */boot/rk3326-g350-linux.dtb*)  
* **Ecosystem:** *The Silicon Preservation Society* (Optimised with AX Tech)

---

## 1\. Physical Chip Topology & Interconnects

*Piccolo-2go* treats the **BATLEXP G350** and **R36S** as a closed, deterministic fixed-hardware appliance rather than a general-purpose Linux PC. Every physical bus, interrupt line, voltage rail, and execution domain is statically mapped across the **Rockchip RK3326** system-on-chip and its companion **RK817** Power Management & Audio IC:

```
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│                        ROCKCHIP RK3326 SYSTEM-ON-CHIP (28nm HKMG)                          │
│                                                                                            │
│  ┌───────────────────┐ ┌───────────────────┐ ┌───────────────────┐ ┌───────────────────┐   │
│  │  Cortex-A35 #0    │ │  Cortex-A35 #1    │ │  Cortex-A35 #2    │ │  Cortex-A35 #3    │   │
│  │  Mask: 0x01 (RT)  │ │  Mask: 0x02 (GPU) │ │  Mask: 0x04 (EMU) │ │  Mask: 0x08 (I/O) │   │
│  │  32KB I / 32KB D  │ │  32KB I / 32KB D  │ │  32KB I / 32KB D  │ │  32KB I / 32KB D  │   │
│  │  SCHED_FIFO 80    │ │  Panfrost KMS/DRM │ │  FBNeo / JIT/DSP  │ │  ZRAM / DMA / TUI │   │
│  │  bInterval=4 1ms  │ │  RGA2-Lite 2x Off │ │  mlock() / Shim   │ │  ax_hugepage_shim │   │
│  └─────────┬─────────┘ └─────────┬─────────┘ └─────────┬─────────┘ └─────────┬─────────┘   │
│            └─────────────────────┴──────────┬──────────┴─────────────────────┘             │
│                                   ┌─────────▼─────────┐                                    │
│                                   │  256 KB Shared L2 │ (64-Byte Cache Lines, Coherent)    │
│                                   └─────────┬─────────┘                                    │
│  ┌──────────────────────────────────────────▼───────────────────────────────────────────┐  │
│  │                 AXI / NIC-400 High-Speed Interconnect Matrix                         │  │
│  └──┬───────────────────┬───────────────────────┬───────────────────────┬───────────────┘  │
│     │                   │                       │                       │                  │
│  ┌──▼───────────────┐ ┌─▼───────────────────┐ ┌─▼───────────────────┐ ┌─▼───────────────┐  │
│  │  DMC (32-bit)    │ │  Mali-G31 MP2 GPU   │ │  VOP (Display Ctrl) │ │  Peripherals    │  │
│  │  720 MHz Default │ │  Bifrost Gen1 (2xEE)│ │  Atomic Page Flip   │ │  • DWC2 USB OTG │  │
│  │  (1440 MT/s)     │ │  520–620 MHz        │ │  640×480 @ 60.00 Hz │ │  • I2S0 (8ch)   │  │
│  │  5.76 GB/s Peak  │ │  16 KB Tile Buffer  │ │  MIPI DSI 4-Lane    │ │  • SDMMC0/1     │  │
│  └──┬───────────────┘ └─────────────────────┘ └─┬───────────────────┘ └─┬─┬─┬───────────┘  │
└─────┼───────────────────────────────────────────┼───────────────────────┼─┼─┼──────────────┘
      │                                           │                       │ │ │
      ▼                                           ▼                       │ │ │
┌───────────────────┐                   ┌───────────────────┐             │ │ │
│  1 GB LPDDR3 RAM  │                   │ 3.5" 640×480 IPS  │             │ │ │
│  (Shared System & │                   │ OCA Laminated LCD │             │ │ │
│   64 MB CMA Pool) │                   │ (Native 4:3)      │             │ │ │
└───────────────────┘                   └───────────────────┘             │ │ │
                                                                          │ │ │
  ┌───────────────────────────────────────────────────────────────────────┘ │ │
  │ I2C0 + I2S0                                                             │ │
  ▼                                                                         │ │
┌───────────────────────────────────────┐   ┌───────────────────────────────┘ │
│  ROCKCHIP RK817 PMIC & AUDIO CODEC    │   │ DWC2 USB 2.0 OTG (Core 0 IRQ)   │
│  • DCDC1 (VDD_ARM): 12.5 mV steps     │   ▼                                 │
│  • DCDC2 (VDD_LOGIC): 12.5 mV steps   │ ┌─────────────────────────────────┐ │
│  • Fuel Gauge: 3,200 mAh LiPo         │ │ Primary HiBy FC3 (VID 0x32BB)   │ │
│  • Fallback DAC: 3.5mm Jack + 1W Spk  │ │ ESS ES9281AC Pro (1.3 Vrms)     │ │
└───────────────────────────────────────┘ │ Bit-Perfect S32_LE / DoP Audio  │ │
                                          └─────────────────────────────────┘ │
                                                                              │
  ┌───────────────────────────────────────────────────────────────────────────┘
  │ SDMMC0 (3.3V SDR50, Core 3 IRQ)           SDMMC1 (3.3V SDR50, Core 3 IRQ)
  ▼                                           ▼
┌───────────────────────────────────────┐   ┌─────────────────────────────────┐
│ TF1: 64 GB SanDisk Extreme PRO        │   │ TF2: 128 GB SanDisk Extreme PRO │
│ • Raw 32KB: idbloader.img (SPL/TPL)   │   │ • p1 (exFAT/ext4, ~119 GB):     │
│ • Raw 8MB:  u-boot.itb (ATF BL31)     │   │   /roms, /music, /bios, /movies│
│ • p1 (FAT32, 256MB): Kernel/DTB/Logo  │   └─────────────────────────────────┘
│ • p2 (SquashFS zstd, <140MB): RootFS  │
│ • p3 (ext4, ~59GB): /userdata Saves   │
└───────────────────────────────────────┘
```

### Bus & Interrupt Wiring Summary (1:2:1 Core Isolation)

Kernel boot parameters (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate` enforce deterministic IRQ and thread pinning across all four physical cores:

* **Core 0 (`0x01` — Real-Time Audio & Controls):** Services `DWC2` USB-C OTG (`HiBy FC3` primary, `JCALLY JM6 Pro` and `HiBy Digital D1` backups) with USB microframe coalescing (`bInterval=4` `1 ms` service intervals) and audio-mode power floor management (`1.296 GHz` CPU Safe Mode / `666 MHz` DDR3 / `520 MHz` GPU floor), `I2S0` (`RK817`), the GPIO button matrix (`A/B/X/Y`, D-Pad, `L1/L2`, `R1/R2`, `Start`, `Select`, `FN`, `KEY_POWER`), and `10-bit` `SARADC` thumbstick IRQs alongside the `SCHED_FIFO` priority `80` ALSA `S32_LE` (`192`\-frame / `4.00 ms`) audio writer.  
* **Core 1 (`0x02` — GPU & Scanout):** Drives **Mali-G31 MP2** command submission via Mesa **Panfrost** (`GLES 3.0`), fixed-function **RGA2-Lite** 2D blitter offload (`CONFIG_VIDEO_ROCKCHIP_RGA=y` for `2x` integer `320x240` to `640x480` scaling), and atomic `KMS/DRM` page flips into the `VOP` \-\> `MIPI DSI` `640x480` panel.  
* **Core 2 (`0x04` — Emulation & DSP):** Executes cycle-accurate emulation (`FBNeo`) and `640x480` `mpv` media playback paired with `FLAC_AX` (with additional emulator cores planned for later releases) with isolated `32 KB` L1-I / `32 KB` L1-D caches, memory locking (`mlock()`), and `LD_PRELOAD` `2 MB` HugePage shim (`ax_hugepage_shim.so`).  
* **Core 3 (`0x08` — Storage DMA & Housekeeping):** Services `dwmmc` (`TF1` & `TF2` SanDisk Extreme PRO) DMA IRQs, `1024 MB` `LZ4` ZRAM compression, `64 MiB` `FLAC_AX` RAM preloading, `ax_appliance_tui`, and `ax-powerd`.

---

## 2\. Enabled Hardware Instruction Sets & Microarchitectural Features

**Piccolo-2go** compiles all user-space binaries (`BR2_TARGET_OPTIMIZATION="-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -fomit-frame-pointer"`) and kernel routines specifically for the **ARMv8-A Cortex-A35** in-order pipeline and **Mali-G31 MP2** (`Bifrost Gen1`) shader cores:

### CPU (ARMv8-A Cortex-A35 — aarch64)

* **Pipeline-Aware In-Order Scheduling (`-mcpu=cortex-a35 -mtune=cortex-a35`):** Aligns instruction issue pairs to the Cortex-A35's dual-issue in-order decoder and `64-byte` L1 cache lines, avoiding pipeline stalls on load-use hazards.  
* **Advanced SIMD / `128-bit` ARMv8 NEON (`BR2_ARM_FPU_NEON_FP_ARMV8=y`):** Enables `32 x 128-bit` vector registers (`V0–V31`) for single-cycle 4-lane `float32` / `int32` / `int16` DSP mixing in `FLAC_AX` / `libopenmpt`, software blitting, and `LZ4` ZRAM block compression (`~650 MB/s`).  
* **ARMv8 Cryptography & CRC32 Extensions (`+crc`, `+crypto`):** Hardware-accelerated `CRC32C` instructions for `ext4` metadata checksums, `SDMMC` block verification, and `FBNeo` ZIP ROM verification.  
* **Transparent HugePages & Memory Locking (`CONFIG_TRANSPARENT_HUGEPAGE_MADVISE=y`):** Maps large `FBNeo` Neo Geo / CPS-2 ROM arenas (`32–64 MB`) using `2 MB` hugepages via `madvise()` and `ax_hugepage_shim.so` paired with `mlock()` for audio arenas, reducing Cortex-A35 L1/L2 TLB pressure by `32x` without bloating background daemons.

### GPU (ARM Mali-G31 MP2 — Bifrost Gen1)

* **Quad-Vectorized Bifrost Shader Execution (`Panfrost GLES 3.0`):** Executes `FP32` / `FP16` (`mediump`) fragment shaders across two shader cores (`MP2`) with `16 KB` on-chip tile buffers for zero-overdraw `640x480` integer and sharp-bilinear scaling.  
* **Linear Scanout (`PAN_MESA_DEBUG=noafbc`):** Disables Arm Framebuffer Compression (`AFBC`) tile-header encode/decode passes on `640x480` 2D surfaces (`~1.2 MB/frame`), eliminating extra LPDDR3 read-modify-write cycles and feeding linear GBM buffers directly into the Rockchip `VOP`.  
* **Zero-Latency RAM Shader Cache (`MESA_SHADER_CACHE_DIR=/tmp/mesa_shader_cache`, `16M` cap):** Keeps compiled Panfrost GLSL programs in `tmpfs` LPDDR3 rather than read-only SquashFS or SD flash.  
* **RGA2-Lite 2D Scaling Offload (`CONFIG_VIDEO_ROCKCHIP_RGA=y`):** Offloads `2x` integer `320x240` to `640x480` scaling and YUV-to-RGB conversion to fixed-function hardware, freeing Mali-G31 MP2 shader resources for sub-pixel and CRT rendering effects.

---

## 3\. Architectural Review: Why This Topology Is Optimal & Key Improvements

### Why 1:2:1 Fixed-Hardware Topology Is Optimal for the RK3326

General-purpose handheld OS distributions run a desktop Linux scheduler across all four cores alongside X11/Wayland compositors, PulseAudio/PipeWire daemons, and background network/scraper services. On an **in-order** quad-core Cortex-A35 with `1 GB` LPDDR3 (no dedicated VRAM), thread migration immediately flushes the tiny `32 KB` L1 caches and injects multi-millisecond jitter into both audio buffers and `60.00 Hz` VBlank deadlines.

By partitioning the silicon into a strict **1:2:1 hardware topology** (`Core 0` Audio/IRQs with USB `bInterval=4` microframe coalescing, `Core 1` GPU/Scanout with RGA2-Lite 2D blitter offload, `Core 2` Emulation/DSP/Video with `mlock()` audio rings and `2 MB` HugePage shims, `Core 3` DMA/ZRAM/UI) and stripping every unused kernel and user-space subsystem, *Piccolo-2go* converts a budget handheld into a deterministic real-time appliance.

### Key Quantified Improvements Delivered

* **Thermal & Acoustic Stability:** Enforcing a `1344 MHz` (`1.344 GHz` / `1.34 GHz`) Default Mode alongside `1296 MHz` (`1.296 GHz` / `1.30 GHz`) Safe Mode and `1392 MHz` (`1.392 GHz` / `1.39 GHz`) OC / Boost Mode (with a `75°C` automatic downclock guard) prevents thermal throttling inside the unvented BATLEXP G350 shell (`42–52°C` typical).  
* **Deterministic Frame Pacing (`60.00 Hz`):** Dedicated `Core 1` Panfrost `KMS/DRM` atomic scanout \+ `PAN_MESA_DEBUG=noafbc` \+ `performance` devfreq governor eliminates mid-frame downclock stutter and compositor copy latency.  
* **Audiophile-Grade Bit-Perfect Transport:** Dedicated `Core 0` `SCHED_FIFO 80` ALSA `S32_LE` / `DoP` output, `bInterval=4` `1 ms` microframe coalescing, `1296 MHz` CPU / `666 MHz` DDR3 / `520 MHz` GPU audio-mode power floor management, `0x32BB` zero-autosuspend USB-C OTG rules, and pop-free `RK817` Class-D amp shutdown deliver noise-free `1.3 Vrms` audio on the early **HiBy FC3** (with `3.5mm` Moondrop Chu) and clean support for **JCALLY JM6 Pro** and **HiBy Digital D1** backup USB-C DACs.  
* **Instant Cold Boot & Corruption Immunity:** Raw Rockchip `idbloader.img` (`32 KB`) \+ `u-boot.itb` (`8 MB`) \+ `640x480` `logo.bmp` splash \+ `LZ4` kernel (`< 180 ms` decompression) into a `< 140 MB` `128 KB`\-block `zstd` SquashFS rootfs boots straight into `ax_appliance_tui` with zero `uInitrd` overhead and complete immunity to abrupt power loss.

---

## 4\. Technical Reference Summary

| Subsystem | Specification / Implementation Detail |
| :---- | :---- |
| **Target Hardware & DTB** | BATLEXP G350 & all R36S revisions (Default DTB: `/boot/rk3326-g350-linux.dtb`, `dtb-select` runtime panel selector) |
| **CPU & Core Partition** | Rockchip RK3326 (*4x ARM Cortex-A35*, *aarch64*), `1:2:1` (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) |
| **CPU PLL Steps** | `1296000 kHz` (*1.296 GHz* / *1.30 GHz* Safe Mode @ *1100.0 mV*) \| `1344000 kHz` (*1.344 GHz* / *1.34 GHz* Default Mode @ *1112.5 mV*) \| `1392000 kHz` (*1.392 GHz* / *1.39 GHz* OC / Boost Mode) |
| **GPU & Display** | ARM Mali-G31 MP2 (*Panfrost GLES 3.0*, RGA2-Lite 2D blitter offload via `CONFIG_VIDEO_ROCKCHIP_RGA=y` for `2x` `320x240` \-\> `640x480` scaling, `PAN_MESA_DEBUG=noafbc`, `/tmp/mesa_shader_cache`), `520–620 MHz` (`600 MHz` default @ `1137.5 mV`), `KMS/DRM` `640x480 @ 60.00 Hz` |
| **Memory & ZRAM** | `1 GB` `32-bit` LPDDR3 @ `720 MHz` Default (*1440 MT/s* @ *1112.5 mV*; *666 MHz* Safe Mode @ *1100.0 mV*; *800–900 MHz* OC / Boost open-case) \+ `1024 MB` `LZ4` ZRAM (`vm.swappiness=120`, `vm.page-cluster=0`, `vm.watermark_scale_factor=125`) \+ `THP=madvise` \+ `mlock()` audio arenas \+ `LD_PRELOAD` `2 MB` HugePage shim `ax_hugepage_shim.so` |
| **Bootloader Chain** | `idbloader.img` (`32 KB` raw offset) \-\> ATF `BL31` \+ U-Boot `2025.01` `u-boot.itb` (`8 MB` raw offset) \-\> `640x480` 24-bit `logo.bmp` splash \-\> `LZ4` Linux `6.12.15` (`booti ${loadaddr} - ${dtb_loadaddr}`) |
| **Root Filesystem** | Immutable `< 140 MB` `128 KB`\-block `zstd` SquashFS on `TF1` `p2`, booting directly into `/usr/bin/ax_appliance_tui` on `tty1` |
| **Dual SD Storage** | **TF1 (64 GB SanDisk Extreme PRO):** `p1` `256 MB` `FAT32` (`BOOT`), `p2` SquashFS, `p3` auto-expanded `ext4` (`/userdata`) **TF2 (128 GB SanDisk Extreme PRO):** `p1` `exFAT`/`ext4` bound to `/userdata/{roms,music,bios,movies}` **Bus Guard:** Runtime `SDR50` (`50 MHz`, `3.3V`) verification with automatic `25 MHz` High-Speed fallback & console warning |
| **Audio & Video Subsystem** | **Primary DAC:** HiBy FC3 (£29.61, *ES9281 PRO / ES9281AC Pro*, `1.3 Vrms`, VID `0x32BB`, `S32_LE` / `DoP`, `ax-audio-route prompt` *Connect & Retry*) \+ `3.5mm` Moondrop Chu **Backup USB-C DACs:** JCALLY JM6 Pro (£12.99, `CX31993 + MAX97220`) & HiBy Digital D1 (£11.11, `CS46L41`) **Fallback:** Internal `RK817` codec (`3.5 mm` jack \+ `1W` mono speaker) **Video:** `640x480` `mpv` reading `/userdata/movies` |
| **Software Stack** | `ax-sdk` (`C++20`), `FLAC_AX` (`S32_LE`/`DoP` \+ `libopenmpt` \+ USB `DWC2` `bInterval=4` `1 ms` microframe coalescing & `1296 MHz` CPU / `666 MHz` DDR3 / `520 MHz` GPU audio-mode power floor), `ax_appliance_tui` (Espresso/Copper TUI), RetroArch \+ `FBNeo` (`640x480` integer KMS), `mpv` (`640x480` KMS/DRM \+ ALSA), `ax-powerd` (`1.5s` `KEY_POWER` / `FN+Power`, `75°C` rollback, `3280 mV` LiPo guard), `dtb-select` |

---

## 5\. v1.1 Mixed-Signal & Silicon Refinements (FBNeo \+ FLAC\_AX \+ 640x480 Movie Playback)

* **USB `DWC2` Microframe Coalescing & Audio-Mode Power Floor:** Applies `bInterval=4` for `1 ms` service intervals on `Core 0` and enforces a `1296 MHz` (`1.296 GHz` / `1100.0 mV`) Safe Mode CPU / `666 MHz` DDR3 / `520 MHz` GPU floor during pure `FLAC_AX` playback to keep VBUS switching noise minimal for the ESS Sabre `ES9281AC Pro` / `ES9281PRO` DAC (and compatible backup USB-C DACs).  
* **Fixed-Function RGA2-Lite 2D Blitter Offload (`CONFIG_VIDEO_ROCKCHIP_RGA=y`):** Offloads `2x` integer `320x240` to `640x480` scaling and YUV-to-RGB conversion to hardware, freeing Mali-G31 MP2 shader resources for sub-pixel and CRT rendering effects.  
* **Memory Locking & `2 MB` HugePage Allocation:** Executes explicit `mlock()` on `FLAC_AX` SPSC audio rings and `64 MiB` preload buffers under `vm.swappiness=120`, paired with an `LD_PRELOAD` `2 MB` HugePage shim (`ax_hugepage_shim.so`) for `FBNeo` ROM arenas larger than `2 MB`.

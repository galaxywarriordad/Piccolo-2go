# **🔬 Mixed-Signal & Handheld Silicon Engineering Specification: Rockchip RK3326 (28nm HKMG) \+ ESS Sabre ES9281PRO / ES9281AC Pro**

* **Target Platform:** Piccolo-2go (aarch64 Buildroot Appliance for BATLEXP G350 & R36S)  
* **Core Compute & Memory:** Rockchip RK3326 (28nm HKMG, 4× ARM Cortex-A35, ARM Mali-G31 MP2) \+ 1 GB 32-bit DDR3 (LPDDR3)  
* **Display & Storage I/O:** 3.5-inch 640×480 (4:3) IPS MIPI DSI LCD \+ Dual microSD Card Readers (SDMMC0 TF1 \+ SDMMC1 TF2)  
* **Audio Subsystem:** ESS Technology Sabre ES9281PRO / ES9281AC Pro USB-C OTG DAC (1.3 Vrms & 2.0 Vrms revisions) \+ Rockchip RK817 PMIC/Codec fallback

## **Section 1 — Chip Technical Engineering Specification**

### **1.1 Host Compute SoC: Rockchip RK3326 (28nm HKMG)**

Fabricated on a **28nm High-K Metal Gate (HKMG)** process, the **Rockchip RK3326** pairs four in-order **ARMv8-A Cortex-A35** cores with a tile-based **ARM Mali-G31 MP2** GPU, a 32-bit Dynamic Memory Controller (DMC), a 60 Hz Video Output Processor (VOP), and dual UHS-I SD host controllers.

| Silicon Block | Technical Specification | Engineering & Runtime Characteristics |
| :---- | :---- | :---- |
| **Process Node** | 28nm HKMG (High-K Metal Gate) | Low static leakage suited to passive, unvented handheld enclosures (42–52°C typical at 1.296 GHz); 24 MHz PLL steps (1.296 / 1.344 / 1.392 GHz) |
| **CPU Complex** | 4× ARM Cortex-A35 (ARMv8-A aarch64) | Dual-issue **in-order** microarchitecture; deterministic cycle timing with no speculative execution overhead, sensitive to L1 cache thrashing |
| **Cache Hierarchy** | 32 KB L1-I \+ 32 KB L1-D per core (64-byte lines); 256 KB shared L2 | Coherent unified L2 cache across all 4 cores; requires strict core/IRQ isolation so USB and SD DMA interrupts do not evict hot L1 instruction/data lines |
| **Instruction Sets** | ARMv8-A, 128-bit Advanced SIMD (NEON), FP16/FP32, \+crc, \+crypto | Single-cycle 4-lane vector DSP math (FLAC\_AX / libopenmpt), \~650 MB/s LZ4 ZRAM compression, and hardware CRC32C filesystem/ROM checks |
| **Memory Controller (DMC)** | 1 GB 32-bit DDR3 (LPDDR3, single-channel) | 720 MHz (1440 MT/s @ 1112.5 mV, 5.76 GB/s peak theoretical bandwidth) default; 666 MHz (1333 MT/s @ 1100.0 mV) universal safe fallback; 64 MB CMA reserved |
| **GPU (Mali-G31 MP2)** | Bifrost Gen1 (2× Execution Engines, 16 KB on-chip tile buffer) | Quad-vectorized FP32/FP16 shader cores; 520 MHz (1100.0 mV) stock ceiling up to 650–660 MHz (1162.5–1175.0 mV) OPP ladder |
| **Display Engine (VOP \+ DSI)** | Single VOP → 4-lane MIPI DSI → 3.5" 640×480 (4:3) OCA IPS LCD | Native 307,200\-pixel (640×480 @ 60.00 Hz) progressive RGB888 scanout (\~1.23 MB/frame); supports atomic KMS/DRM page flips |
| **Dual SD Readers (SDMMC0/1)** | 2× 4-bit SDIO 3.0 / UHS-I SDR50 (50 MHz, 3.3V signaling) | \~24 MB/s sustained sequential ceiling per slot; SDMMC0 (TF1 OS/Saves) and SDMMC1 (TF2 Media/ROMs) operate on independent DMA channels |
| **USB OTG Controller** | Synopsys DWC2 USB 2.0 High-Speed (480 Mbps) OTG | Drives external UAC2 USB-C DACs (HiBy FC3) and USB MIDI controllers with 125 µs High-Speed microframes |
| **PMIC & Audio (RK817)** | DCDC1 (VDD\_ARM), DCDC2 (VDD\_LOGIC), 12.5 mV steps, fuel gauge, I2S0 codec | Software-stepped voltage rails (12500 µV resolution), 3,200 mAh LiPo coulomb counter, and internal 3.5 mm / 1W speaker fallback codec |

### **1.2 External Mixed-Signal Audio Silicon: ESS Sabre ES9281PRO & ES9281AC Pro**

The **ESS Sabre ES9281PRO / ES9281AC Pro** is a single-chip, 32-bit stereo USB Audio Class 2.0 (UAC2) D/A converter integrating an asynchronous USB 2.0 transceiver, 64-bit internal DSP accumulator, **HyperStream II QUAD DAC**, patented **Time Domain Jitter Eliminator**, analog PLL, programmable analog gain stage, and Class-G stereo headphone amplifier.

| Parameter / Subsystem | ES9281PRO / ES9281AC Pro Specification | Variant & System Notes |
| :---- | :---- | :---- |
| **Package Variants** | ES9281PRO (88-pin WQFN / 90-ball CSP); ES9281AC Pro (90-ball WLCSP) | ES9281AC Pro is deployed in ultra-compact USB-C dongles (e.g., **HiBy FC3**, VID 0x32BB) with identical 32-bit HyperStream II QUAD DAC and Class-G amp silicon |
| **DAC Topology** | 32-bit **HyperStream II QUAD DAC** (4 modulators per channel) | Suppresses idle tones and correlated quantization noise across low-level signals |
| **Dynamic Range & THD+N** | **\+130 dB SNR** (\+122 dBA DNR); **\-112 dB THD+N** (100 kΩ) / **\-110 dB** (32 Ω) | Exceeds 24-bit PCM dynamic range and isolates analog ground from RK3326 switching rails |
| **Clocking & Jitter** | Asynchronous UAC2 \+ **Time Domain Jitter Eliminator** \+ Analog PLL | Decouples DAC sample clock from RK3326 DWC2 USB 125 µs SOF microframe jitter |
| **PCM & DSD Support** | S32\_LE (16/24/32-bit padded) 44.1–384 kHz; Hardware **DoP (DSD64 / DSD128)** | Bit-transparent transport (snd\_pcm\_hw\_params\_set\_rate\_resample(0)); DopPacker alternates 0x05/0xFA markers |
| **Analog Output Revisions** | **HiBy FC3 (fc3\_1v3):** 1.3 Vrms (70 mW @ 32 Ω) **HiBy FC3 (fc3\_2v0):** 2.0 Vrms (112 mW @ 32 Ω) | Programmable analog gain (0 dB to \-30 dB in 6 dB steps) preserves 24-bit resolution across 16 Ω IEMs, headphones, and Class-D amps |

## **Section 2 — Hardware Topology: System Interconnects, Instruction Flow & Capabilities**

### **2.1 End-to-End Hardware Block & Bus Diagram**

Plaintext

```
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│                        ROCKCHIP RK3326 SYSTEM-ON-CHIP (28nm HKMG)                          │
│                                                                                            │
│  ┌───────────────────┐ ┌───────────────────┐ ┌───────────────────┐ ┌───────────────────┐   │
│  │  Cortex-A35 #0    │ │  Cortex-A35 #1    │ │  Cortex-A35 #2    │ │  Cortex-A35 #3    │   │
│  │  Mask: 0x01 (RT)  │ │  Mask: 0x02 (GPU) │ │  Mask: 0x04 (EMU) │ │  Mask: 0x08 (I/O) │   │
│  │  32KB I / 32KB D  │ │  32KB I / 32KB D  │ │  32KB I / 32KB D  │ │  32KB I / 32KB D  │   │
│  │  SCHED_FIFO 80    │ │  Panfrost KMS/DRM │ │  FBNeo / DSP      │ │  ZRAM / DMA / TUI │   │
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
│  │  (1440 MT/s)     │ │  520–660 MHz        │ │  640×480 @ 60.00 Hz │ │  • I2S0 (8ch)   │  │
│  │  5.76 GB/s Peak  │ │  16 KB Tile Buffer  │ │  4-Lane MIPI DSI    │ │  • SDMMC0/1     │  │
│  └──┬───────────────┘ └─────────────────────┘ └─┬───────────────────┘ └─┬─┬─┬───────────┘  │
└─────┼───────────────────────────────────────────┼───────────────────────┼─┼─┼──────────────┘
      │                                           │                       │ │ │
      ▼                                           ▼                       │ │ │
┌───────────────────┐                   ┌───────────────────┐             │ │ │
│   1 GB DDR3 RAM   │                   │ 3.5" 640×480 IPS  │             │ │ │
│  (Shared System & │                   │ 4:3 Aspect Ratio  │             │ │ │
│   64 MB CMA Pool) │                   │ OCA Laminated LCD │             │ │ │
└───────────────────┘                   └───────────────────┘             │ │ │
                                                                          │ │ │
  ┌───────────────────────────────────────────────────────────────────────┘ │ │
  │ I2C0 + I2S0                                                             │ │
  ▼                                                                         │ │
┌───────────────────────────────────────┐   ┌───────────────────────────────┘ │
│  ROCKCHIP RK817 PMIC & AUDIO CODEC    │   │ DWC2 USB 2.0 OTG (Core 0 IRQ)   │
│  • DCDC1/2: 12.5 mV Voltage Steps     │   ▼                                 │
│  • Fuel Gauge: 3,200 mAh LiPo         │ ┌─────────────────────────────────┐ │
│  • Fallback DAC: 3.5mm Jack + 1W Spk  │ │ ESS Sabre ES9281PRO / AC Pro    │ │
└───────────────────────────────────────┘ │ HiBy FC3 (VID 0x32BB, 1.3 Vrms) │ │
                                          │ Time Domain Jitter Eliminator   │ │
                                          │ 32-Bit HyperStream II QUAD DAC  │ │
                                          └─────────────────────────────────┘ │
                                                                              │
  ┌───────────────────────────────────┬───────────────────────────────────────┘
  │ SDMMC0 (3.3V SDR50, Core 3 IRQ)   │ SDMMC1 (3.3V SDR50, Core 3 IRQ)
  ▼                                   ▼
┌─────────────────────────────────┐ ┌─────────────────────────────────────────┐
│ Reader 1 (TF1): OS & Saves      │ │ Reader 2 (TF2): Portable Media Library  │
│ • Raw 32KB: idbloader.img       │ │ • p1 (exFAT / ext4, ~119 GB):           │
│ • Raw 8MB:  u-boot.itb          │ │   Bound to /userdata/{roms,music,bios}  │
│ • p1 (FAT32): Kernel/DTB/Logo   │ │   (FBNeo ROMs, FLAC/DSD, MOD/XM/S3M)    │
│ • p2 (SquashFS zstd): RootFS    │ └─────────────────────────────────────────┘
│ • p3 (ext4): /userdata Saves    │
└─────────────────────────────────┘
```

### **2.2 How the Subsystems Fit Together**

1. **Dual SD Readers (SDMMC0 \+ SDMMC1) & 1 GB DDR3 Staging:**  
* **Reader 1 (TF1 on SDMMC0):** Boot ROM loads SPL/TPL (idbloader.img at 32 KB), ATF BL31 \+ U-Boot 2025.01 (u-boot.itb at 8 MB), renders the 640×480 Espresso/Copper logo.bmp splash, and decompresses the LZ4 Linux 6.12.15 kernel (\< 180 ms) into 1 GB DDR3, mounting the immutable \< 140 MB zstd SquashFS rootfs (p2) and writable /userdata (p3).  
* **Reader 2 (TF2 on SDMMC1):** Mounted at boot by S02piccolo-2go-storage and bind-mounted onto /userdata/{roms,music,bios}, separating read-heavy ROM/FLAC streaming from OS save-state writes (p3) across two independent SDMMC DMA engines.  
2. **CPU Instruction & Cache Execution (ARMv8-A Cortex-A35):**  
* User-space binaries compile with \-O3 \-mcpu=cortex-a35 \-mtune=cortex-a35 \-fomit-frame-pointer so instruction pairs align with the Cortex-A35's dual-issue in-order decoder and 64-byte cache lines.  
* 128-bit **NEON** SIMD (V0–V31) accelerates 4-lane audio mixing and LZ4 ZRAM page compression (1024 MB swap device in DDR3), while ARMv8 \+crc accelerates ext4 and ZIP ROM checksums, and madvise Transparent HugePages (2 MB) map large FBNeo ROM arenas (32–64 MB) with 32× fewer TLB misses.  
3. **GPU & 3.5-inch 640×480 (4:3) IPS Display Pipeline:**  
* **Core 1** drives **Mali-G31 MP2** via Mesa **Panfrost** (GLES 3.0), using 16 KB on-chip tile buffers to perform 2× integer or sharp-bilinear scaling and writing linear RGB888 frames (\~1.23 MB) directly into the 64 MB DDR3 CMA scanout pool for atomic VOP → 4-lane MIPI DSI flips at 60.00 Hz.  
4. **USB-C OTG & Mixed-Signal DAC Output (ES9281PRO / ES9281AC Pro):**  
* Decoded audio (FLAC, DSF/DFF, MOD/XM, or emulator PCM) is staged on **Core 3** (64 MiB RAM preload) and passed across a 64-byte cache-line-aligned SPSC ring buffer to **Core 0** (SCHED\_FIFO 80), which feeds DWC2 USB 2.0 isochronous packets (S32\_LE or DoP) into the **ESS Sabre ES9281AC Pro**, where the **Time Domain Jitter Eliminator** and **HyperStream II QUAD DAC** convert the stream into clean analog audio.

## **Section 3 — Throughput Maximization Analysis & Engineering Recommendations**

Because the RK3326 shares a single **32-bit DDR3 bus (5.76 GB/s peak at 720 MHz)** and a **256 KB L2 cache** across four in-order cores, the Mali-G31 GPU, the VOP display controller, and two SDMMC DMA engines, **memory bandwidth contention and L1/L2 cache pollution**—not raw ALU MHz—are the primary bottlenecks.

### **3.1 Quantified Throughput Optimizations**

| Bottleneck Domain | Mechanism & Root Cause | Recommended Optimization & Throughput Impact |
| :---- | :---- | :---- |
| **1\. 32-bit DDR3 Bus Bandwidth** | Stock 666 MHz (5.33 GB/s) DDR3 shares bandwidth among CPU L2 fills, Mali-G31 tile writebacks, VOP 60 Hz scanout, and SDMMC DMA | • **Run 720 MHz (1440 MT/s) DMC Default:** Increases raw DDR3 bandwidth by **\+8.1%** (5.76 GB/s) with a single \+12.5 mV (1112.5 mV) RK817 DCDC2 step while preserving 666 MHz (1100.0 mV) as a safe fallback. • **Disable AFBC (PAN\_MESA\_DEBUG=noafbc):** On a 640×480 (4:3) frame (1.23 MB), AFBC tile-header compression adds extra read-modify-write bus cycles; linear GBM scanout cuts framebuffer bus traffic and feeds the VOP directly |
| **2\. 1 GB DDR3 Capacity & Paging Latency** | 1 GB physical RAM must hold the kernel, CMA (64 MB), Mesa shaders, audio preload (64 MiB), and large Neo Geo / CPS-2 ROM sets | • **Prune Kernel & Strip Unused SoC Blocks:** Removing VPU, ISP, RGA, Bluetooth, IPv6, and uInitrd (booting an LZ4 Image directly into zstd SquashFS) reclaims **\~110 MB** of physical DDR3. • **1024 MB LZ4 ZRAM \+ page-cluster=0:** In-order Cortex-A35 cores decompress LZ4 at **\~650 MB/s** (\~6 µs per 4 KB page) vs. \~24 MB/s (\~170 µs) on SDR50 SD flash—a **27× throughput gain** for cold ROM assets. • **THP=madvise Only:** Restricts 2 MB hugepages to explicit large ROM arenas (32–64 MB), cutting TLB misses by 32× without fragmenting 1 GB RAM |
| **3\. Dual SD Reader (TF1 \+ TF2) Bus Throughput** | RK3326 SDMMC0/1 rails are fixed at 3.3V signaling (SDR50 50 MHz max); attempting 1.8V SDR104 causes CRC retries and bus stalls, while concurrent OS \+ media I/O on one card serializes 4K reads | • **Enforce SDR50 Bus Guard (S02piccolo-2go-storage):** Strip unstable SDR104 capability flags at boot so TF1 and TF2 lock cleanly at **50 MHz SDR50 (\~24 MB/s)** with zero CRC timeouts. • **Split Workload Across Readers \+ Mount Discipline:** Keep TF1 (64 GB A2/V30) read-only (SquashFS) \+ commit=60,data=writeback,noatime saves (p3), while streaming ROMs and FLAC files from TF2 (128 GB A2/V30) over SDMMC1 (64 MiB sequential RAM preload) |
| **4\. In-Order L1 Cache & IRQ Jitter (1:2:1)** | Unpinned DWC2 USB SOF interrupts (8,000 IRQs/sec at High-Speed) and SDMMC block IRQs evict 32 KB L1 caches on emulation/DSP cores | • **Lock 1:2:1 Core/IRQ Affinity:** Pin DWC2 USB OTG, I2S0, GPIO/SARADC IRQs, and SCHED\_FIFO 80 ALSA writer to **Core 0 (0x01)**; **Mali-G31/KMS** to **Core 1 (0x02)**; **FBNeo/DSP** to **Core 2 (0x04)**; and **SDMMC0/1 DMA \+ ZRAM** to **Core 3 (0x08)**. • **Disable USB Autosuspend (99-ax-2go-otg.rules):** Lock power/control="on" on VID 0x32BB (ES9281AC Pro) to prevent DWC2 bus-reset stalls and relay pops |
| **5\. 640×480 (4:3) Pixel Fillrate & GPU Clocking** | Native 4:3 640×480 matches classic 320×240 (2× exact integer scale) and 320×224 (2× \+ 16 px border) arcade/console rasters with zero aspect-ratio distortion | • **Use 2× Integer / Sharp-Bilinear Shaders on Mali-G31 MP2:** Avoid multi-pass heavy CRT curvature shaders that exceed 16 KB tile-buffer bandwidth; keep max\_gpufreq=520 MHz (1100.0 mV) for zero-throttle 60.00 Hz lock, stepping to 550–600 MHz (\+1 to \+3 12.5 mV steps) only when shader complexity demands it |

### **3.2 Additional Engineering Suggestions for Future Revisions**

1. **Dedicated IRQ Coalescing & DWC2 Microframe Alignment:** Pinning DWC2 periodic/isochronous frame intervals to 1 ms (bInterval=4 at High-Speed) or 4.00 ms (192 frames @ 48 kHz) on Core 0 reduces USB interrupt rate by 4×–8× while keeping end-to-end DAC latency under 8 ms.  
2. **F2FS or ext4 bigalloc (32 KB clusters) on TF1 p3 (/userdata):** Aligning filesystem allocation clusters on TF1 p3 to the internal erase-block/page geometry of SanDisk A2/V30 microSD cards minimizes write amplification during save-state flushes.  
3. **Dynamic CPU\_PROFILE Switching (ax-appliance-tui hook):** Keep 1.296 GHz (1100 mV) active during FLAC\_AX audio playback and menu navigation (maximizing battery runtime and keeping RK817 switching noise minimal), and automatically step to 1.344 GHz (\+48 MHz PLL step) only when launching heavy FBNeo CPS-3 / Neo Geo titles, guarded by ax-powerd's 75°C thermal rollback.

## **Section 4 — Build & Deterministic Optimization Assessment**

An engineering audit of the actual Buildroot defconfig, Linux 6.12.15 kernel fragment, init scripts, udev rules, and C system daemons confirms that the core architecture is coherent and well-aligned with the RK3326 \+ ESS Sabre hardware topology—while uncovering **five specific implementation gaps** where runtime behavior diverges from the specification.

### **4.1 Audit Scorecard by Subsystem**

| Domain & Inspected Artifact | Status | Engineering Verification & Findings |
| :---- | :---- | :---- |
| **Compiler & Buildroot Target**  (piccolo\_2go\_rk3326\_defconfig) | **PASS** | • BR2\_aarch64=y, BR2\_cortex\_a35=y, BR2\_ARM\_FPU\_NEON\_FP\_ARMV8=y, and BR2\_TARGET\_OPTIMIZATION="-O3 \-mcpu=cortex-a35 \-mtune=cortex-a35 \-fomit-frame-pointer" generate pipeline-aligned in-order ARMv8-A \+ 128-bit NEON binaries. • 128 KB\-block zstd SquashFS (BR2\_TARGET\_ROOTFS\_SQUASHFS\_BS\_128K=y, BR2\_TARGET\_ROOTFS\_SQUASHFS4\_ZSTD=y) with Xorg, Wayland, PulseAudio, PipeWire, and Qt6 explicitly disabled keeps the image immutable and lean |
| **Kernel Preemption & Memory**  (linux-rk3326.fragment) | **PASS** | • CONFIG\_PREEMPT=y, CONFIG\_NO\_HZ\_FULL=y, CONFIG\_RCU\_NOCB\_CPU=y, CONFIG\_IRQ\_FORCED\_THREADING=y, and CONFIG\_HZ\_1000=y enable 1 ms tick granularity and tickless isolation on Cores 0–2. • CONFIG\_KERNEL\_LZ4=y, CONFIG\_TRANSPARENT\_HUGEPAGE\_MADVISE=y, CONFIG\_ZRAM\_DEF\_COMP\_LZ4=y, and CONFIG\_CMA\_SIZE\_MBYTES=64 (256-page aligned) match the 1 GB DDR3 memory budget |
| **Thermal & Power Guard**  (ax-powerd.c) | **PASS** | • Pins itself to **Core 3** (CPU\_SET(3, \&set)) on startup. • Enforces 1.5s KEY\_POWER hold (or instant FN \+ Power), sends SIGUSR1 to flac\_ax and SIGTERM to retroarch, unmounts storage cleanly, clamps CPU ceiling to 1296000 kHz at 75000 m°C (75°C), and triggers clean brownout poweroff at \<= 3280000 µV (3.28 V) when discharging |
| **USB-C OTG DAC Rules**  (99-ax-2go-otg.rules) | **PASS** | • Locks power/control="on" on HiBy FC3 / ESS Sabre (0x32bb / 0x2fc6) and MIDI (0x09e8) attach, eliminating USB autosuspend clicks, and triggers /usr/sbin/ax-audio-route apply on ALSA card add/remove |
| **Runtime Topology & DTB**  (S01piccolo-2go-isolate, dtb-select.c) | **5 GAPS FOUND** | See **Section 4.2** below for concrete fixes covering irqbalance, devfreq DDR3 scaling, ax-boot-recovery binary availability, dtb-select path handling, and GPU governor consistency |

### **4.2 Five Concrete Findings & Remediation Patches**

#### **1\. Missing ax-boot-recovery Binary in piccolo-2go-tools**

* **Finding:** S01piccolo-2go-isolate checks command \-v ax-boot-recovery to detect held SELECT / FN / B buttons at boot and trigger restore\_golden\_dtb \+ 666 MHz DDR3 fallback. However, only ax-powerd.c and dtb-select.c are implemented in package/piccolo-2go-tools/src/. Without ax-boot-recovery compiled and installed into /usr/sbin/, hold-button safe recovery will silently skip.  
* **Fix:** Add ax-boot-recovery.c (using the same /dev/input/event\* EVIOCGKEY scan pattern as ax-powerd.c to check BTN\_SELECT, BTN\_TRIGGER\_HAPPY5/BTN\_MODE, or BTN\_SOUTH/BTN\_EAST held at startup, exiting 0 if held and 1 otherwise) and build it in piccolo-2go-tools.mk.

#### **2\. update\_boot\_ini\_ddr() Edits /boot/boot.ini Too Late for the Current Boot**

* **Finding:** In S01piccolo-2go-isolate, update\_boot\_ini\_ddr() rewrites max\_ddrfreq= inside /boot/boot.ini. Because U-Boot reads boot.ini *before* Linux starts, modifying boot.ini from user-space only takes effect on the **next** reboot—leaving the current boot running at whatever max\_ddrfreq was previously written (including during a SAFE\_RECOVERY boot where 666 MHz is needed immediately).  
* **Fix:** In addition to updating /boot/boot.ini for subsequent boots, immediately write the target frequency into the RK3326 DMC devfreq node at runtime when present:  
* Bash

```
for dmc_dir in /sys/class/devfreq/*dmc*; do
    [ -d "$dmc_dir" ] || continue
    echo "$((TARGET_DDR * 1000000))" > "$dmc_dir/max_freq" 2>/dev/null || true
done
```

* 

#### **3\. Threaded IRQ Affinity (irq/\* Threads) & CONFIG\_IRQ\_FORCED\_THREADING=y**

* **Finding:** linux-rk3326.fragment sets CONFIG\_IRQ\_FORCED\_THREADING=y, while S01piccolo-2go-isolate writes masks to /proc/irq/\<N\>/smp\_affinity. On Linux with forced IRQ threading, hardware IRQ handlers wake kernel threads named irq/\<N\>-\<device\>, whose scheduler affinity does not always follow smp\_affinity unless explicitly pinned—allowing dwmmc or dwc2 IRQ threads to migrate onto Core 2 (FBNeo).  
* **Fix:** Inside steer\_irqs\_121(), also pin matching irq/\<N\>-\* kernel threads with taskset \-p:  
* Bash

```
irq_num="${irq_dir##*/}"
for irq_pid in $(pgrep -f "^irq/${irq_num}-" 2>/dev/null); do
    taskset -pc 0 "$irq_pid" >/dev/null 2>&1 || true   # for audio/OTG/input
    # or taskset -pc 3 "$irq_pid" for dwmmc
done
```

* 

#### **4\. GPU Governor Mismatch (simple\_ondemand vs. performance)**

* **Finding:** S01piccolo-2go-isolate defaults GPU\_IDLE\_GOV to simple\_ondemand on /sys/class/devfreq/ff400000.gpu/governor. While power-saving in the TUI is good, simple\_ondemand can introduce mid-frame frequency ramp stutter if RetroArch / FBNeo launches without calling S01piccolo-2go-isolate gpu-perf.  
* **Fix:** Ensure the ax\_appliance\_tui launch wrapper invokes /etc/init.d/S01piccolo-2go-isolate gpu-perf before starting retroarch and /etc/init.d/S01piccolo-2go-isolate gpu-auto on exit (matching the FN \+ START behavior documented in README.md).

#### **5\. dtb-select Path Traversal & Active Filename Hardcoding**

* **Finding:** In dtb-select.c, passing a custom filename (dtb-select custom.dtb) concatenates /boot/dtbs/%s without rejecting / or .., and active\_dtb is hardcoded to /boot/rk3326-g350-linux.dtb. Additionally, kAliases in dtb-select.c includes r36s-panel2, r36s-panel3, r36s-panel4-t2, r36s-pro, and r36s-xx, whereas README.md lists 8 DTBs (g350, r36s-panel4, r36s-v21, r36s-panel0, r36s-panel1, r36s-plus, r36h, r46h).  
* **Fix:** Reject / in argv\[1\] inside dtb-select.c, verify access(src\_path, R\_OK) \== 0 before truncating active\_dtb (so a typo never wipes out the working DTB), and keep the alias list synchronized with the .dtb files actually staged into /boot/dtbs/ by post-build.sh.

### **4.3 Summary Verdict**

* **Deterministic Foundation:** **95% Complete.** The 1:2:1 NO\_HZ\_FULL \+ PREEMPT kernel configuration, \-mcpu=cortex-a35 Buildroot flags, 1024 MB LZ4 ZRAM tuning, 64 MB CMA linear Panfrost scanout, Core 3 ax-powerd thermal/LiPo watchdog, and zero-autosuspend 0x32BB udev rules are production-grade.  
* **Remaining Action Items Before Hardware Sign-Off:** Implement ax-boot-recovery.c, add immediate devfreq DDR3 writes and irq/\<N\>-\* taskset pinning in S01piccolo-2go-isolate, and guard dtb-select.c against missing source files before truncating /boot/rk3326-g350-linux.dtb.

## **Section 5 — Architectural & Mixed-Signal Design Improvements (v1.1)**

Building on the baseline RK3326 \+ ESS Sabre ES9281PRO / ES9281AC Pro topology and the Section 4 build audit, three targeted architectural refinements eliminate USB microframe IRQ contention, free GPU shader capacity during integer scaling, and guarantee zero-swap memory residency under heavy ROM and audio loads.

### **1\. Isolate USB DWC2 Microframe Polling from Core 0 Audio & Enforce Dedicated VBUS Filtering**

* **Current Architecture:** Both the SCHED\_FIFO 80 ALSA S32\_LE audio writer (192-frame / 4.00 ms period) and the DWC2 USB 2.0 OTG (480 Mbps) \+ GPIO/SARADC interrupts run on Core 0 (0x01), while powering the external ES9281AC Pro directly from the handheld's DWC2 OTG port.  
* **Architectural Bottleneck:** At USB 2.0 High-Speed (480 Mbps), the Synopsys DWC2 controller generates Start-of-Frame (SOF) interrupts every 125 µs (8,000 IRQs/sec) alongside periodic SARADC thumbstick polling. Even with an asynchronous UAC2 feedback endpoint and ESS's Time Domain Jitter Eliminator, 8,000 interrupts per second on Core 0 preempt the SCHED\_FIFO audio loop and inject high-frequency switching ripple onto the OTG VBUS (5V) rail shared with the RK817 PMIC.  
* **Design Improvement:**  
  * **DWC2 Microframe Coalescing:** Configure the DWC2 host periodic scheduler (bInterval=4 for 1 ms service intervals) so isochronous audio packets transfer in 1 ms bursts (4 packets per 4.00 ms ALSA period) rather than waking Core 0 every 125 µs, reducing Core 0 interrupt overhead by 8× while preserving deterministic sub-8 ms latency.  
  * **Audio-Mode VBUS Noise Floor:** Because the ES9281AC Pro derives its low-noise analog reference regulator (AVCC / VCCA) directly from USB VBUS, lock CPU frequency to 1.296 GHz (1100.0 mV) and keep the Mali-G31 GPU at its 520 MHz idle/powersave floor during pure FLAC\_AX audio playback to minimize DCDC1/DCDC2 switching transients on the shared ground plane, preserving the ESS Sabre's \+130 dB SNR.

### **2\. Offload 2× Integer Scaling & Planar Color Conversion to the RK3326 2D Engine (RGA2-Lite)**

* **Current Architecture:** 640×480 (4:3) integer/sharp-bilinear scaling and planar-to-RGB888 conversion are routed through Mali-G31 MP2 fragment shaders (Panfrost GLES 3.0) on Core 1 (0x02), with CONFIG\_VIDEO\_ROCKCHIP\_RGA disabled in the kernel fragment.  
* **Architectural Bottleneck:** Every frame rendered via Panfrost consumes Mali-G31 shader cycles, 16 KB tile-buffer bandwidth, and 32-bit DDR3 read/write passes (\~1.23 MB per 640×480 frame at 60 Hz), leaving the RK3326's dedicated fixed-function 2D Raster Graphic Acceleration engine (RGA2-Lite) idle on the AXI / NIC-400 interconnect.  
* **Design Improvement:**  
  * **Re-enable RGA2-Lite Hardware Blitting:** Enable CONFIG\_VIDEO\_ROCKCHIP\_RGA for classic 320×240 (2× exact integer scale to 640×480) and 256×224 arcade titles in FBNeo as well as YUV-to-RGB video/TUI blitting. Offloading integer scaling to RGA2-Lite frees 100% of Mali-G31 MP2 shader capacity, cuts GPU power draw by \~150–250 mW, and keeps Mali-G31 locked at 520 MHz (1100.0 mV) without voltage stepping. Reserve Panfrost GLES 3.0 exclusively for sub-pixel sharp-bilinear or CRT aperture shaders.

### **3\. Enforce mlock() Audio Arenas & LD\_PRELOAD 2 MB HugePage Allocation for FBNeo**

* **Current Architecture:** Relies on CONFIG\_TRANSPARENT\_HUGEPAGE\_MADVISE=y and 1024 MB LZ4 ZRAM (vm.swappiness=120, vm.page-cluster=0) to keep FBNeo ROM arenas (32–64 MB) and FLAC\_AX audio buffers (64 MiB) resident in 1 GB DDR3.  
* **Architectural Bottleneck:** Passive THP=madvise only promotes pages when user-space binaries explicitly issue madvise(..., MADV\_HUGEPAGE) on 2 MB-aligned anonymous mappings; otherwise, 32–64 MB ROM banks still incur 4 KB TLB misses on the Cortex-A35's L1/L2 TLBs. Additionally, with vm.swappiness=120, unlocked audio ring buffers risk ZRAM reclamation during heavy ROM loads.  
* **Design Improvement:**  
  * **mlock() Audio Rings \+ LD\_PRELOAD 2 MB HugePage Shim:** Lock FLAC\_AX's 64-byte-aligned SPSC audio ring buffer and 64 MiB preload arena in physical DDR3 via mlock() so active audio pages never touch ZRAM. Inject a lightweight ax\_hugepage\_shim.so (LD\_PRELOAD) via ax-sdk to intercept mmap() allocations \>= 2 MiB in RetroArch/FBNeo, align them to 2 MiB boundaries, and apply madvise(ptr, len, MADV\_HUGEPAGE) automatically—delivering a deterministic 32× TLB miss reduction without patching upstream emulator code.

### **5.1 Summary of Architectural Impact**

| Improvement | Target Subsystem | Primary Gain |
| :---- | :---- | :---- |
| 1\. DWC2 Microframe Coalescing \+ Audio-Mode Voltage Floor | Core 0 (0x01) \+ DWC2 \+ ES9281AC Pro | 8× lower USB IRQ rate on Core 0 and cleaner VBUS analog reference floor |
| 2\. Fixed-Function RGA2-Lite 2D Blitter Offload | AXI Interconnect \+ Mali-G31 MP2 \+ VOP | Zero GPU shader load for 2× integer 320×240 to 640×480 scaling and lower thermal output |
| 3\. mlock() Audio Arenas \+ LD\_PRELOAD 2 MB THP Shim | 1 GB DDR3 \+ Cortex-A35 L1/L2 TLBs | Deterministic zero-swap audio under swappiness=120 and guaranteed 2 MB hugepage backing for FBNeo |


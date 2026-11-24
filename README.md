# Piccolo-2go (Optimised with AX Tech)

**Repository:** [galaxywarriordad/Piccolo-2go](https://github.com/galaxywarriordad/Piccolo-2go) ([https\://github.com/galaxywarriordad/Piccolo-2go.git](https://github.com/galaxywarriordad/Piccolo-2go.git))

**License:** GPL v3 \| **Docs:** Piccolo-2go \| **CI:** GitHub Actions

**Piccolo-2go** is a deterministic `aarch64` Buildroot operating system (`128 KB`\-block `zstd` SquashFS `< 140 MB`) engineered as a closed, fixed-hardware console and audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon, **optimised with AX tech**. Part of **The Silicon Preservation Society** ecosystem, the initial `v1.1` release focuses on **FinalBurn Neo (FBNeo)** arcade bring-up, `640×480` `mpv` movie playback (`/userdata/movies`), and the **FLAC\_AX** bit-perfect `S32_LE` / `DoP` media player (tuned for the primary **HiBy FC3** `£29.61`, `ES9281 PRO` / `ES9281AC Pro`, `1.3 Vrms` USB-C OTG DAC with `3.5mm` wired **Moondrop Chu**, alongside **JCALLY JM6 Pro** `£12.99`, `CX31993` \+ `MAX97220` and **HiBy Digital D1** `£11.11`, `CS46L41` backup USB-C DACs and internal **RK817** fallback), with additional emulator cores planned for later releases.

---

## Fixed-Hardware Topology & Silicon Specification (Optimised with AX Tech)

Piccolo-2go treats the **BATLEXP G350** and **R36S** as a deterministic fixed-hardware appliance rather than a general-purpose Linux PC—mapping every physical bus, interrupt line, voltage rail, and execution plane onto the **Rockchip RK3326** silicon:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                  FIXED-HARDWARE APPLIANCE TOPOLOGY — BATLEXP G350 / R36S               │
│            Rockchip RK3326 (1.296 / 1.344 / 1.392 GHz) + 1 GB LPDDR3 @ 720 MHz         │
│                                                                                        │
│  [ Core 0 (0x01) ] ──► SCHED_FIFO 80 ALSA S32_LE + DWC2 (bInterval=4) / I2S0 / GPIO    │
│  [ Core 1 (0x02) ] ──► Mali-G31 MP2 (Panfrost GLES 3.0) + RGA2-Lite 2x ──► VOP 640×480 │
│  [ Core 2 (0x04) ] ──► Deterministic FBNeo / DSP Worker (mlock + 2 MB HugePage Shim)   │
│  [ Core 3 (0x08) ] ──► SDMMC0/1 DMA IRQs + 1024 MB LZ4 ZRAM + 64 MiB FLAC Preload      │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│        AUDIO & USB-C OTG SUBSYSTEM        │        DUAL SANDISK EXTREME PRO STORAGE    │
│  • DWC2 USB-C OTG (Core 0 IRQ)            │  • TF1 (64 GB A2/V30 — OS & Save Slot)     │
│    └─► Primary HiBy FC3 (ES9281AC Pro)    │     ├── p1: BOOT (FAT32, 256 MB, DTBs)     │
│        1.3 Vrms / 70 mW | S32_LE & DoP    │     ├── p2: ROOTFS (SquashFS zstd, <140MB) │
│        + Backup JCALLY JM6 Pro / D1       │     └── p3: STORAGE (ext4, ~59 GB auto)    │
│  • I2S0 Fallback ──► RK817 PMIC Codec     │  • TF2 (128 GB A2/V30 — Media Library)     │
│    (3.5 mm Stereo Jack + 1W Mono Speaker) │     └── p1: PICCOLO_MEDIA (exFAT/ext4)     │
│  • Controls: GPIO Matrix + 10-bit SARADC  │         /roms/fbneo, /music, /bios, /movies│
└───────────────────────────────────────────┴────────────────────────────────────────────┘
```

### 1\. CPU, Memory Bus & 1:2:1 Core/IRQ Partition (4× ARM Cortex-A35 \+ 1 GB LPDDR3)

The Rockchip **RK3326** pairs four dual-issue **in-order ARM Cortex-A35** cores (`32 KB` L1-I / `32 KB` L1-D per core, `256 KB` shared L2) with a `32-bit` `1 GB` LPDDR3 bus. Tickless kernel boot flags (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2 threadirqs`) and `S01piccolo-2go-isolate` pin both hardware IRQ affinities and threaded `irq/<N>-*` handlers across all four cores:

| Core | Affinity Mask | Scheduler & Governor | Fixed Hardware Responsibility |
| :---- | :---- | :---- | :---- |
| Core 0 | `0x01` | `SCHED_FIFO` 80 / `performance` | **Real-Time Audio Clock & Controller IRQ Plane:** Direct ALSA `S32_LE` writer (`192`\-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`, `bInterval=4` microframe coalescing), `I2S0` (`RK817`), and `GPIO` / `SARADC` control IRQs. |
| Core 1 | `0x02` | `SCHED_OTHER` | **GPU Submission & Display Scanout Plane:** Drives **Mali-G31 MP2** (`Panfrost` OpenGL ES 3.0), fixed-function **RGA2-Lite** `2x` integer scaling (`CONFIG_VIDEO_ROCKCHIP_RGA=y`), and atomic `KMS/DRM` page flips (`PAN_MESA_DEBUG=noafbc`) to the `640×480 @ 60.00 Hz` panel. |
| Core 2 | `0x04` | `SCHED_OTHER` | **Deterministic Emulation & DSP Plane:** Runs `FBNeo` and DSP workers with hot `32 KB` L1 caches, `mlock()` audio rings, and `LD_PRELOAD` `2 MB` HugePage backing (`ax_hugepage_shim.so`). |
| Core 3 | `0x08` | `SCHED_OTHER` / `schedutil` | **Housekeeping & SD Storage DMA Plane:** `dwmmc` (`TF1`/`TF2` SanDisk SD) DMA IRQs, `1024 MB` `LZ4` ZRAM (`vm.swappiness=120`, `vm.page-cluster=0`), `64 MiB` `FLAC_AX` RAM preloader, `ax_appliance_tui`, and `ax-powerd`. |

**Unified Hardware Profiles (`/boot/ax-2go.conf` — `HW_PROFILE`):**

* **default (Default Mode):** `1344 MHz` (`1.344 GHz` / `1.34 GHz` @ `1112.5 mV`) CPU, `600 MHz` (`1137.5 mV` / `1137500 µV`) Mali-G31 GPU ceiling, and `720 MHz` (`1440 MT/s` @ `1112.5 mV`) LPDDR3 DMC bus.  
* **safe (Safe Mode / Pure Audio Floor):** `1296 MHz` (`1.296 GHz` / `1.30 GHz` @ `1100.0 mV`) CPU, `520 MHz` (`1100.0 mV` / `1100000 µV`) GPU, and `666 MHz` (`1333 MT/s` @ `1100.0 mV`) LPDDR3—also used during pure `FLAC_AX` DAP playback to keep VBUS switching noise minimal.  
* **oc\_boost (OC / Boost Mode):** `1392 MHz` (`1.392 GHz` / `1.39 GHz`) CPU, `620 MHz` (`1150.0–1162.5 mV` / `1150000–1162500 µV` custom OPP push) GPU, and `786+ MHz` (`800–900 MHz` open-case) LPDDR3, guarded by `ax-powerd` automatic `75°C` thermal rollback.

### 2\. GPU & LCD Display Pipeline (Mali-G31 MP2 \+ RGA2-Lite \-\> VOP \-\> 3.5" 640×480 MIPI DSI IPS)

* **Hardware-Accelerated GPU & 2D Blitter Pipeline:** Uses Mesa **Panfrost** OpenGL ES 3.0 (`BR2_PACKAGE_MESA3D_GALLIUM_DRIVER_PANFROST=y`, `PAN_MESA_DEBUG=noafbc`) paired with fixed-function **RGA2-Lite** (`CONFIG_VIDEO_ROCKCHIP_RGA=y`) for `2x` integer `320×240` \-\> `640×480` scaling and atomic `KMS/DRM` scanout (`SDL_VIDEODRIVER=kmsdrm`) onto the `640×480 @ 60.00 Hz` `4:3` OCA IPS panel—with zero X11/Wayland compositor overhead (`~35–50 MB` LPDDR3 saved).

**Mali-G31 MP2 Voltage-Scaled OPP Ladder (`12.5 mV` / `12500 µV` RK817 Steps):**

* `520 MHz` (`1100.0 mV` / `1100000 µV`): Safe baseline ceiling (`Safe` profile).  
    
* `550 MHz` (`1112.5 mV` / `1112500 µV`, `+1` step): Mild shader/fillrate step.  
    
* `580 MHz` (`1125.0 mV` / `1125000 µV`, `+2` steps): Mid shader/fillrate step.  
    
* `600 MHz` (`1137.5 mV` / `1137500 µV`, `+3` steps): Default Mode ceiling (`Default` profile).  
    
* `620 MHz` (`1150.0–1162.5 mV` / `1150000–1162500 µV`, `+4–5` steps): OC / Boost custom push (requires DTB/kernel OPP support; guarded by `75°C` thermal rollback).  
    
* **Multi-Board Device Tree Resolution (`dtb-select`):** Ships `/boot/rk3326-g350-linux.dtb` active by default plus bundled `.dtb` files in `/boot/dtbs/` (`g350`, `r36h`, `r46h`, `r36s-plus`, `r36s-panel0..4`, `r36s-panel4-t2`, `r36s-v21`, `r36s-pro`, `r36s-xx`) and CMake staging support from `lcdyk0517/arkos4clone` while preserving your custom Piccolo-2go `640×480` Espresso/Copper `logo.bmp` splash.

### 3\. Physical Controls & Safe Power Guard (GPIO Matrix \+ SARADC \+ ax-powerd)

* **Zero-Latency Input (Core 0 IRQ):** Dedicated GPIO lines handle the D-pad, quiet face buttons (`A/B/X/Y`), sloped shoulder triggers (`L1/L2`, `R1/R2`), `Start`, `Select`, `FN` (`BTN_TRIGGER_HAPPY5` / `BTN_MODE`), and `KEY_POWER`, while the `10-bit` **SARADC** samples dual analog thumbsticks.  
* **Core 3 Hardware Protection Daemon (`ax-powerd`):** Intercepts `1.5s` `KEY_POWER` hold (or `FN + Power`) to release the HiBy FC3 DAC (`SIGUSR1`), flush saves, unmount both SanDisk cards, and power off safely—while enforcing `75°C` thermal clock rollback (`1296 MHz`) and `3280 mV` LiPo brownout shutdown.

### 4\. Bit-Perfect Audio DAC Subsystem (HiBy FC3 Primary \+ USB-C Backups \+ RK817 Fallback)

* **Primary USB-C OTG DAC & Recommended Earphones — HiBy FC3 (`£29.61`, `ES9281 PRO` / `ES9281AC Pro`, `1.3 Vrms` / `70 mW` @ `32 Ω`, `fc3_1v3`, VID `0x32BB`) \+ `3.5mm` Moondrop Chu:**  
  * Hot-plugged via `99-ax-2go-otg.rules` and `/usr/sbin/ax-audio-route`, locking `power/control="on"` (zero USB autosuspend clicks) and bypassing PulseAudio/PipeWire/software resampling (`snd_pcm_hw_params_set_rate_resample(0)`).  
  * Streams bit-perfect **32-bit** integer **S32\_LE (`44.1–384 kHz`)** and hardware **DoP (`DSD64` `176.4 kHz` / `DSD128` `352.8 kHz`)** with `fc3_1v3` hardware attenuator presets (`--volume 0.95` portable speakers, `0.65` wired headphones, `0.35` sensitive `16–32 Ω` IEMs) and `mlock()`\-pinned audio arenas.  
* **Backup USB-C DACs:** Supports **JCALLY JM6 Pro** (`£12.99`, `CX31993` \+ `MAX97220`) and **HiBy Digital D1** (`£11.11`, `CS46L41`) via `ax-audio-route` (`usb_backup` profile).  
* **Interactive Connect & Retry (`ax-audio-route prompt`) & RK817 Fallback:** If a USB-C OTG DAC is unplugged when launching audio, Piccolo-2go prompts to plug in the DAC and retry (`[R]`) or fall back cleanly (`[S]`) to the internal **RK817** codec (`3.5 mm` jack \+ `1W` mono speaker) with pop-free path switching.

### 5\. Dual SanDisk Extreme PRO Storage (64 GB TF1 \+ 128 GB TF2), SD Profiles & 1024 MB LZ4 ZRAM

* **TF1 — OS & Save Slot (`64 GB` SanDisk Extreme PRO A2 / V30):**  
  * `p1` (`FAT32`, `256 MB`): `LZ4`\-compressed pruned `linux-rk3326.fragment` kernel `Image` (`< 180 ms` boot decompression, `~110 MB` LPDDR3 reclaimed), `boot.ini`, `ax-2go.conf`, `640×480` Espresso/Copper U-Boot splash `logo.bmp`, and `/boot/dtbs/`.  
  * `p2` (`SquashFS zstd`, `128 KB` blocks, `< 140 MB`): Immutable, read-only root filesystem immune to sudden power loss.  
  * `p3` (`ext4`, `~59 GB`): Auto-expanded on first boot (`S02piccolo-2go-storage`, `noatime,nodiratime,commit=60,data=writeback`) for saves and configs.  
* **TF2 — Portable Media Slot (`128 GB` SanDisk Extreme PRO A2 / V30):**  
  * `p1` (`exFAT` / `ext4`, `~119 GB`): Dedicated media card automatically bound to `/userdata/{roms,music,bios,movies}` on boot for `FBNeo` arcade/Neo Geo ROMs, lossless `.flac` / `DoP` albums, `.mod`/`.xm`/`.s3m`/`.it` tracker libraries, and `640×480` movies.

**SD Card Profiles (`SD_PROFILE` in `/boot/ax-2go.conf`) & SDR50 Guard:**

* **sdr50\_pro (Default):** Verifies `50 MHz` SDR50 (`3.3V` signaling, `~24 MB/s`) on `mmc0` (`TF1`, `256 KB` read-ahead) and `mmc1` (`TF2`, `512 KB` read-ahead), stripping unstable SDR104.  
* **hs\_compat:** Locks `25 MHz` High-Speed compatibility (`128 KB` read-ahead) for older or CRC-sensitive cards.  
* **audio\_quiet:** Conservative `128 KB` read-ahead paired with `64 MiB` `FLAC_AX` RAM preload on `Core 3` for quiet DAP playback.

---

## Repository Structure (`galaxywarriordad/Piccolo-2go`)

```
Piccolo-2go/
├── .github/workflows/
│   ├── ci.yml                                       # Automated ShellCheck, C tool & Buildroot image workflow
│   └── cmake-verify.yml                             # Automated CMake DTB/profile staging verification
├── board/piccolo-2go/
│   ├── ax-2go.conf                                  # User hardware overrides (HW/CPU/GPU/DDR/SD profiles & LEDs)
│   ├── boot.ini                                     # U-Boot bootargs, Piccolo-2go logo.bmp & 1344/600/720 MHz clocks
│   ├── dtbs/                                        # Bundled DTBs (rk3326-g350-linux.dtb default + R36S/H variants)
│   ├── genimage.cfg                                 # Dual-partition MBR sdcard.img layout (BOOT + ROOTFS)
│   ├── linux-rk3326.fragment                        # Pruned RK3326 kernel config (LZ4, NO_HZ_FULL, RGA2, 64 MB CMA)
│   ├── logo.bmp                                     # 640x480 24-bit Espresso/Copper Piccolo-2go U-Boot splash
│   ├── post-build.sh                                # Permissions, boot.ini/ax-2go.conf/logo.bmp/DTB staging & lvp purge
│   └── rootfs-overlay/
│       ├── etc/
│       │   ├── init.d/
│       │   │   ├── S01piccolo-2go-isolate           # 1:2:1 core/IRQ pinning, 1024 MB LZ4 ZRAM & CPU/GPU/DDR profiles
│       │   │   ├── S02piccolo-2go-storage           # Dual SanDisk mounter, SD profiles & SDR50 bus guard
│       │   │   └── S03piccolo-2go-powerd            # Core 3 ax-powerd init service (start/stop/restart/reload)
│       │   ├── mpv/
│       │   │   └── mpv.conf                         # 640x480 DRM/GLES + ALSA playback configuration
│       │   ├── udev/rules.d/
│       │   │   └── 99-ax-2go-otg.rules              # DAC & MIDI zero-autosuspend rules
│       │   └── retroarch-2go.cfg                    # 640x480 integer-scaled KMS/Panfrost FBNeo configuration
│       └── usr/
│           ├── bin/
│           │   └── ax-movie                         # 640x480 mpv launcher wired to /run/ax/audio.env
│           └── sbin/
│               └── ax-audio-route                   # DAC detector with interactive Connect & Retry prompt
├── configs/
│   └── piccolo_2go_rk3326_defconfig                 # Buildroot aarch64 defconfig (FLAC_AX + FBNeo + Panfrost + mpv)
├── docs/
│   ├── 2go_INSTALLGUIDE.md                          # Step-by-step dual SanDisk setup, SD profiles & DTB calibration
│   ├── 2go_USERGUIDE.md                             # Complete hardware manual, audio & 1:2:1 engineering guide
│   ├── ARCHITECTURE.md                              # System architecture, chip topology & v1.1 mixed-signal spec
│   ├── BUILDING.md                                  # Buildroot compilation, CMake staging & flashing reference
│   └── technical_engineering.md                     # Deep-dive silicon, throughput, build audit & Mali-G31 analysis
├── package/piccolo-2go-tools/
│   ├── piccolo-2go-tools.mk                         # Buildroot target build recipe
│   └── src/
│       ├── ax-boot-recovery.c                       # Hold-button safe recovery (SELECT / FN / B -> G350 safe defaults)
│       ├── ax-powerd.c                              # Core 3 safe KEY_POWER, 75C rollback & 3280 mV guard
│       └── dtb-select.c                             # Runtime DTB panel selector (G350 & all R36S variants)
├── CMakeLists.txt                                   # Optional CMake DTB & profile staging helper (Piccolo-2go splash)
├── Config.in                                        # Root BR2_EXTERNAL Kconfig menu
├── LICENSE                                          # GNU General Public License v3.0
└── README.md                                        # Project overview & fixed-hardware topology
```

### Documentation Links in This Repository

* [Installation & First-Boot Setup Guide (`docs/2go_INSTALLGUIDE.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/2go_INSTALLGUIDE.md)  
* [Complete User Guide (`docs/2go_USERGUIDE.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/2go_USERGUIDE.md)  
* [System Architecture & Fixed-Hardware Topology (`docs/ARCHITECTURE.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/ARCHITECTURE.md)  
* [Building from Source (`docs/BUILDING.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/BUILDING.md)  
* [Technical & Mixed-Signal Engineering Specification (`docs/technical_engineering.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/technical_engineering.md)

---

## Silicon Preservation Society Ecosystem

### All 9 Repositories

* **Bare-Metal Audio & A/V Engine (GitHub):** [galaxywarriordad/FLAC\_AX](https://github.com/galaxywarriordad/FLAC_AX.git) — RAM-resident bit-perfect audio player, `ax_av_studio` MIDI/DSP synth workstation, and `ax_video_upscaler` stream pipeline.  
* **Linux USB Console & A/V Appliance (GitHub):** [galaxywarriordad/Piccolo-AX](https://github.com/galaxywarriordad/Piccolo-AX.git) — Standalone `< 512 MB` `toram` `EROFS` Batocera / Buildroot Linux `6.12-rt` `x86_64` appliance (`batocera-ax-external`) with direct ALSA `SCHED_FIFO` and `iHD` VA-API `dma_buf`.  
* **Optional Desktop Tier (GitHub):** [galaxywarriordad/piccolo-ax-desktop](https://github.com/galaxywarriordad/piccolo-ax-desktop.git) — On-demand C/C++ `LXQt`/`X11` desktop workbench (`< 42 MB` budget).  
* **Linux Handheld Appliance (GitHub):** [galaxywarriordad/Piccolo-2go](https://github.com/galaxywarriordad/Piccolo-2go.git) — Deterministic Buildroot `aarch64` (`RK3326` `1:2:1`) retro handheld OS for **BATLEXP G350** and **R36S**.  
* **Shared Silicon SDK (GitHub):** [galaxywarriordad/ax-sdk](https://github.com/galaxywarriordad/ax-sdk.git) — Shared dependency-free C++20 deterministic hardware runtime (`ax::sdk` / `v1_36_2`), dynamic Intel SMT / AMD CCX topology resolver, platform shims, and master architecture specs.  
* **Retro Multi-System Engine (GitHub):** [galaxywarriordad/Generation-X-AX](https://github.com/galaxywarriordad/Generation-X-AX.git) — Deterministic multi-console `[AX]` integration across `Flycast` and `RMG`.  
* **Main Switch Engine Fork (Forgejo):** [galaxywarriordad/Eden-Espresso-Wave-AX](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) — Deterministic Nintendo Switch fork tuned for `ax::sdk` core topology, Vulkan telemetry, and low-latency audio, kept aligned with upstream for frictionless PR cherry-picks.  
* **Wii U Engine (GitHub):** [galaxywarriordad/Cemu-Latte-AX](https://github.com/galaxywarriordad/Cemu-Latte-AX.git) — Deterministic Nintendo Wii U (`Latte` GPU) AX engine powered by `ax::sdk`.  
* **Commodore Amiga Engine (GitHub):** [galaxywarriordad/Amiberry-AX](https://github.com/galaxywarriordad/Amiberry-AX.git) — Deterministic Commodore Amiga `500` / `1200` / `CD32` engine linked against `ax-sdk`.

### Upstream Reference

* **Upstream Ecosystem Hub (Forgejo):** [eden-emu/eden](https://git.eden-emu.dev/eden-emu/eden/) — Upstream Eden Switch emulator reference repository.

---

## Included Hotkey Cheatsheet (Zero Manual Setup Needed)

* **FN** (single press) or **FN \+ FN**: Toggle RetroArch `RGUI` menu  
* **FN \+ START**: Cleanly exit `FBNeo` back to `ax_appliance_tui` (and restore `gpu-auto` power saving)  
* **FN \+ R1 / FN \+ L1**: Quick Save State / Quick Load State (`TF1` `p3` `/userdata/saves`)  
* **FN \+ Left / FN \+ Right**: Decrease / Increase Save State Slot  
* **FN \+ B**: Reset current arcade/console board

---

## Supported Hardware

Piccolo-2go is developed, tuned, and primarily validated on the **BATLEXP G350** (`rk3326-g350-linux.dtb`, active by default).

**Supported DAC Subsystem (Piccolo-2go v1.1):**

* **Primary USB-C DAC:** HiBy FC3 (`£29.61`, `ES9281 PRO` / `ES9281AC Pro`, `1.3 Vrms` `fc3_1v3` profile) paired with `3.5mm` wired Moondrop Chu.  
* **Backup USB-C DACs:** JCALLY JM6 Pro (`£12.99`, `CX31993` \+ `MAX97220`) and HiBy Digital D1 (`£11.11`, `CS46L41`).  
* **Onboard Fallback:** Internal Rockchip `RK817` codec (`3.5 mm` jack \+ `1W` mono speaker) when no USB-C DAC is attached.

To support the broader Rockchip RK3326 handheld family, the following **community-sourced Device Trees (`.dtb`)** are bundled in `/boot/dtbs/` and selectable via `dtb-select <alias>` (or by copying the matching file to `/boot/rk3326-g350-linux.dtb`). **These additional DTBs are currently untested on Piccolo-2go—please open an Issue in this repository if you encounter panel, audio, or control mapping quirks on your revision:**

> **Hold-Button Safe Recovery:** If you switch to an incompatible `.dtb` and get a black screen, hold **SELECT**, **FN**, or **B** while powering on—`ax-boot-recovery` will automatically restore `rk3326-g350-linux.dtb`, `1.296 GHz` (`1.30 GHz`) safe CPU clocks, `520 MHz` GPU, and `666 MHz` DDR3 backup timings.

| `dtb-select` Alias | Bundled `.dtb` Filename | Target Model & How to Identify Your Revision | Status |
| :---- | :---- | :---- | :---- |
| `g350` | `rk3326-g350-linux.dtb` | **BATLEXP G350** — Vertical `3.5"` `640×480` OCA IPS, matte shell, sloped shoulder buttons, single USB-C OTG \+ USB-C charge port | **Primary / Validated** |
| `r36s-panel4` / `r36s-panel4-t1` | `rk3326-r36s-panel4-linux.dtb` | **R36S (Panel 4 Type 1\)** — Common modern R36S vertical handheld with updated `3.5"` `640×480` MIPI DSI panel | *Untested (Community DTB)* |
| `r36s-panel4-t2` | `rk3326-r36s-panel4-type2-linux.dtb` | **R36S (Panel 4 Type 2\)** — Alternate Panel 4 MIPI DSI timing variant | *Untested (Community DTB)* |
| `r36s-v21` | `rk3326-r36s-v21-linux.dtb` | **R36S (V21 / V22 Motherboard)** — Newer R36S PCB revision (`V21`/`V22` silkscreened near battery bay) with revised OTG/rumble GPIO routing | *Untested (Community DTB)* |
| `r36s-panel0` | `rk3326-r36s-panel0-linux.dtb` | **R36S (Original / Panel 0\)** — First-generation 2023 R36S vertical unit with original `640×480` IPS panel timings | *Untested (Community DTB)* |
| `r36s-panel1` | `rk3326-r36s-panel1-linux.dtb` | **R36S (Panel 1\)** — Early second-batch R36S vertical revision (Panel 1 screen timing variant) | *Untested (Community DTB)* |
| `r36s-panel2` | `rk3326-r36s-panel2-linux.dtb` | **R36S (Panel 2\)** — R36S vertical revision (Panel 2 screen timing variant) | *Untested (Community DTB)* |
| `r36s-panel3` | `rk3326-r36s-panel3-linux.dtb` | **R36S (Panel 3\)** — R36S vertical revision (Panel 3 screen timing variant) | *Untested (Community DTB)* |
| `r36s-plus` | `rk3326-r36splus-linux.dtb` | **R36S Plus (1:1 Square Screen)** — Widescreen/square `720×720` `1:1` IPS R36S variant | *Untested (Community DTB)* |
| `r36s-pro` | `rk3326-r36pro-linux.dtb` | **R36 Pro** — RK3326 R36 Pro variant | *Untested (Community DTB)* |
| `r36s-xx` | `rk3326-r36xx-linux.dtb` | **R36XX** — RK3326 R36XX variant | *Untested (Community DTB)* |
| `r36h` | `rk3326-r36h-linux.dtb` | **GameConsole R36H** — Horizontal candybar-form-factor RK3326 handheld (`3.5"` `640×480` IPS, twin thumbsticks below D-pad/buttons) | *Untested (Community DTB)* |
| `r46h` | `rk3326-r46h-linux.dtb` | **GameConsole R46H** — Larger horizontal RK3326 handheld (`4.0"` `720×720` / `640×480` IPS class) | *Untested (Community DTB)* |

> **Analog Thumbstick MUX Note (G350 vs. R36S/R36H):** The BATLEXP G350 and R36S/R36H wire their 4-channel SARADC analog multiplexers (`amux-channel-mapping`) and `invert-absx` / `invert-absrx` axis polarities differently. Booting with the matching `.dtb` for your board automatically applies the correct hardware axis mapping in `odroidgo3-joypad`.

---
## Ecosystem Architecture & Topology

This repository is part of **The Silicon Preservation Society** (`[AX]`) ecosystem—a nine-repository bare-metal hardware preservation stack built around **Policy-in-SDK / Adapter-in-Consumer** separation:

* **Shared Runtime Core:** [`ax-sdk`](https://github.com/galaxywarriordad/ax-sdk) — C++20 `2:3:1` / `1:2:1` core topology, `EngineConfig`, HugeTLB fastmem, and VRAM guardrails.  
* **Bare-Metal OS & Workbench:** [`Piccolo-AX`](https://github.com/galaxywarriordad/Piccolo-AX) (`x86_64` `6.12-rt` appliance), [`Piccolo-2go`](https://github.com/galaxywarriordad/Piccolo-2go) (`ARM64` `RK3326` handheld OS), [`piccolo-ax-desktop`](https://github.com/galaxywarriordad/piccolo-ax-desktop) (isolated X11 workbench).  
* **Media & Emulation Consumers:** [`FLAC_AX`](https://github.com/galaxywarriordad/FLAC_AX), [`Eden-Espresso-Wave-AX`](https://github.com/galaxywarriordad/Eden-Espresso-Wave-AX), [`Cemu-Latte-AX`](https://github.com/galaxywarriordad/Cemu-Latte-AX), [`Amiberry-AX`](https://github.com/galaxywarriordad/Amiberry-AX), and [`Generation-X-AX`](https://github.com/galaxywarriordad/Generation-X-AX).

For full core-pinning tables (`2:3:1` and `1:2:1`), cross-repository data flow, and architectural specifications, see the master guide in [**`ax-sdk/docs/ECOSYSTEM_ARCHITECTURE.md`](https://github.com/galaxywarriordad/ax-sdk/blob/main/docs/ECOSYSTEM_ARCHITECTURE.md)**.
---

## License

Released under the [GNU General Public License v3.0](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/LICENSE).

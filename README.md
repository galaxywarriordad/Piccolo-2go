# Piccolo-2go (Optimised with AX Tech)
[![Build & Lint Status](https://shields.io)](https://github.com)
[![Architecture](https://shields.io)](https://github.com)
[![Display](https://shields.io)](#2-gpu--lcd-display-pipeline-mali-g31-mp2--vop--35-640480-mipi-dsi-ips)
[![Audio](https://shields.io)](#4-bit-perfect-audio-dac-subsystem-early-hiby-fc3-13-vrms--rk817-fallback)
[![License](https://shields.io)](LICENSE)

**Piccolo-2go** is a deterministic `aarch64` Buildroot operating system (`128 KB`-block `zstd` SquashFS `< 140 MB`) engineered as a closed, fixed-hardware console and audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon, **optimised with AX tech**. Part of **The Silicon Preservation Society** ecosystem, it combines `1:2:1` Cortex-A35 core isolation, `1024 MB` `LZ4` ZRAM, zero-compositor Mali-G31 MP2 KMS/DRM + Panfrost GLES 3.0 rendering, dual SanDisk Extreme PRO (`64 GB` + `128 GB`) storage with runtime `SDR50` bus verification, **FinalBurn Neo (`FBNeo`)** arcade bring-up, and bit-perfect `FLAC_AX` `S32_LE` / `DoP` audio tuned for the early **HiBy FC3 (`1.3 Vrms`)** USB-C OTG DAC.

---

## ⚙️ Fixed-Hardware Topology & Silicon Specification (Optimised with AX Tech)

`Piccolo-2go` treats the **BATLEXP G350** and **R36S** as a deterministic fixed-hardware appliance rather than a general-purpose Linux PC—mapping every physical bus, interrupt line, voltage rail, and execution plane onto the **Rockchip RK3326** silicon:

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                  FIXED-HARDWARE APPLIANCE TOPOLOGY — BATLEXP G350 / R36S               │
│                  Rockchip RK3326 (1.30 / 1.34 / 1.39 GHz) + 1 GB LPDDR3 @ 720 MHz      │
│                                                                                        │
│  [ Core 0 (0x01) ] ──► SCHED_FIFO 80 ALSA S32_LE + DWC2 OTG / I2S0 / GPIO+SARADC IRQs │
│  [ Core 1 (0x02) ] ──► Mali-G31 MP2 (Panfrost GLES 3.0) ──► VOP ──► 640×480 DSI LCD    │
│  [ Core 2 (0x04) ] ──► Deterministic FBNeo / JIT & DSP Worker (32 KB L1 Cache Hot)     │
│  [ Core 3 (0x08) ] ──► SDMMC0/1 DMA IRQs + 1024 MB LZ4 ZRAM + 64 MiB FLAC Preload      │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│        AUDIO & USB-C OTG SUBSYSTEM        │        DUAL SANDISK EXTREME PRO STORAGE    │
│  • DWC2 USB-C OTG (Core 0 IRQ)            │  • TF1 (64 GB A2/V30 — OS & Save Slot)     │
│    └─► Early HiBy FC3 (ES9281AC Pro)      │     ├── p1: BOOT (FAT32, 256 MB, DTBs)     │
│        1.3 Vrms / 70 mW | S32_LE & DoP    │     ├── p2: ROOTFS (SquashFS zstd, <140MB) │
│        + Interactive Connect & Retry      │     └── p3: STORAGE (ext4, ~59 GB auto)    │
│  • I2S0 Fallback ──► RK817 PMIC Codec     │  • TF2 (128 GB A2/V30 — Media Library)     │
│    (3.5 mm Stereo Jack + 1W Mono Speaker) │     └── p1: PICCOLO_MEDIA (exFAT/ext4)     │
│  • Controls: GPIO Matrix + 10-bit SARADC  │         /roms, /music (FLAC/MOD), /bios    │
└───────────────────────────────────────────┴────────────────────────────────────────────┘
```

### 1. CPU, Memory Bus & `1:2:1` Core/IRQ Partition (`4× ARM Cortex-A35` + `1 GB LPDDR3`)

The Rockchip **RK3326** pairs four dual-issue **in-order ARM Cortex-A35** cores (`32 KB` L1-I / `32 KB` L1-D per core, `256 KB` shared L2) with a `32-bit` `1 GB LPDDR3` bus. Tickless kernel boot flags (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate` lock each core to a single hardware domain:

| Core | Affinity Mask | Scheduler & Governor | Fixed Hardware Responsibility |
| :--- | :--- | :--- | :--- |
| **Core 0** | `0x01` | `SCHED_FIFO` (prio `80`) / `performance` | **Real-Time Audio Clock & Controller IRQ Plane:** Direct ALSA `S32_LE` writer (`192`-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`), `I2S0` (`RK817`), and `GPIO` / `SARADC` control IRQs |
| **Core 1** | `0x02` | `SCHED_OTHER` / `performance` | **GPU Submission & Display Scanout Plane:** Drives **Mali-G31 MP2** (`Panfrost` OpenGL ES 3.0) draw calls and atomic `KMS/DRM` page flips to the `640×480` panel |
| **Core 2** | `0x04` | `SCHED_OTHER` / `performance` | **Deterministic Emulation & DSP Plane:** Runs `FBNeo` / JIT / DSP workers with hot `32 KB` L1 caches and zero cache eviction |
| **Core 3** | `0x08` | `SCHED_OTHER` (`nice +5`) / `schedutil` | **Housekeeping & SD Storage DMA Plane:** `dwmmc` (`TF1`/`TF2` SanDisk SD) IRQs, `1024 MB` `LZ4` ZRAM (`vm.swappiness=120`, `vm.page-cluster=0`), `64 MiB` `FLAC_AX` RAM preloader, `ax_appliance_tui`, and `ax-powerd` |

- **Safe RK3326 CPU PLL Steps (`24 MHz` Increments, Passive-Cooled Safe):**
  - `1296000 kHz` (`1.296 GHz` / `1.30 GHz` — Default): Official stock safe ceiling for uncooled BATLEXP G350 / R36S shells (`~42–52°C`).
  - `1344000 kHz` (`1.344 GHz` / `1.34 GHz` — Sweet-Spot `+48 MHz`): Mild PLL step (`~52–58°C`, set via `CPU_PROFILE=1.34` in `/boot/ax-cpu.conf` or `S01piccolo-2go-isolate set-1.34`).
  - `1392000 kHz` (`1.392 GHz` / `1.39 GHz` — High `+96 MHz`): Maximum safe ceiling just under the `1.4 GHz` thermal wall (`~58–65°C`, guarded by `ax-powerd` automatic `75°C` rollback).

- **LPDDR3 / DMC Bus Profiles (`RK817 DCDC2` — `VDD_LOGIC`, `12.5 mV` / `12500 µV` Steps):**
  - `720 MHz` (`1440 MT/s` @ `1112.5 mV` / `1112500 µV` — Default): `max_ddrfreq=720` (`+8.1%` memory bandwidth over stock `666 MHz` with a single `+12.5 mV` step).
  - `666 MHz` (`1333 MT/s` @ `1100.0 mV` / `1100000 µV` — Safe Backup): Universal fallback profile (`max_ddrfreq=666`) compatible with every clone RAM bin.
  - `800–900 MHz` (`1600–1800 MT/s` @ `1137.5–1162.5 mV` — Open-Case Only): High-bandwidth enthusiast profile reserved for open-case / heatsinked boards.

### 2. GPU & LCD Display Pipeline (`Mali-G31 MP2` → `VOP` → `3.5" 640×480 MIPI DSI IPS`)

- **Hardware-Accelerated GPU Rendering (`ARM Mali-G31 MP2`):** Uses Mesa **Panfrost** OpenGL ES 3.0 (`BR2_PACKAGE_MESA3D_GALLIUM_DRIVER_PANFROST=y`) for hardware texture uploads, `2×` integer/sharp-bilinear scaling shaders, and atomic `KMS/DRM` scanout (`SDL_VIDEODRIVER=kmsdrm`) onto the `640×480 @ 60.00 Hz` `4:3` OCA IPS panel—with zero X11/Wayland compositor overhead (`~35–50 MB` LPDDR3 saved).
- **Mali-G31 MP2 Voltage-Scaled OPP Ladder (`12.5 mV` / `12500 µV` RK817 Steps):**
  - `520 MHz` (`1100.0 mV` / `1100000 µV`): Default stock ceiling (`max_gpufreq=520`).
  - `550 MHz` (`1112.5 mV` / `1112500 µV`, `+1` step): Sweet-spot mild boost (`+5.8%` shader/fillrate, minimal heat).
  - `600 MHz` (`1137.5 mV` / `1137500 µV`, `+3` steps): High GPU boost (`+15.4%` shader/fillrate).
  - `650–660 MHz` (`1162.5–1175.0 mV` / `1162500–1175000 µV`, `+5–6` steps): Max PLL ceiling (`+26.9%`, silicon-lottery dependent; guarded by `75°C` thermal rollback).
- **Multi-Board Device Tree Resolution (`dtb-select`):** Ships instant aliases (`g350`, `r36s-panel0..4`, `r36s-v21`, `r36s-plus`, `r36s-pro`, `r36s-xx`, `r36s-t1-p1..5`, `r36s-t2-p1..2`) to resolve MIPI DSI panel timings across BATLEXP G350 and R36S revisions.

### 3. Physical Controls & Safe Power Guard (`GPIO Matrix` + `SARADC` + `ax-powerd`)

- **Zero-Latency Input (`Core 0` IRQ):** Dedicated GPIO lines handle the D-pad, quiet face buttons (`A/B/X/Y`), sloped shoulder triggers (`L1/L2`, `R1/R2`), `Start`, `Select`, `FN` (`BTN_TRIGGER_HAPPY5` / `BTN_MODE`), and `KEY_POWER`, while the 10-bit **SARADC** samples dual analog thumbsticks.
- **Core 3 Hardware Protection Daemon (`ax-powerd`):** Intercepts `1.5s` `KEY_POWER` hold (or `FN + Power`) to release the HiBy FC3 DAC (`SIGUSR1`), flush saves, unmount both SanDisk cards, and power off safely—while enforcing `75°C` thermal clock rollback and `3280 mV` LiPo brownout shutdown.

### 4. Bit-Perfect Audio DAC Subsystem (`Early HiBy FC3 1.3 Vrms` + `RK817` Fallback)

- **Primary USB-C OTG DAC — Early HiBy FC3 (`ESS ES9281AC Pro`, `1.3 Vrms / 70 mW @ 32 Ω`, VID `0x32BB`):**
  - Hot-plugged via `99-ax-2go-otg.rules` and `/usr/sbin/ax-audio-route`, locking `power/control="on"` (zero USB autosuspend clicks) and bypassing PulseAudio/PipeWire/software resampling (`snd_pcm_hw_params_set_rate_resample(0)`).
  - Streams bit-perfect `32-bit` integer **`S32_LE` (`44.1–384 kHz`)** and hardware **`DoP` (`DSD64` `176.4 kHz` / `DSD128` `352.8 kHz`)** with `fc3_1v3` hardware attenuator presets (`--volume 0.95` portable speakers, `0.65` wired headphones, `0.35` sensitive `16–32 Ω` IEMs).
- **Interactive Connect & Retry (`ax-audio-route prompt`):** If the HiBy FC3 is unplugged when launching audio, `Piccolo-2go` prompts you to plug in the USB-C OTG DAC and retry (`[R]`) or fall back cleanly (`[S]`) to the internal **RK817** codec (`3.5 mm` jack + mono speaker) with pop-free path switching.

### 5. Dual SanDisk Extreme PRO Storage (`64 GB TF1` + `128 GB TF2`) & `1024 MB LZ4` ZRAM

- **TF1 — OS & Save Slot (`64 GB` SanDisk Extreme PRO `A2` / `V30`):**
  - `p1` (`FAT32`, `256 MB`): `LZ4`-compressed pruned `linux-rk3326.fragment` kernel `Image` (`< 180 ms` boot decompression, `~110 MB` LPDDR3 reclaimed), `boot.ini`, and `/boot/dtbs/`.
  - `p2` (`SquashFS zstd`, `128 KB` blocks, `< 140 MB`): Immutable, read-only root filesystem immune to sudden power loss.
  - `p3` (`ext4`, `~59 GB`): Auto-expanded on first boot (`S02piccolo-2go-storage`, `noatime,nodiratime,commit=60,data=writeback`) for saves and configs.
- **TF2 — Portable Media Slot (`128 GB` SanDisk Extreme PRO `A2` / `V30`):**
  - `p1` (`exFAT` / `ext4`, `~119 GB`): Dedicated media card automatically bound to `/userdata/{roms,music,bios}` on boot for FBNeo arcade/Neo Geo ROMs, lossless `.flac` / `DoP` albums, and `.mod`/`.xm`/`.s3m`/`.it` tracker libraries.
- **Automatic SDR50 Bus Verification & `1024 MB LZ4` ZRAM:**
  - `S02piccolo-2go-storage` probes `mmc0` (`TF1`) and `mmc1` (`TF2`) at boot to verify `SDR50` (`50 MHz`, `3.3V` signaling, `~24 MB/s`)—automatically stripping unstable `SDR104` on fixed-`3.3V` `VQMMC` boards or flashing a console warning and locking safe `25 MHz` High-Speed mode if CRC tuning fails.
  - Configures `1024 MB` `lz4` ZRAM (`~650 MB/s` in-order Cortex-A35 decompression, `vm.swappiness=120`, `vm.page-cluster=0`, `vm.watermark_scale_factor=125`) alongside bounded `/tmp` (`32 MB`) and `/run` (`16 MB`) tmpfs.

---

## 📁 Repository Structure

```text
Piccolo-2go/
├── .github/workflows/
│   └── ci.yml                                       # Automated ShellCheck & aarch64 C tool validation
├── board/piccolo-2go/
│   ├── boot.ini                                     # U-Boot bootargs (1296 MHz CPU, 520 MHz GPU, 720 MHz DDR3)
│   ├── genimage.cfg                                 # Dual-partition MBR sdcard.img layout (BOOT + ROOTFS)
│   ├── linux-rk3326.fragment                        # Pruned RK3326 kernel config (LZ4, NO_HZ_FULL, 64 MB CMA)
│   ├── post-build.sh                                # Permissions, boot.ini staging & lvp Vulkan ICD purge
│   ├── post-image.sh                                # Final sdcard.img assembly via genimage
│   └── rootfs-overlay/
│       ├── etc/init.d/
│       │   ├── S01piccolo-2go-isolate               # 1:2:1 core/IRQ pinning, 1024 MB LZ4 ZRAM & 1.30-1.39 GHz PLL
│       │   └── S02piccolo-2go-storage               # Dual SanDisk (64 GB + 128 GB) mounter & SDR50 bus guard
│       ├── etc/udev/rules.d/
│       │   └── 99-ax-2go-otg.rules                  # Early HiBy FC3 1.3 Vrms & MIDI zero-autosuspend rules
│       └── usr/sbin/
│           └── ax-audio-route                       # HiBy FC3 detector with interactive Connect & Retry prompt
├── configs/
│   └── piccolo_2go_rk3326_defconfig                 # Buildroot aarch64 defconfig (FLAC_AX + FBNeo + Panfrost)
├── docs/
│   ├── BUILDING.md                                  # Buildroot compilation & flashing reference
│   ├── Piccolo_2go_Complete_User_Guide.md           # Full hardware manual, audio & 1:2:1 engineering guide
│   └── Piccolo_2go_Installation_and_First_Boot.md   # Step-by-step dual SanDisk setup & DTB calibration
├── package/piccolo-2go-tools/
│   ├── Config.in                                    # Kconfig definition for native C utilities
│   ├── piccolo-2go-tools.mk                         # Buildroot target build recipe
│   └── src/
│       ├── dtb-select.c                             # Runtime DTB panel selector (G350 & all R36S variants)
│       └── ax-powerd.c                              # Core 3 safe KEY_POWER, 75C rollback & 3280 mV guard
├── Config.in                                        # Root BR2_EXTERNAL Kconfig menu
├── external.desc                                    # Buildroot external tree descriptor (PICCOLO_2GO)
├── external.mk                                      # Buildroot external package include hook
├── LICENSE                                          # GNU General Public License v3.0
└── README.md                                        # Project overview & fixed-hardware topology
```

---

## 🌐 Silicon Preservation Society Ecosystem

- **ax-sdk** — Shared dependency-free C++20 deterministic hardware runtime (`v1_36_2`).
- **Piccolo-AX** — Deterministic `x86_64` Buildroot / Linux `6.12-rt` bare-metal console & A/V appliance (`batocera-ax-external`).
- **Piccolo-2go** — Deterministic `aarch64` (`RK3326` `1:2:1`) fixed-hardware handheld OS for BATLEXP G350 & R36S.
- **piccolo-ax-desktop** — Optional on-demand C/C++ LXQt/X11 desktop workbench (`< 42 MB` budget).
- **FLAC_AX** — Bit-perfect `S32_LE` / `DoP` audiophile transport, `libopenmpt` tracker jukebox, A/V studio, upscaler & Espresso TUI.
- **Amiberry-AX** — Deterministic Commodore Amiga 500 / 1200 / CD32 engine linked against `ax-sdk`.
- **Generation-X-AX** — Cycle-accurate Sega Mega Drive / Genesis (BlastEm) & Atari ST (Hatari) engines.
- **Cemu-Latte-AX** — Deterministic Wii U execution engine (`2:3:1` topology).

---

## 📄 License

Released under the GNU General Public License v3.0.

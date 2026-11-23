# ☕📖 Piccolo-2go — Complete User Guide & Hardware Manual (BATLEXP G350 & R36S)

* **Target Hardware:** BATLEXP G350 & R36S Handheld Consoles (`dtb-select` runtime panel selector)
* **SoC & PMIC:** Rockchip RK3326 (`4× ARM Cortex-A35`, `ARM Mali-G31 MP2`, `1 GB LPDDR3`) + Rockchip `RK817`
* **Firmware:** `Piccolo-2go` (`aarch64` Buildroot `BR2_EXTERNAL`, `128 KB`-block `zstd` SquashFS `< 140 MB`)
* **Ecosystem:** **The Silicon Preservation Society** (Optimised with AX Tech)

**Piccolo-2go** is a deterministic 64-bit (`aarch64`) Buildroot operating system (`128 KB`-block `zstd` SquashFS `< 140 MB`) engineered as a closed, fixed-hardware retro console and bit-perfect audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon. Designed as the portable handheld sibling within **The Silicon Preservation Society** ecosystem, it eliminates desktop bloat, X11/Wayland compositors, background daemons, and user-space audio mixers (`PulseAudio`/`PipeWire`) to turn a pocket handheld into a cycle-accurate retro arcade/computer system and bit-perfect `S32_LE` / `DoP` DAC transport.

---

## 1. 🧠 Silicon & `1:2:1` Core/IRQ Partition (`4× ARM Cortex-A35` + `1 GB LPDDR3`)

The **BATLEXP G350** and **R36S** run on the **Rockchip RK3326** (`4× ARM Cortex-A35`, `32 KB` L1-I / `32 KB` L1-D per core, `256 KB` shared L2, `ARM Mali-G31 MP2` GPU, `1 GB` `32-bit` `LPDDR3`, `3.5" 640×480` `4:3` OCA MIPI DSI IPS screen). Because the Cortex-A35 uses a dual-issue **in-order** microarchitecture sharing a single `256 KB` L2 cache with no dedicated VRAM, unpinned hardware interrupts and background tasks immediately flush L1 caches and inject multi-millisecond audio clicks and frame-pacing judder.

`Piccolo-2go` enforces a strict **`1:2:1` physical core and IRQ partition** via tickless kernel boot flags (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate`:

| Core | Affinity Mask | Scheduler & Governor | Dedicated Hardware Plane & Pinned IRQs |
| :--- | :--- | :--- | :--- |
| **Core 0** | `0x01` | `SCHED_FIFO` (prio `80`) / `performance` | **Real-Time Audio Clock & Controller IRQ Plane:** Direct ALSA `S32_LE` writer (`192`-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`), `I2S0` (`RK817`), GPIO button matrix, and `10-bit` `SARADC` control IRQs |
| **Core 1** | `0x02` | `SCHED_OTHER` / `performance` | **Zero-Copy GPU & Display Scanout Plane:** Drives **Mali-G31 MP2** (`Mesa Panfrost` OpenGL ES 3.0) draw calls and atomic `KMS/DRM` page flips (`SDL_VIDEODRIVER=kmsdrm`) to the `640×480 @ 60.00 Hz` panel |
| **Core 2** | `0x04` | `SCHED_OTHER` / `performance` | **Deterministic Emulation & DSP Plane:** Runs `FBNeo` / `Amiberry` / `Hatari` JIT and DSP workers with hot `32 KB` L1-I / `32 KB` L1-D caches and zero thread migration |
| **Core 3** | `0x08` | `SCHED_OTHER` (`nice +5`) / `schedutil` | **Storage DMA, ZRAM & Housekeeping Plane:** `dwmmc` (`TF1`/`TF2` SanDisk SD) DMA IRQs, `1024 MB` `LZ4` ZRAM, `64 MiB` `FLAC_AX` RAM preloader, `ax_appliance_tui`, and `ax-powerd` |

### ⚙️ RK3326 Clock, Voltage & Memory Tuning Parameters

* **Safe RK3326 CPU PLL Steps (`24 MHz` Increments, Passive-Cooled Safe):**
  * `1296000 kHz` (`1.296 GHz` / `1.30 GHz` — Default): Stock safe ceiling for passive, unvented BATLEXP G350 / R36S shells (`~42–52°C`, zero thermal throttling).
  * `1344000 kHz` (`1.344 GHz` / `1.34 GHz` — Sweet-Spot `+48 MHz`): Mild PLL step (`~52–58°C`), configured via `CPU_PROFILE=1.34` in `/boot/ax-cpu.conf` or `S01piccolo-2go-isolate set-1.34`.
  * `1392000 kHz` (`1.392 GHz` / `1.39 GHz` — High `+96 MHz`): Maximum safe ceiling just under the `1.4 GHz` thermal wall (`~58–65°C`), guarded by `ax-powerd` automatic `75°C` rollback.
* **LPDDR3 / DMC Bus Profiles (`RK817 DCDC2` — `VDD_LOGIC`, `12.5 mV` / `12500 µV` Steps):**
  * `720 MHz` (`1440 MT/s` @ `1112.5 mV` / `1112500 µV` — Default): `max_ddrfreq=720` (`+8.1%` bandwidth over stock `666 MHz` with a single `+12.5 mV` step).
  * `666 MHz` (`1333 MT/s` @ `1100.0 mV` / `1100000 µV` — Safe Backup): Universal fallback profile (`max_ddrfreq=666`) compatible with every clone RAM bin.
  * `800–900 MHz` (`1600–1800 MT/s` @ `1137.5–1162.5 mV` — Open-Case Only): High-bandwidth enthusiast profile reserved for open-case or heatsinked boards.
* **Mali-G31 MP2 Voltage-Scaled OPP Ladder (`12.5 mV` / `12500 µV` `RK817` Steps):**
  * `520 MHz` (`1100.0 mV` / `1100000 µV` — Default): Stock GPU ceiling (`max_gpufreq=520`).
  * `550 MHz` (`1112.5 mV` / `1112500 µV`, `+1` step): Sweet-spot mild boost (`+5.8%` shader/fillrate, minimal heat).
  * `600 MHz` (`1137.5 mV` / `1137500 µV`, `+3` steps): High GPU boost (`+15.4%` shader/fillrate).
  * `650–660 MHz` (`1162.5–1175.0 mV` / `1162500–1175000 µV`, `+5–6` steps): Max PLL ceiling (`+26.9%`, silicon-lottery dependent; guarded by `75°C` thermal rollback).
* **ZRAM, Kernel Pruning & Linear Scanout (`~110 MB` LPDDR3 Reclaimed):**
  * **`1024 MB` `LZ4` ZRAM:** Configured by `S01piccolo-2go-isolate` (`~650 MB/s` in-order Cortex-A35 decompression, `vm.swappiness=120`, `vm.page-cluster=0`, `vm.watermark_scale_factor=125`) alongside bounded `/tmp` (`32 MB`) and `/run` (`16 MB`) `tmpfs`.
  * **Pruned Kernel (`linux-rk3326.fragment`):** Uses `LZ4` kernel compression (`< 180 ms` decompression), `64 MB` CMA, and strips unused SoC blocks (`VPU`, `ISP`, `RGA`, `I2S1/TDM`, `PDM`, `MTD/NAND`, `Bluetooth`, `IPv6`), reclaiming `~110 MB` of physical LPDDR3.
  * **Zero-Compositor Linear Scanout (`PAN_MESA_DEBUG=noafbc`):** Disables `AFBC` tile-header encode/decode passes on `640×480` 2D frames and caches shaders in `/tmp/mesa_shader_cache` (`16M` cap) in RAM.

---

## 2. 🎧 Bit-Perfect Audio DAC Subsystem (`Early HiBy FC3 1.3 Vrms` + `RK817` Fallback)

### 🔌 A. Primary USB-C OTG DAC — Early HiBy FC3 (`ESS ES9281AC Pro`, `1.3 Vrms / 70 mW`)

`Piccolo-2go` is tuned out of the box for the **early HiBy FC3 (`ESS ES9281AC Pro`, `1.3 Vrms / 70 mW @ 32 Ω`, USB VID `0x32BB`)** connected to the handheld's USB-C OTG port:

* **Zero USB Autosuspend (`99-ax-2go-otg.rules`):** Immediately locks `power/control="on"` when the HiBy FC3 (`0x32BB`) or a USB MIDI controller is attached, preventing DAC sleep clicks and latency spikes.
* **Bit-Perfect `S32_LE` & Native `DoP` (`DSD64` / `DSD128`):** Bypasses PulseAudio, PipeWire, and ALSA software resampling (`snd_pcm_hw_params_set_rate_resample(0)`), outputting `32-bit` integer **`S32_LE` (`44.1–384 kHz`)** PCM and hardware **`DoP` (`DSD64` `176.4 kHz` / `DSD128` `352.8 kHz`)** with `64 MiB` RAM file preloading on `Core 3`.
* **Gain-Staged Hardware Volume Presets (`fc3_1v3`):**
  * **Portable Powered Speakers / Line-Out:** `--chip fc3_1v3 --volume 0.95` (`~1.24 Vrms` clean output with zero clipping).
  * **Wired Open/Closed Headphones (e.g., Grado SR325x, Koss Porta Pro):** `--chip fc3_1v3 --volume 0.65`.
  * **Sensitive IEMs (`16–32 Ω`, e.g., Moondrop Chu II / Truthear Hexa):** `--chip fc3_1v3 --volume 0.35`.

### 🔊 B. Interactive Connect & Retry (`ax-audio-route`) & Internal `RK817` Fallback

* **Interactive Connect & Retry (`ax-audio-route prompt`):** If the HiBy FC3 is unplugged when launching an audio session, `Piccolo-2go` prompts you to plug in the USB-C OTG DAC and retry (`[R]`) or fall back cleanly (`[S]`) to the internal codec.
* **Pop-Free Internal `RK817` Fallback (`hw:CARD=rockchiprk817,DEV=0`):** Routes audio to the built-in `3.5 mm` stereo headphone jack and `1W` mono speaker with clean *Playback Path* initialization and pop-free Class-D shutdown.

---

## 3. 💾 Dual SanDisk Extreme PRO Storage (`64 GB TF1` + `128 GB TF2`) & `SDR50` Guard

Because the RK3326's `SDMMC0` (`TF1`) and `SDMMC1` (`TF2`) controllers operate on UHS-I `SDR50` (`50 MHz`, `3.3V` signaling), **random 4K IOPS (`A2` / `V30` rating)** governs boot speed and ROM/WHDLoad responsiveness. Keep your original stock card safely stored as a backup and use genuine **SanDisk Extreme PRO (`A2` / `V30` / `U3`)** cards:

| Slot | Recommended Card | Partition Layout & Mount | Purpose & Runtime Guard |
| :--- | :--- | :--- | :--- |
| **`TF1` (Primary OS Slot)** | **SanDisk Extreme PRO `64 GB`** (`A2` / `V30`) | • Raw `32 KB`: `idbloader.img` (`SPL`/`TPL`)<br>• Raw `8 MB`: `u-boot.itb` (`ATF BL31`)<br>• `p1`: `BOOT` (`256 MB`, `FAT32`)<br>• `p2`: `ROOTFS` (`< 140 MB`, `128 KB`-block `zstd` SquashFS)<br>• `p3`: `STORAGE` (`~59 GB`, auto-expanded `ext4`) | Holds the immutable read-only OS (immune to sudden power loss), `LZ4` kernel, `640×480` Espresso/Copper `logo.bmp`, `/boot/dtbs/`, save states, and local `/userdata` (`noatime,nodiratime,commit=60,data=writeback`). |
| **`TF2` (Secondary Media Slot)** | **SanDisk Extreme PRO `128 GB`** (`A2` / `V30`) | • `p1`: `PICCOLO_MEDIA` (`~119 GB`, **`exFAT`** or **`ext4`**) | Dedicated portable media library automatically bound to `/userdata/{roms,music,bios}` on boot (`S02piccolo-2go-storage`) for `FBNeo` arcade/Neo Geo ROMs, lossless `.flac` / `DoP` albums, and `.mod`/`.xm`/`.s3m`/`.it` tracker files. |

* **Runtime `SDR50` Bus Verification (`S02piccolo-2go-storage`):** Probes `mmc0` (`TF1`) and `mmc1` (`TF2`) at boot to verify **`SDR50` (`50 MHz`, `3.3V` signaling, `~24 MB/s`)**—automatically stripping unstable `SDR104` on fixed-`3.3V` VQMMC boards or flashing a console warning and locking safe `25 MHz` High-Speed mode if CRC tuning fails.

---

## 4. 🕹️ Physical Controls, Espresso TUI & Safe Power Guard (`ax-powerd`)

* **Zero-Latency Controls (`Core 0` IRQ):** Dedicated GPIO lines read the D-pad, quiet face buttons (`A/B/X/Y`), sloped shoulder triggers (`L1/L2`, `R1/R2`), `Start`, `Select`, `FN` (`BTN_TRIGGER_HAPPY5` / `BTN_MODE`), and `KEY_POWER`, while the `10-bit` `SARADC` samples dual analog thumbsticks.
* **Cold Boot into `ax_appliance_tui` (`tty1`):** Boots directly into the 24-bit Espresso/Copper TUI launcher (`#0B0806` canvas, `#C88A55` copper accent, `#F4EDE4` warm cream text) with live DAC lock status, RAM/ZRAM telemetry, and instant launch into `FLAC_AX` or `RetroArch` + `FBNeo` (`640×480` integer-scaled KMS/Panfrost).
* **Safe Shutdown & Hardware Protection (`ax-powerd` on `Core 3`):**
  * **Graceful Power-Off:** Hold **`KEY_POWER` for `1.5` seconds** (or press `FN + Power`). `ax-powerd` sends `SIGUSR1` to release the HiBy FC3 DAC cleanly, flushes save files, unmounts both SanDisk cards, and powers off the `RK817` PMIC without SD corruption.
  * **Thermal & Brownout Guards:** Automatically downclocks the CPU cluster back to `1.296 GHz` (`1.30 GHz`) if `thermal_zone0` reaches **`75°C`**, and triggers an emergency clean shutdown if LiPo battery voltage drops to **`3280 mV`**.

---

## 5. 🚀 Installation, DTB Calibration & Source Build References

* **[Installation & First-Boot Setup Guide (`docs/Piccolo_2go_Installation_and_First_Boot.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/Piccolo_2go_Installation_and_First_Boot.md)** — Flashing `sdcard.img` onto `TF1`, formatting `TF2` (`PICCOLO_MEDIA`), selecting `dtb-select` aliases (`g350`, `r36s-panel0..4`, `r36s-v21`, `r36s-plus`, `r36s-pro`, `r36s-xx`), and calibrating `3,200 mAh` `rk817` battery gauge and `50 Hz` PAL / `60 Hz` NTSC timings.
* **[System Architecture & Topology (`docs/ARCHITECTURE.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/ARCHITECTURE.md)** — Full RK3326 / RK817 chip interconnect diagram, Cortex-A35 NEON/CRC32 compiler flags, and Panfrost `noafbc` scanout details.
* **[Building from Source (`docs/BUILDING.md`)](https://github.com/galaxywarriordad/Piccolo-2go/blob/main/docs/BUILDING.md)** — Out-of-tree Buildroot `BR2_EXTERNAL` compilation steps for `piccolo_2go_rk3326_defconfig`.

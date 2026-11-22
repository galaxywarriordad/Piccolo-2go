# 📖 Piccolo-2go — Complete User Guide & Hardware Manual (BATLEXP G350 & R36S)

**Piccolo-2go** is an ultra-lean (`< 180 MB` immutable `SquashFS`), deterministic 64-bit (`aarch64`) Linux handheld OS built on Buildroot for the **BATLEXP G350** and **R36S** (`Rockchip RK3326`), **optimised with AX tech**. Designed as the portable sibling to **Piccolo** within **The Silicon Preservation Society** ecosystem, it eliminates desktop bloat, background daemons, and user-space audio mixers (`PulseAudio`/`PipeWire`) to turn a £50–£60 pocket handheld into a cycle-accurate retro console and bit-perfect audiophile transport.

---

## 1. Silicon & `1:2:1` Core Topology Optimisations

The **BATLEXP G350** and **R36S** run on the **Rockchip RK3326** (`4× ARM Cortex-A35 @ 1.5 GHz`, `Mali-G31 MP2` GPU, `1 GB LPDDR3-1600`, `3.5" 640×480` 4:3 OCA IPS screen). Because the Cortex-A35 uses a dual-issue **in-order** microarchitecture sharing a single `256 KB` L2 cache, unpinned interrupts and background tasks cause immediate audio clicks and frame-pacing judder.

`Piccolo-2go` is **optimised with AX tech** to enforce a strict **`1:2:1` physical core partition** via kernel boot isolation (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate`:

| Core Mask | Physical Core | Governor & Priority | Dedicated Role & Pinned Hardware IRQs |
| :--- | :--- | :--- | :--- |
| **`0x01`** | **Core 0** | `SCHED_FIFO` (`80`) for Audio<br>`performance` | **Real-Time Audio Clock & Input Plane:** Direct ALSA `S32_LE` writer (`192`-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`), `I2S0` (`RK817`), and `GPIO`/`SARADC` control IRQs. |
| **`0x02`** | **Core 1** | `SCHED_OTHER`<br>`performance` (in-game) | **Zero-Copy Display Plane:** Direct **KMS/DRM + Mesa Panfrost GLES 3.0** scanout (`640×480 @ 60 Hz`, zero X11/Wayland compositor overhead). |
| **`0x04`** | **Core 2** | `SCHED_OTHER`<br>`performance` (in-game) | **Emulation JIT & DSP Execution Plane:** Dedicated 68020/AGA JIT, Z80/68k cycle-accurate emulation, and DSP synthesis worker. |
| **`0x08`** | **Core 3** | `SCHED_OTHER` (`nice +5`)<br>`schedutil` | **Storage DMA, RAM Preload & Housekeeping Plane:** `dwmmc` (`TF1`/`TF2` microSD) IRQs, `384 MB` `zstd` ZRAM, `FLAC_AX` `64 MiB` `SingleFile` preloader, `ax_appliance_tui`, and `ax-powerd`. |

### Additional RK3326 Silicon & Memory Optimisations
* **384 MB `zstd` ZRAM (`vm.swappiness=100`, `vm.page-cluster=0`):** Compresses cold background pages in RAM at ~`2.8:1` (`~1.05 GB` effective swap) with single-page (`4 KB`) latency and **zero microSD flash wear**.
* **Pruned SoC Kernel (`linux-rk3326.fragment`):** Shrinks Contiguous Memory Allocator (CMA) space to `64 MB` and disables unused hardware blocks (`VPU`, `ISP`, `RGA`, `Crypto`, `I2S1/TDM`, `PDM`, `MTD/NAND`, `Bluetooth`, `IPv6`), freeing **~110 MB of physical LPDDR3** for games and audio buffers.
* **Modular USB-C OTG Drivers (`=m`):** Wi-Fi (`RTL8xxxU` / `MT7601U`) and USB-C Ethernet (`RTL8152` / `AX88179`) drivers consume **0 KB of RAM and 0 CPU cycles** until an adapter is physically plugged into the OTG port.

---

## 2. Audio DAC & Dual SanDisk Extreme PRO Requirements

### A. Early HiBy FC3 (`1.3 Vrms / 70 mW`) USB-C OTG DAC & Internal `RK817` Fallback
`Piccolo-2go` is tuned out of the box for the **early HiBy FC3 (`ES9281AC Pro`, `1.3 Vrms / 70 mW @ 32 Ω`, USB VID `0x32BB`)** connected to the handheld's USB-C OTG port (via a short USB-C to USB-C OTG adapter/cable):
* **Zero USB Autosuspend (`99-ax-2go-otg.rules`):** Immediately locks `power/control="on"` when the HiBy FC3 is attached, preventing DAC sleep clicks between tracks.
* **Bit-Perfect `S32_LE` & Native `DoP` (`DSD64` / `DSD128`):** Bypasses all software resampling (`snd_pcm_hw_params_set_rate_resample(0)`), outputting `44.1–384 kHz` `S32_LE` PCM and `176.4 kHz` / `352.8 kHz` `DoP` streams.
* **Gain-Staged Hardware Volume Presets (`fc3_1v3`):**
  * **Portable Powered Speakers / Line-Out:** `--chip fc3_1v3 --volume 0.95` (`~1.24 Vrms` clean output with zero clipping).
  * **Wired Open/Closed Headphones (e.g., Grado SR325x, Koss Porta Pro):** `--chip fc3_1v3 --volume 0.65`.
  * **Sensitive IEMs (`16–32 Ω`, e.g., Moondrop Chu II / Truthear Hexa):** `--chip fc3_1v3 --volume 0.35`.
* **Automatic Internal Fallback:** Unplugging the USB-C DAC seamlessly reverts ALSA output to the internal **Rockchip RK817** codec (`hw:CARD=rockchiprk817,DEV=0`) driving the built-in `3.5 mm` headphone jack and bottom-firing mono speaker.

### B. Dual SanDisk Extreme PRO Storage Requirements (`64 GB` + `128 GB`)
Because the RK3326's `SDMMC0` (`TF1`) and `SDMMC1` (`TF2`) controllers run on UHS-I `SDR50` (`50 MHz`), **random 4K IOPS (`A2` / `V30` rating)** determines boot speed and WHDLoad/ROM loading responsiveness. Always keep your stock factory microSD card safely stored as a backup and flash onto genuine **SanDisk Extreme PRO (`A2` / `V30` / `U3`)** cards:

| Slot | Recommended Card | Partition Layout | Purpose |
| :--- | :--- | :--- | :--- |
| **`TF1` (Primary OS Slot)** | **SanDisk Extreme PRO 64 GB** (`A2` / `V30`) | `p1`: `BOOT` (`256 MB`, `FAT32`)<br>`p2`: `ROOTFS` (`256 MB`, read-only `SquashFS`)<br>`p3`: `STORAGE` (`~59 GB`, auto-expanded `ext4`) | Holds the immutable OS, Device Tree Blobs (`/boot/dtbs/`), shader caches, save states, and local fallback `/userdata`. Reflashing `TF1` never touches your media library. |
| **`TF2` (Secondary Media Slot)** | **SanDisk Extreme PRO 128 GB** (`A2` / `V30`) | `p1`: `PICCOLO_MEDIA` (`~119 GB`, **`exFAT`** or **`ext4`**) | Dedicated portable library automatically bound to `/userdata/{roms,music,bios}` on boot (`S02piccolo-2go-storage`). Format as `exFAT` to drag-and-drop ROMs and FLACs directly from Windows, macOS, or Linux. |

---

## 3. Step-by-Step Installation & First-Boot Setup

### Step 1: Flash `TF1` (`64 GB` SanDisk Extreme PRO)
1. Download or build `sdcard.img` (`output/images/sdcard.img`).
2. Flash `sdcard.img` onto your **64 GB SanDisk Extreme PRO** using **Rufus** (Windows), **balenaEtcher** (Windows/macOS/Linux), or `dd` on Linux:
   ```sh
   sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
   ```
3. **Select Your Device Tree (DTB) on `BOOT` (`p1`):**
   * Re-insert the `64 GB` card into your PC and open the `BOOT` (`FAT32`) partition.
   * Copy the matching `.dtb` for your hardware to the root of `BOOT` renamed as **`rk3326-linux.dtb`**:
     * **BATLEXP G350 (Default):** Copy `dtbs/rk3326-batlexp-g350.dtb` → `rk3326-linux.dtb`
     * **R36S (Original V1 Screen):** Copy `dtbs/rk3326-r36s-v1.dtb` → `rk3326-linux.dtb`
     * **R36S (Panel 4 Revision):** Copy `dtbs/rk3326-r36s-panel4.dtb` → `rk3326-linux.dtb`
   *(Tip: If you ever boot with a black screen on a clone revision, swap `rk3326-linux.dtb` on `BOOT`—or run `dtb-select g350` / `dtb-select r36s-panel4` over SSH/serial.)*

### Step 2: Prepare `TF2` (`128 GB` SanDisk Extreme PRO)
1. Format your **128 GB SanDisk Extreme PRO** as a single **`exFAT`** (or `ext4`) partition labeled `PICCOLO_MEDIA`.
2. Create three top-level folders on `TF2`:
   * `roms/` (`amiga`, `amigacd32`, `atarist`, `megadrive`, `psx`, `snes`, `gba`, `arcade`)
   * `music/` (your lossless `.flac` albums, `.dsf`/`.dff` DSD files, and Amiga/ST `.mod`/`.xm`/`.s3m`/`.it` tracker modules)
   * `bios/` (Amiga Kickstart `kick34005.A500` / `kick40068.A1200`, Atari ST `tos.img`, PS1 `scph5501.bin`)

### Step 3: First Boot & Safe Shutdown
1. Insert the **64 GB** card into **`TF1` (OS)** and the **128 GB** card into **`TF2` (Media)**.
2. Hold the **Power** button for ~2 seconds to boot. On first boot, `S02piccolo-2go-storage` automatically expands `TF1` `p3` (`~59 GB` `ext4`) and binds `TF2` (`128 GB`) into `/userdata`.
3. **Safe Shutdown (`ax-powerd`):** Always **hold the Power button for `1.5` seconds** to power off. `ax-powerd` (pinned to Core `3`) sends `SIGUSR1` to release the HiBy FC3 DAC cleanly, flushes save files, unmounts both SanDisk cards, and powers down the RK817 PMIC without SD corruption.

---

## 4. UI on Boot (`Piccolo_2go` Espresso TUI — Optimised with AX Tech)

Within ~6 seconds of power-on, `Piccolo-2go` launches **`ax_appliance_tui`** directly on the `640×480` IPS framebuffer (`80×30` crisp VGA text cells, `#0B0806` Espresso canvas, `#C88A55` coffee highlight, `#F4EDE4` cream text) pinned to **Core `3` (`0x08`)**:

```text
 PICCOLO_2GO (Optimised with AX tech) | 1:2:1 RK3326 (Core 3 UI)
 ────────────────────────────────────────────────────────────────────────
 [ CORES 0-2: RT AUDIO/EMU ]  [ CORE 3: IO/UI ]
  HiBy FC3 [USB 0x32BB] • S32_LE Ready [UNLOCKED]
  SQUASHFS SD  |  RAM: 118 / 984 MB  |  TEMP: 41°C

  APPLIANCE LAUNCHER
 ────────────────────────────────────────────────────────────────────────
 > FLAC_AX Audiophile      — Bit-Perfect FLAC, DSD & .MOD Player
   Amiberry                — Commodore Amiga 500 / 1200 / CD32 (AX tech)
   Hatari                  — Atari ST / STE / TT / Falcon (AX tech)
   BlastEm                 — Cycle-Accurate Sega Mega Drive (AX tech)
   PlayStation 1           — PCSX-ReARMed / DuckStation 640x480
   DTB Panel Selector      — Switch BATLEXP G350 / R36S Panel
   Safe Shutdown           — Sync Dual SanDisk & Power Off
 ────────────────────────────────────────────────────────────────────────
 ↑/↓ SELECT  |  ENTER LAUNCH  |  R REFRESH  |  Q SHELL
```

* **Live Telemetry Header:** Automatically detects whether your **HiBy FC3 (`0x32BB`)** is connected via USB-C OTG or whether audio is routed to the internal **RK817 `3.5 mm` / mono speaker**, alongside live LPDDR3 usage and SoC temperature.
* **Handheld Controls:** Navigate with the **D-Pad (`Up/Down`)**, launch with **A / Start (`Enter`)**, refresh DAC/SD status with **X (`R`)**, or drop to a root maintenance shell with **Select (`Q`)**.

---

## 5. Available Apps & Retro Systems

A. Bit-Perfect Audio & Tracker Transport (FLAC_AX — Optimised with AX Tech)

• Lossless PCM & Native DoP: Plays 16-bit and 24-bit .flac (44.1–384 kHz) and .dsf / .dff (DSD64 176.4 kHz & DSD128 352.8 kHz DoP) straight to your early HiBy FC3 (1.3 Vrms) via SCHED_FIFO Core 0, preloading one track at a time into a safe 64 MiB locked RAM window.
• Amiga & Atari ST Tracker Jukebox (libopenmpt): Plays classic .mod, .xm, .s3m, .it, .med, and .okt demoscene and game soundtracks at bit-perfect S32_LE with adjustable Amiga stereo separation (--mod-stereo 75) and 8-tap sinc interpolation.

B. Supported Console & Home Computer Engines (640×480 Native 4:3)

Because the BATLEXP G350 and R36S feature a native 640×480 4:3 IPS screen, classic 320×240 and 320×200 systems scale at an exact 2× integer ratio with razor-sharp pixels and zero aspect distortion:

System	Engine (Optimised with AX Tech)	Performance & Scaling on RK3326 (1.5 GHz)
Commodore Amiga (A500, A1200, CD32)	Amiberry (optimised with AX tech)	Full 60/50 Hz speed on OCS/ECS/AGA & WHDLoad .lha titles with ax-sdk Core 0 Paula audio (S32_LE).
Atari ST / STE / TT / Falcon	Hatari (optimised with AX tech)	Cycle-accurate 68000/68030 + YM2149 PSG & STE DMA stereo sound at locked 60/50 Hz.
Sega Mega Drive / Genesis / Master System	BlastEm (optimised with AX tech)	Cycle-accurate 68k + Z80 + YM2612 FM synthesis at native 640×480 (2× integer scale).
Sony PlayStation 1 (PSX)	PCSX-ReARMed	Full-speed 60 fps across the PS1 library using NEON GPU/SPU acceleration at 640×480.
SNES, NES, Game Boy / GBC / GBA, PC Engine	Libretro / Standalone	Effortless 60 fps with sub-frame input latency and schedutil battery-saving clocks (~6–7 hours runtime).
Classic 80s/90s Arcade (MAME / FBNeo)	FBNeo / Cannonball (optimised with AX tech)	Full-speed 16-bit arcade classics (Capcom CPS1/CPS2, Neo Geo, Sega OutRun 60 fps).

Audiophile Sound Quality & 1:2:1 Performance Engineering

A. Why Piccolo-2go Sounds Dramatically Better Than Stock Handheld OSes

On stock RK3326 firmware (Ubuntu/Debian or unisolated EmuELEC forks), every audio stream passes through PulseAudio or PipeWire, gets software-resampled to 44.1 kHz/48 kHz (float32 → S16_LE), suffers USB autosuspend power-saving clicks, and shares CPU cores with microSD DMA and screen refreshes—causing audible background hiss, softened transients, and micro-dropouts.
Piccolo-2go (optimised with AX tech) eliminates every single bottleneck in the signal chain:
• Zero Software Resampling (snd_pcm_hw_params_set_rate_resample(0)): A 44.1 kHz CD rip, 96 kHz / 192 kHz studio master, DSD64 (176.4 kHz DoP), or 48 kHz Amiga/Mega Drive synth stream locks the HiBy FC3's ESS ES9281AC Pro hardware clock directly to the exact native sample rate—preserving micro-detail, soundstage depth, and razor-sharp percussion transients.
• 32-Bit Integer Bit-Perfect Pipeline (S32_LE) + Hardware Attenuator Gain Staging: Instead of crushing dynamic range with digital volume sliders, FLAC_AX left-shifts 16-bit and 24-bit samples cleanly into S32_LE and controls volume inside the ES9281AC Pro's hardware attenuator (fc3_1v3: --volume 0.95 for line-out/speakers, 0.65 for headphones, 0.35 for sensitive IEMs), keeping the noise floor dead silent even on 16 Ω IEMs.
• Dedicated Core 0 (0x01) SCHED_FIFO 80 Audio Clock & Powered-Down Internal Speaker Amp: Pinning DWC2 USB-C OTG IRQs and the ALSA writer exclusively to Core 0 (nohz_full, zero timer ticks) eliminates USB packet jitter, while hot-plugging the early HiBy FC3 (1.3 Vrms) powers down the internal RK817 Class-D speaker amplifier to remove PMIC switching noise from the power rails.

B. How the 1:2:1 Topology Squeezes the Last Drop of Juice Out of the RK3326

The Rockchip RK3326's four ARM Cortex-A35 (1.5 GHz) cores are in-order processors sharing a tiny 256 KB L2 cache and 1 GB LPDDR3 bus. Stock OSes let the Linux scheduler bounce threads randomly across all 4 cores, flushing the 32 KB L1 caches thousands of times per second and starving the Mali-G31 GPU of memory bandwidth.
Piccolo-2go extracts maximum frames per second and maximum battery life from the 3,200 mAh cell by locking each workload into its own cache-coherent lane:
• Zero L1/L2 Cache Thrashing (isolcpus=0,1,2): Core 1 (0x02) stays dedicated to KMS/DRM + Panfrost GLES 3.0 (640×480 direct scanout with zero X11/Wayland compositor copy), while Core 2 (0x04) stays dedicated to the 68k/Z80/MIPS emulator JIT—keeping instruction and data caches hot for full 60.00 Hz / 50.00 Hz frame pacing without dips.
• Zero SD-Card Stutter (Core 3 0x08 Isolation): All dwmmc microSD interrupts (TF1 64 GB OS + TF2 128 GB media), 384 MB zstd ZRAM page compression, and FLAC_AX 64 MiB file preloading run strictly on Core 3. Loading a 30 MB Amiga .hdf hardfile or streaming a 24-bit/192 kHz FLAC never steals a single clock cycle from Core 0 audio or Cores 1–2 gameplay.
• ~110 MB Reclaimed Physical RAM & Longer Battery Runtime: Stripping unused SoC silicon (VPU, ISP, RGA, Crypto, I2S1/TDM, PDM, Bluetooth, IPv6) and running schedutil frequency scaling during headless FLAC_AX listening lets Cores 1–2 sleep cleanly, stretching the 3,200 mAh battery to 6–7 hours of retro gaming or 7.5+ hours as a dedicated pocket Hi-Fi DAP.

C. Deep-Dive Application Suite (optimised with AX tech)

Application / Engine	What It Does & How AX Tech Elevates It on the BATLEXP G350 / R36S
FLAC_AX Audiophile Transport
(Bit-Perfect Lossless & DoP Player)	Turns the handheld + early HiBy FC3 (1.3 Vrms) into a pocket reference DAP. Preloads tracks into mlockall-pinned RAM (64 MiB safe window on 1 GB LPDDR3) on Core 3 and feeds bit-perfect 16/24-bit FLAC (44.1–384 kHz) and native DSD64/DSD128 (DoP) over SCHED_FIFO Core 0 with zero clicks, zero resampling, and instant gapless track transitions.
FLAC_AX Tracker Jukebox (libopenmpt)
(Amiga & Atari ST .MOD/.XM/.S3M/.IT)	Plays authentic Amiga ProTracker (.mod), FastTracker II (.xm), ScreamTracker (.s3m), Impulse Tracker (.it), and OctaMED (.med) demoscene/game modules directly to S32_LE with configurable Amiga stereo separation (--mod-stereo 75) and 8-tap sinc interpolation—sounding richer and cleaner than any original hardware Paula chip.
ax_appliance_tui Espresso Launcher
(Zero-Dependency Framebuffer UI)	Boots in ~6 seconds directly onto the 640×480 IPS screen (80×30 cells, zero X11/Wayland overhead) on Core 3. Displays real-time /run/lock/ax-dac.lock status for your HiBy FC3 (0x32BB) or internal RK817 codec, live LPDDR3 + ZRAM telemetry, and SoC temperature.
Amiberry (optimised with AX tech)
(Commodore Amiga 500 / 1200 / CD32)	Runs OCS, ECS, and AGA Amiga titles plus WHDLoad .lha archives at locked 50 Hz PAL / 60 Hz NTSC. Splits 68020 JIT + Paula audio synthesis cleanly across Cores 1–2 while Core 0 delivers crisp S32_LE stereo separation through your HiBy FC3 or headphones.
Hatari (optimised with AX tech)
(Atari ST / STE / TT / Falcon)	Cycle-accurate Motorola 68000/68030 emulation with authentic YM2149 PSG and Atari STE 50 kHz DMA stereo audio routed straight to Core 0 ALSA S32_LE with zero buffer drift.
BlastEm (optimised with AX tech)
(Sega Mega Drive / Genesis / Master System)	Cycle-accurate 68k + Z80 + YM2612 FM + SN76489 PSG synthesis scaled at an exact 2× integer ratio (640×480) on the G350's 4:3 IPS display—delivering sub-frame input response and punchy FM bass through the HiBy FC3.
PCSX-ReARMed
(Sony PlayStation 1)	Uses ARM NEON GPU/SPU acceleration to run the 32-bit PS1 library at full 60 fps in native 640×480 (2× 320×240) with CD-DA audio routed bit-perfectly over ALSA.
FBNeo & Cannonball (optimised with AX tech)
(80s/90s Arcade & Sega OutRun)	Runs Capcom CPS1/CPS2, SNK Neo Geo, and the C++ Cannonball Sega OutRun engine at a locked 60 fps on the 640×480 4:3 panel.
dtb-select & ax-powerd
(Hardware Resilience Daemons)	dtb-select switches MIPI DSI panel Device Trees between BATLEXP G350 and R36S revisions without reflashing; ax-powerd (Core 3) intercepts the physical Power button (1.5s hold) to release the HiBy FC3 DAC cleanly (SIGUSR1), flush save states, unmount both SanDisk Extreme PRO cards, and power off safely.

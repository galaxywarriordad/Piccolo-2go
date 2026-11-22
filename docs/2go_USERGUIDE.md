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


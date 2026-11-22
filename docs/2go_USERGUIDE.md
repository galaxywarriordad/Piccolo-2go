# ☕📖 Piccolo-2go — Complete User Guide & Hardware Manual (BATLEXP G350 & R36S)

**Piccolo-2go** is an ultra-lean (`< 180 MB` immutable `SquashFS`), deterministic 64-bit (`aarch64`) Linux handheld OS built on Buildroot for the **BATLEXP G350** and **R36S** (`Rockchip RK3326`), **optimised with AX tech**. Designed as the portable sibling to **Piccolo** within **The Silicon Preservation Society** ecosystem, it eliminates desktop bloat, background daemons, and user-space audio mixers (`PulseAudio`/`PipeWire`) to turn a £50–£60 pocket handheld into a cycle-accurate retro console and bit-perfect audiophile transport.

---

## 1. 🧠 Silicon & `1:2:1` Core Topology Optimisations

The **BATLEXP G350** and **R36S** run on the **Rockchip RK3326** (`4× ARM Cortex-A35 @ 1.5 GHz`, `Mali-G31 MP2` GPU, `1 GB LPDDR3-1600`, `3.5" 640×480` 4:3 OCA IPS screen). Because the Cortex-A35 uses a dual-issue **in-order** microarchitecture sharing a single `256 KB` L2 cache, unpinned interrupts and background tasks cause immediate audio clicks and frame-pacing judder.

`Piccolo-2go` is **optimised with AX tech** to enforce a strict **`1:2:1` physical core partition** via kernel boot isolation (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2`) and `S01piccolo-2go-isolate`:

| Core Mask | Physical Core | Governor & Priority | Dedicated Role & Pinned Hardware IRQs |
| :--- | :--- | :--- | :--- |
| **`0x01`** | **Core 0** | `SCHED_FIFO` (`80`) for Audio<br>`performance` | **Real-Time Audio Clock & Input Plane:** Direct ALSA `S32_LE` writer (`192`-frame / `4.00 ms` period); `DWC2` USB-C OTG (`HiBy FC3`), `I2S0` (`RK817`), and `GPIO`/`SARADC` control IRQs. |
| **`0x02`** | **Core 1** | `SCHED_OTHER`<br>`performance` (in-game) | **Zero-Copy Display Plane:** Direct **KMS/DRM + Mesa Panfrost GLES 3.0** scanout (`640×480 @ 60 Hz`, zero X11/Wayland compositor overhead). |
| **`0x04`** | **Core 2** | `SCHED_OTHER`<br>`performance` (in-game) | **Emulation JIT & DSP Execution Plane:** Dedicated 68020/AGA JIT, Z80/68k cycle-accurate emulation, and DSP synthesis worker. |
| **`0x08`** | **Core 3** | `SCHED_OTHER` (`nice +5`)<br>`schedutil` | **Storage DMA, RAM Preload & Housekeeping Plane:** `dwmmc` (`TF1`/`TF2` microSD) IRQs, `384 MB` `zstd` ZRAM, `FLAC_AX` `64 MiB` `SingleFile` preloader, `ax_appliance_tui`, and `ax-powerd`. |

### ⚙️ Additional RK3326 Silicon & Memory Optimisations

* **384 MB `zstd` ZRAM (`vm.swappiness=100`, `vm.page-cluster=0`):** Compresses cold background pages in RAM at ~`2.8:1` (`~1.05 GB` effective swap) with single-page (`4 KB`) latency and **zero microSD flash wear**.
* **Pruned SoC Kernel (`linux-rk3326.fragment`):** Shrinks Contiguous Memory Allocator (CMA) space to `64 MB` and disables unused hardware blocks (`VPU`, `ISP`, `RGA`, `Crypto`, `I2S1/TDM`, `PDM`, `MTD/NAND`, `Bluetooth`, `IPv6`), freeing **~110 MB of physical LPDDR3** for games and audio buffers.
* **Modular USB-C OTG Drivers (`=m`):** Wi-Fi (`RTL8xxxU` / `MT7601U`) and USB-C Ethernet (`RTL8152` / `AX88179`) drivers consume **0 KB of RAM and 0 CPU cycles** until an adapter is physically plugged into the OTG port.

---

## 2. 🎧 Audio DAC & 💾 Dual SanDisk Extreme PRO Requirements

### 🔌 A. Early HiBy FC3 (`1.3 Vrms / 70 mW`) USB-C OTG DAC & Internal `RK817` Fallback

`Piccolo-2go` is tuned out of the box for the **early HiBy FC3 (`ES9281AC Pro`, `1.3 Vrms / 70 mW @ 32 Ω`, USB VID `0x32BB`)** connected to the handheld's USB-C OTG port (via a short USB-C to USB-C OTG adapter/cable):

* **Zero USB Autosuspend (`99-ax-2go-otg.rules`):** Immediately locks `power/control="on"` when the HiBy FC3 is attached, preventing DAC sleep clicks between tracks.
* **Bit-Perfect `S32_LE` & Native `DoP` (`DSD64` / `DSD128`):** Bypasses all software resampling (`snd_pcm_hw_params_set_rate_resample(0)`), outputting `44.1–384 kHz` `S32_LE` PCM and `176.4 kHz` / `352.8 kHz` `DoP` streams.
* **Gain-Staged Hardware Volume Presets (`fc3_1v3`):**
  * **Portable Powered Speakers / Line-Out:** `--chip fc3_1v3 --volume 0.95` (`~1.24 Vrms` clean output with zero clipping).
  * **Wired Open/Closed Headphones (e.g., Grado SR325x, Koss Porta Pro):** `--chip fc3_1v3 --volume 0.65`.
  * **Sensitive IEMs (`16–32 Ω`, e.g., Moondrop Chu II / Truthear Hexa):** `--chip fc3_1v3 --volume 0.35`.
* **Automatic Internal Fallback:** Unplugging the USB-C DAC seamlessly reverts ALSA output to the internal **Rockchip RK817** codec (`hw:CARD=rockchiprk817,DEV=0`) driving the built-in `3.5 mm` headphone jack and bottom-firing mono speaker.

### 💳 B. Dual SanDisk Extreme PRO Storage Requirements (`64 GB` + `128 GB`)

Because the RK3326's `SDMMC0` (`TF1`) and `SDMMC1` (`TF2`) controllers run on UHS-I `SDR50` (`50 MHz`), **random 4K IOPS (`A2` / `V30` rating)** determines boot speed and WHDLoad/ROM loading responsiveness. Always keep your stock factory microSD card safely stored as a backup and flash onto genuine **SanDisk Extreme PRO (`A2` / `V30` / `U3`)** cards:

| Slot | Recommended Card | Partition Layout | Purpose |
| :--- | :--- | :--- | :--- |
| **`TF1` (Primary OS Slot)** | **SanDisk Extreme PRO 64 GB** (`A2` / `V30`) | `p1`: `BOOT` (`256 MB`, `FAT32`)<br>`p2`: `ROOTFS` (`256 MB`, read-only `SquashFS`)<br>`p3`: `STORAGE` (`~59 GB`, auto-expanded `ext4`) | Holds the immutable OS, Device Tree Blobs (`/boot/dtbs/`), shader caches, save states, and local fallback `/userdata`. Reflashing `TF1` never touches your media library. |
| **`TF2` (Secondary Media Slot)** | **SanDisk Extreme PRO 128 GB** (`A2` / `V30`) | `p1`: `PICCOLO_MEDIA` (`~119 GB`, **`exFAT`** or **`ext4`**) | Dedicated portable library automatically bound to `/userdata/{roms,music,bios}` on boot (`S02piccolo-2go-storage`). Format as `exFAT` to drag-and-drop ROMs and FLACs directly from Windows, macOS, or Linux. |

---

## 3. 🚀 Go to Installation & First-Boot Setup guide

# 🚀 Piccolo-2go — Installation & First-Boot Setup Guide

* **Target Hardware:** BATLEXP G350 & R36S Handheld Consoles (`dtb-select` runtime panel selector)
* **SoC & PMIC:** Rockchip RK3326 (`4× ARM Cortex-A35`, `ARM Mali-G31 MP2`, `1 GB LPDDR3`) + Rockchip `RK817`
* **Firmware:** `Piccolo-2go` (`aarch64` Buildroot `BR2_EXTERNAL`, `128 KB`-block `zstd` SquashFS `< 140 MB`)
* **Ecosystem:** **The Silicon Preservation Society** (Optimised with AX Tech)

**Piccolo-2go** is a deterministic 64-bit (`aarch64`) Buildroot operating system (`128 KB`-block `zstd` SquashFS `< 140 MB`) engineered as a closed, fixed-hardware retro console and bit-perfect audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon. This guide covers flashing the `TF1` (`64 GB`) OS image, preparing the `TF2` (`128 GB`) media card, selecting Device Tree Blobs (`DTBs`), performing safe shutdowns via `ax-powerd`, and applying on-device hardware and clock calibrations.

---

## 💿 1. Flash `TF1` (`64 GB` SanDisk Extreme PRO) & Select DTB

### 🛠️ Flash `sdcard.img` to `TF1`

1. Obtain or compile `sdcard.img` (`build-piccolo-2go/images/sdcard.img` or `output/images/sdcard.img`).
2. Flash `sdcard.img` onto your **64 GB SanDisk Extreme PRO (`A2` / `V30`)** card using **Rufus** (Windows), **balenaEtcher** (Windows/macOS/Linux), or `dd` on Linux:

```sh
sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
```

### 🖥️ Select Your Device Tree (DTB) on `BOOT` (`p1`)

1. Re-insert the `64 GB` card into your host PC and open the `BOOT` (`256 MB`, `FAT32`) partition.
2. Copy the matching `.dtb` for your handheld revision to the root of `BOOT` as **`rk3326-linux.dtb`**:
   * **BATLEXP G350 (Default):** Copy `dtbs/rk3326-batlexp-g350.dtb` → `rk3326-linux.dtb`
   * **R36S (Original V1 Screen):** Copy `dtbs/rk3326-r36s-v1.dtb` → `rk3326-linux.dtb`
   * **R36S (Panel 4 Revision):** Copy `dtbs/rk3326-r36s-panel4.dtb` → `rk3326-linux.dtb`

> 💡 **Tip:** If you boot to a black screen on a clone or panel revision, swap `rk3326-linux.dtb` on `BOOT`—or run `dtb-select` (`g350`, `r36s-panel0..4`, `r36s-v21`, `r36s-plus`, `r36s-pro`, `r36s-xx`, `r36s-t1-p1..5`, `r36s-t2-p1..2`) over SSH/serial.

---

## 📂 2. Prepare `TF2` (`128 GB` SanDisk Extreme PRO)

### 🗂️ Partition & Directory Layout

1. Format your **128 GB SanDisk Extreme PRO (`A2` / `V30`)** card as a single **`exFAT`** (or `ext4`) partition labeled `PICCOLO_MEDIA` (`~119 GB` usable).
2. Create three top-level directories on `TF2` (automatically bound to `/userdata/{roms,music,bios}` on boot by `S02piccolo-2go-storage`):
   * `roms/` (`amiga`, `amigacd32`, `atarist`, `megadrive`, `psx`, `snes`, `gba`, `arcade`)
   * `music/` (lossless `.flac` albums, `.dsf`/`.dff` `DSD64`/`DSD128` files, and `.mod`/`.xm`/`.s3m`/`.it` tracker modules)
   * `bios/` (Amiga Kickstart `kick34005.A500` / `kick40068.A1200`, Atari ST `tos.img`, PS1 `scph5501.bin`)

---

## ⚡ 3. First Boot, Storage Expansion & Safe Shutdown (`ax-powerd`)

### ▶️ First Boot & Automatic `SDR50` Verification

1. Keep your stock factory microSD card safely stored as a backup, then insert the **64 GB** card into **`TF1` (OS)** and the **128 GB** card into **`TF2` (Media)**.
2. Hold the **Power** button for `~2` seconds to boot. On first boot, `S02piccolo-2go-storage` verifies **`SDR50` (`50 MHz`, `3.3V` signaling, `~24 MB/s`)** across `mmc0` and `mmc1`, expands `TF1` `p3` (`~59 GB` `ext4`, `noatime,nodiratime,commit=60,data=writeback`), initializes `1024 MB` `LZ4` ZRAM, and binds `TF2` into `/userdata`.

### 🔌 Safe Shutdown (`ax-powerd` on `Core 3`)

* **Always hold `KEY_POWER` for `1.5` seconds** (or press `FN + Power`) to power off. `ax-powerd` sends `SIGUSR1` to release the **HiBy FC3 (`1.3 Vrms`, VID `0x32BB`)** USB-C OTG DAC cleanly, flushes saves, unmounts both SanDisk cards, and powers down the `RK817` PMIC without SD corruption.

---

## 🔧 4. On-Device Hardware Calibration & Clock Profiles (`dtb-select` & `fdtput`)

Because **BATLEXP G350** and **R36S** boards ship across multiple revisions with different MIPI DSI panels, thumbstick wiring, and battery fuel-gauge defaults, `Piccolo-2go` provides `dtb-select` and `dtc` (`fdtput`) for direct on-device or SSH/serial calibration:

### 💾 Backup Your Stock DTB

* Copy your original factory DTB into `/boot/dtbs/factory-backup.dtb` on `TF1` before overwriting it so you can restore stock panel timings anytime via `dtb-select factory-backup.dtb`.

### 🔋 Calibrate the BATLEXP G350 `3,200 mAh` Battery Fuel Gauge (`rk817`)

* Update `design_capacity` and `design_qmax` to **`3200` mAh (`0x0c80`)** in the `rk817` battery node so battery percentage reporting matches the G350's `3,200 mAh` LiPo cell:

```sh
fdtput -t i /boot/rk3326-linux.dtb /i2c@ff3c0000/rk817@20/battery design_capacity 3200
fdtput -t i /boot/rk3326-linux.dtb /i2c@ff3c0000/rk817@20/battery design_qmax 3200
```

### 📡 Gate Unused Onboard SDIO Wi-Fi (`dwmmc@ff380000`)

* Disable the unpopulated onboard SDIO controller in the DTB to reduce idle clock/pin leakage while keeping USB-C OTG adapters functional:

```sh
fdtput -t s /boot/rk3326-linux.dtb /dwmmc@ff380000 status "disabled"
```

### 🎮 Fix Inverted Analog Axes or Deadzones (`odroidgo3-joypad`)

* Toggle axis inversion or analog deadzone values directly in `/boot/rk3326-linux.dtb` without recompiling:

```sh
fdtput -t empty /boot/rk3326-linux.dtb /odroidgo3-joypad invert-absy
fdtput -t i /boot/rk3326-linux.dtb /odroidgo3-joypad button-adc-deadzone 120
```

### 🖥️ `50 Hz` PAL vs. `60 Hz` NTSC Panel Refresh & CPU Clock/Thermal Profiles

* **Authentic `50 Hz` PAL Refresh:** Point `panel@0` `native-mode` to your panel's `50.00 Hz` timing node when supported for tear-free **Amiberry** (Amiga) and **Hatari** (Atari ST) scrolling, then reboot.
* **RK3326 CPU PLL Steps (`24 MHz` Increments, `720 MHz` LPDDR3 @ `1112.5 mV` Default):**
  * `1296000 kHz` (`1.296 GHz` / `1.30 GHz` — Default): Stock safe ceiling for passive, unvented shells (`~42–52°C`).
  * `1344000 kHz` (`1.344 GHz` / `1.34 GHz` — Sweet-Spot `+48 MHz`): Configured via `CPU_PROFILE=1.34` in `/boot/ax-cpu.conf` or `S01piccolo-2go-isolate set-1.34` (`~52–58°C`).
  * `1392000 kHz` (`1.392 GHz` / `1.39 GHz` — High `+96 MHz`): High ceiling just under `1.4 GHz` (`~58–65°C`), guarded by `ax-powerd` automatic **`75°C`** thermal rollback and **`3280 mV`** LiPo brownout shutdown.
